#include "include/types.hh"
#include "include/spmc_ring.hh"
#include "include/order_book.hh"
#include "include/matching_engine.hh"
#include "include/sequencer.hh"
#include "include/order_manager.hh"
#include "include/gateway.hh"
#include "include/perf.hh"
#include <thread>
#include <csignal>
#include <atomic>

// =============================================================================
// Pipeline topology (single-symbol example):
//
//   [Broker TCP]
//       │  FIX bytes
//       ▼
//   Gateway::on_fix_message()
//       │  Order
//       ▼
//   OrderManager::submit()        ← validates, assigns order_id
//       │  Order
//       ▼
//   Sequencer::sequence()         ← stamps seq_num, release-pushes to ring
//       │
//   OrderRing  (SPMC, 1024 slots)
//       │
//       ▼
//   MatchingEngine::run()         ← pinned core, spins on ring
//       │  Execution
//   ExecutionRing (SPMC, 1024 slots)
//       │
//       ▼
//   OrderManager::process_executions()
//       │  Execution
//       ▼
//   Gateway::encode_execution()   → FIX bytes → Broker TCP
// =============================================================================

// Shared rings — producer/consumer boundary between pipeline stages
static OrderRing     order_ring;
static ExecutionRing exec_ring;

// Global metrics (defined here, declared extern in perf.hh)
ExchangeMetrics g_metrics;

static std::atomic<bool> g_running{true};

void signal_handler(int) { g_running.store(false, std::memory_order_relaxed); }

int main() {
    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    // -------------------------------------------------------------------------
    // Init perf counters before any threads start
    // -------------------------------------------------------------------------
    g_metrics.init();

    // -------------------------------------------------------------------------
    // Construct pipeline components
    // -------------------------------------------------------------------------
    Sequencer     sequencer(order_ring);
    MatchingEngine engine(order_ring, exec_ring);
    OrderManager  om(order_ring, exec_ring);
    Gateway       gateway(om);

    // Register execution callback: OM → gateway → encode FIX → send
    om.on_execution([&](const Execution& exec) {
        // TODO: write encoded FIX to the broker's TCP send buffer
        uint8_t buf[512];
        gateway.encode_execution(exec, buf);
    });

    // -------------------------------------------------------------------------
    // Thread layout:
    //   T0 (main)      : gateway I/O loop (recv FIX, call on_fix_message)
    //   T1             : matching engine (pinned, spins on order_ring)
    //   T2             : execution drain (spins on exec_ring → om.process_executions)
    //   T3             : monitoring (reads perf counters, prints histograms)
    //
    // Pin T1 to an isolated core (isolcpus= in kernel cmdline) for determinism.
    // -------------------------------------------------------------------------

    // T1: matching engine
    std::thread engine_thread([&] {
        // TODO: pin thread to isolated core via pthread_setaffinity_np
        engine.run(); // spins until engine.stop()
    });

    // T2: execution drain
    std::thread exec_thread([&] {
        while (g_running.load(std::memory_order_relaxed)) {
            om.process_executions();
            // TODO: yield or spin based on measured latency budget
        }
    });

    // T3: monitoring — runs every second, never on hot path
    std::thread monitor_thread([&] {
        while (g_running.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            g_metrics.report();
            g_metrics.reset();
        }
    });

    // T0 (main): gateway I/O loop
    while (g_running.load(std::memory_order_relaxed)) {
        // TODO: poll TCP socket (epoll/io_uring), read FIX bytes into buf
        // FIXMessage msg{buf, len};
        // auto t0 = rdtsc_start();
        // gateway.on_fix_message(msg);
        // g_metrics.gateway_parse.record(ticks_to_ns(rdtsc_end() - t0, TSC_HZ));
    }

    engine.stop();
    engine_thread.join();
    exec_thread.join();
    monitor_thread.join();

    g_metrics.report();
    return 0;
}
