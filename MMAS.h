#pragma once

#include "Graph.h"
#include "Config.h"
#include <vector>
#include <random>

struct Ant {
    std::vector<int> path;
    std::vector<bool> visited;
    double path_length = 0.0;

    void reset(size_t sub_size) {
        path.clear();
        path.reserve(sub_size);
        visited.assign(sub_size, false);
        path_length = 0.0;
    }
};

class MMASOptimizer {
public:
    MMASOptimizer(const Graph& graph, const Config& config);
    MMASOptimizer(const MMASOptimizer&) = delete;
    MMASOptimizer& operator=(const MMASOptimizer&) = delete;

    [[nodiscard]] std::vector<int> optimizeSubPath(
        const std::vector<int>& subNodes,
        const std::vector<std::vector<double>>& attentionWeights);

private:
    const Graph& graph_;
    const Config& config_;

    void applyLocalTwoOpt(Ant& ant, const std::vector<int>& subNodes) const;
};