public class PrimMST {
    private:
        vector<Edge> edgeTo;
        vector<int> distanceTo;
        vector<bool> marked;
        PriorityQueue<double> pq;

    public:
        PrimMST(EdgeWeightedGraph graph){
            edgeTo(graph.V());
            distanceTo(graph.V());
            marked(graph.V());
            pq = new PriorityQueue<>(grap.V());

            for(int i = 0; i < graph.V(); ++i)
                distanceTo[i] = std::numeric_limits::infinity();

            for (int v = 0; v < graph.V(); ++i)
                if (!marked[v]) prim(graph, v);
        }
}

int main(){

}
