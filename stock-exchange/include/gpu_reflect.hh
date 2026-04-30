#pragma once
// =============================================================================
// C++26 Reflection — CPU ↔ GPU Struct Mirroring  (P2996)
//
// To enable: switch CXX to clang++ and uncomment -freflection in Makefile.
//
// C++26 REFLECTION SYNTAX:
//   ^T                              reflect on type T → std::meta::info
//   nonstatic_data_members_of(^T)   compile-time range of member reflections
//   [:m:]                           splice: turn reflection back into a name
//   type_of(m)                      reflected type of member m
//   identifier_of(m)                compile-time string name of member
//   size_of(type_of(m))             size in bytes of member's type
// =============================================================================
// #include <experimental/meta>   // uncomment with -freflection
#include <array>
#include <span>
#include <string>
#include <type_traits>
#include "probability_model.hh"
#include "types.hh"

// -----------------------------------------------------------------------------
// 1. gpu_field_count<T>()
//    Counts arithmetic (GPU-transferable) fields at compile time.
//    Replaces fragile: sizeof(T) / sizeof(double)
//
// PSEUDOCODE:
//   template <typename T>
//   consteval std::size_t gpu_field_count() {
//       std::size_t n = 0;
//       // iterate every non-static data member of T at compile time
//       [:nonstatic_data_members_of(^T):] >> [&]<auto m>() {
//           // only count plain arithmetic types — skip string, vector, etc.
//           if constexpr (std::is_arithmetic_v<typename[:type_of(m):]>)
//               ++n;
//       };
//       return n;
//   }
//
// USAGE (probability_model.hh):
//   static constexpr int N_FEATURES = gpu_field_count<MarketFeatures>();
//   // N_FEATURES updates automatically when fields are added/removed
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 2. serialize_to_gpu<T>(src, dst)
//    Flattens a CPU struct into a contiguous double[] for cudaMemcpyAsync.
//    Members are written in declaration order — must match kernel parameter order.
//
// PSEUDOCODE:
//   template <typename T>
//   void serialize_to_gpu(const T& src, std::span<double> dst) {
//       std::size_t i = 0;
//       [:nonstatic_data_members_of(^T):] >> [&]<auto m>() {
//           if constexpr (std::is_arithmetic_v<typename[:type_of(m):]>)
//               dst[i++] = static_cast<double>(src.[:m:]);
//               //                             ^^^^^^^^
//               //   splice: src.[:m:] expands to src.poll_avg,
//               //   src.poll_std, etc. — no runtime name lookup
//       };
//       // assert i == gpu_field_count<T>() at debug time
//   }
//
// USAGE (cuda/kernels.cu, before cudaMemcpyAsync):
//   serialize_to_gpu(features, pinned_host_buf);
//   cudaMemcpyAsync(d_features, pinned_host_buf.data(),
//                   N_FEATURES * sizeof(double), cudaMemcpyHostToDevice, stream);
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 3. deserialize_from_gpu<T>(src)
//    Inverse: GPU output double[] → typed CPU struct.
//    Used after book_stats_kernel or risk_check_kernel writes results back.
//
// PSEUDOCODE:
//   template <typename T>
//   T deserialize_from_gpu(std::span<const double> src) {
//       T dst{};
//       std::size_t i = 0;
//       [:nonstatic_data_members_of(^T):] >> [&]<auto m>() {
//           if constexpr (std::is_arithmetic_v<typename[:type_of(m):]>)
//               // cast back from double to the field's original type
//               // (e.g. int32_t imbalance gets narrowed back correctly)
//               dst.[:m:] = static_cast<typename[:type_of(m):]>(src[i++]);
//       };
//       return dst;
//   }
//
// USAGE (order_book.hh, after book_stats_kernel):
//   cudaMemcpyAsync(host_stats_buf, d_stats, ...);
//   cuda_risk_sync();
//   BookStats stats = deserialize_from_gpu<BookStats>(host_stats_buf);
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 4. to_json<T>(obj)
//    Compile-time field names via identifier_of() → zero-overhead JSON encoding.
//    Replaces hand-rolled snprintf in kalshi_gateway.hh serialize_order().
//
// PSEUDOCODE:
//   template <typename T>
//   std::string to_json(const T& obj) {
//       std::string out = "{";
//       bool first = true;
//       [:nonstatic_data_members_of(^T):] >> [&]<auto m>() {
//           if (!first) out += ',';
//           out += '"';
//           out += identifier_of(m);           // e.g. "ticker", "side", "count"
//           out += "\":";
//           // handle string vs numeric fields differently
//           if constexpr (std::is_same_v<typename[:type_of(m):], std::string>)
//               out += '"' + obj.[:m:] + '"';
//           else
//               out += std::to_string(obj.[:m:]);
//           first = false;
//       };
//       return out += '}';
//   }
//
// USAGE (kalshi_gateway.hh serialize_order()):
//   std::string body = to_json(req);
//   // produces: {"ticker":"KXBTCD-...","side":"yes","type":"limit","count":10,"yes_price":0.65}
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// 5. layouts_match<CpuT, GpuT>()
//    Compile-time static_assert that CPU and GPU mirror structs have identical
//    field count and sizes. Fires at compile time if they diverge.
//
// PSEUDOCODE:
//   template <typename CpuT, typename GpuT>
//   consteval bool layouts_match() {
//       constexpr auto cpu_members = nonstatic_data_members_of(^CpuT);
//       constexpr auto gpu_members = nonstatic_data_members_of(^GpuT);
//       // field count must match
//       if (cpu_members.size() != gpu_members.size()) return false;
//       // each corresponding field must have the same byte size
//       for (auto [cm, gm] : std::views::zip(cpu_members, gpu_members))
//           if (size_of(type_of(cm)) != size_of(type_of(gm))) return false;
//       return true;
//   }
//
//   // Place these in a dedicated layout_checks.hh included by the build:
//   static_assert(layouts_match<MarketFeatures, GpuMarketFeatures>(),
//                 "CPU/GPU MarketFeatures diverged — update both structs");
//   static_assert(layouts_match<BookStats, GpuBookStats>(),
//                 "CPU/GPU BookStats diverged");
// -----------------------------------------------------------------------------

// =============================================================================
// INTEGRATION MAP
//
//   gpu_field_count<MarketFeatures>     probability_model.hh  N_FEATURES constant
//   serialize_to_gpu<MarketFeatures>    cuda/kernels.cu        risk_check input
//   serialize_to_gpu<GpuPosition>       cuda/kernels.cu        fanout input
//   deserialize_from_gpu<BookStats>     order_book.hh          after book_stats_kernel
//   to_json<KalshiOrderRequest>         kalshi_gateway.hh      serialize_order()
//   layouts_match<CpuT, GpuT>          compile-time CI guard  zero runtime cost
// =============================================================================
