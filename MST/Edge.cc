#pragma once
#include <cmath>
#include <stdexcept>

class Edge {
  private:
    int u = -1, v = -1;
    double w = 0.0;

  public:
    Edge() = default;
    Edge(int u, int v, double w) {
        if (u < 0) throw std::invalid_argument("vertex index must be non-negative");
        if (v < 0) throw std::invalid_argument("vertex index must be non-negative");
        if (std::isnan(w)) throw std::invalid_argument("weight is NaN");
        this->u = u;
        this->v = v;
        this->w = w;
    }

    double weight() { return w; }

    int other(int vertex) {
        if (vertex == v) return u;
        else if (vertex == u) return v;
        else throw std::invalid_argument("invalid vertex");
    }

    auto operator<=>(const Edge &e) const { return w <=> e.w; }
};
