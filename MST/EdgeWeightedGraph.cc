class EdgeWeightedGraph {
    private:
        final int V;
        int E;
        vector<Bag<Edge>> adj;

    public:
        EdgeWeightedGraph(int V){
            if (V < 0) throw std::invalid_argument();
            this->V = V;
            this->E = 0;
            adj(V);

            for(int v = 0; v < V; ++v)
                adj[v] = Bag();
        }

}
