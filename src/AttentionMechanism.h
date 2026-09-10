#pragma once
#include "Graph.h"
#include <vector>
#include <utility>
#include <cstddef>

struct AttentionNode {
    size_t city_id;
    double weight;
};

class Attention {
public:
    Attention(const Graph& graph, size_t k_neighbors);
    Attention(const Attention&) = delete;
    Attention& operator=(const Attention&) = delete;
    [[nodiscard]] std::vector<std::vector<AttentionNode>> computeAttentionWeights() const;
private:
    const Graph& graph_;
    size_t k_;
    std::vector<std::pair<double, double>> norm_features_;
    void preprocessFeatures();
};
