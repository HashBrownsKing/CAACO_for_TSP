#pragma once

#include "Graph.h"
#include "Config.h"
#include "AttentionMechanism.h"
#include "MMAS.h"
#include <vector>
#include <memory>
#include <utility>
#include <random>

class CCOptimizer {
public:
    CCOptimizer(const Graph& graph, const Config& config);
    CCOptimizer(const CCOptimizer&) = delete;
    CCOptimizer& operator=(const CCOptimizer&) = delete;

    [[nodiscard]] std::pair<std::vector<int>, double> run(size_t global_iterations);

private:
    const Graph& graph_;
    const Config& config_;

    [[nodiscard]] double calculateTourLength(const std::vector<int>& tour) const;
    [[nodiscard]] std::vector<int> generateInitialTour(const std::vector<std::vector<AttentionNode>>& sparse_att) const;
    [[nodiscard]] std::vector<std::vector<double>> buildDenseAttention(const std::vector<std::vector<AttentionNode>>& sparse_att) const;

    void applyTwoOpt(std::vector<int>& tour) const;
    void applyDoubleBridge(std::vector<int>& tour, std::mt19937& rng) const;
};