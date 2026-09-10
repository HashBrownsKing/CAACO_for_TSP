#include "AttentionMechanism.h"
#include <cmath>
#include <numeric>
#include <algorithm>
#include <stdexcept>
#include <limits>

Attention::Attention(const Graph& graph, size_t k_neighbors)
    : graph_(graph), k_(k_neighbors) {
    if (k_ == 0 || k_ >= graph_.getNumCities()) {
        throw std::invalid_argument("Attention Error: Invalid K neighbors value.");
    }
    preprocessFeatures();
}

void Attention::preprocessFeatures() {
    size_t num_cities = graph_.getNumCities();
    norm_features_.reserve(num_cities);
    double sum_x = 0.0, sum_y = 0.0;
    for (size_t i = 0; i < num_cities; ++i) {
        auto [x, y] = graph_.getCoord(i);
        sum_x += x;
        sum_y += y;
    }
    double mean_x = sum_x / num_cities;
    double mean_y = sum_y / num_cities;
    for (size_t i = 0; i < num_cities; ++i) {
        auto [x, y] = graph_.getCoord(i);
        double dx = x - mean_x;
        double dy = y - mean_y;
        double norm = std::hypot(dx, dy);
        if (norm > 1e-6) {
            norm_features_.emplace_back(dx / norm, dy / norm);
        }
        else {
            norm_features_.emplace_back(0.0, 0.0);
        }
    }
}

[[nodiscard]] std::vector<std::vector<AttentionNode>> Attention::computeAttentionWeights() const {
    size_t num_cities = graph_.getNumCities();
    std::vector<std::vector<AttentionNode>> attention_matrix(num_cities);
    for (auto& row : attention_matrix) {
        row.reserve(k_);
    }
    const double sqrt_dk = std::sqrt(2.0);
    for (size_t i = 0; i < num_cities; ++i) {
        std::vector<size_t> indices(num_cities);
        std::iota(indices.begin(), indices.end(), 0);
        std::swap(indices[i], indices.back());
        auto dist_cmp = [this, i](size_t a, size_t b) {
            return graph_.getDistance(i, a) < graph_.getDistance(i, b);
            };
        std::nth_element(indices.begin(), indices.begin() + k_, indices.end() - 1, dist_cmp);
        std::vector<double> scores(k_);
        double max_score = std::numeric_limits<double>::lowest();
        const auto& [qi_x, qi_y] = norm_features_[i];
        for (size_t idx = 0; idx < k_; ++idx) {
            size_t j = indices[idx];
            const auto& [kj_x, kj_y] = norm_features_[j];
            double qk_dot = qi_x * kj_x + qi_y * kj_y;
            double distance = graph_.getDistance(i, j);
            double b_ij = 100.0 / (distance + 1.0);
            double score = (qk_dot / sqrt_dk) + b_ij;
            scores[idx] = score;
            if (score > max_score) {
                max_score = score;
            }
        }
        double sum_exp = 0.0;
        std::vector<double> exps(k_);
        for (size_t idx = 0; idx < k_; ++idx) {
            exps[idx] = std::exp(scores[idx] - max_score);
            sum_exp += exps[idx];
        }
        for (size_t idx = 0; idx < k_; ++idx) {
            double a_ij = exps[idx] / sum_exp;
            attention_matrix[i].push_back({ indices[idx], a_ij });
        }
    }
    return attention_matrix;
}
