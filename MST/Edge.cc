#include <stdexcept>
#include <cmath>
#include <string>

public class Edge : Comparable<Edge> {
    private:
        final int u;
        final int v;
        final double weight;

    public:
        Edge(int u, int v, double w){
            if (u < 0) throw std::invalid_argument("vertex index must be non-negative");
            if (v < 0) throw std::invalid_argument("vertex index must be non-negative");
            if (std::isnan(w)) throw std::invalid_argument("weight is NaN");
            this->u = u;
            this->v = v;
            this->weight = w;
        }

        double weight(){ return weight; }

        int other(int vertex){
            if (vertex == v) return u;
            else if (vertex == u) return v;
            else throw std::invalid_argument();
        }

        int compareTo(Edge e){
            return double.Comparable(this.weight, e.weight);
        }


}
