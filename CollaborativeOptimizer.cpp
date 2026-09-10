#include "CollaborativeOptimizer.h"
#include <iostream>
#include <iomanip>
#include <future>
#include <algorithm>
#include <limits>
#include <stdexcept>

CCOptimizer::CCOptimizer(const Graph& graph, const Config& config)
    : graph_(graph), config_(config) {}

[[nodiscard]] double CCOptimizer::calculateTourLength(const std::vector<int>& tour) const {
    if (tour.empty()) return 0.0;
    double length = 0.0;
    size_t n = tour.size();
    for (size_t i = 0; i < n - 1; ++i) {
        length += graph_.getDistance(tour[i], tour[i + 1]);
    }
    length += graph_.getDistance(tour.back(), tour.front());
    return length;
}

void CCOptimizer::applyTwoOpt(std::vector<int>& tour) const {
    size_t n = tour.size();
    bool improvement = true;
    while (improvement) {
        improvement = false;
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = i + 2; j < n; ++j) {
                if (i == 0 && j == n - 1) continue;
                int n1 = tour[i], n2 = tour[(i + 1) % n], n3 = tour[j], n4 = tour[(j + 1) % n];
                double d_old = graph_.getDistance(n1, n2) + graph_.getDistance(n3, n4);
                double d_new = graph_.getDistance(n1, n3) + graph_.getDistance(n2, n4);
                if (d_new < d_old - 1e-5) {
                    std::reverse(tour.begin() + i + 1, tour.begin() + j + 1);
                    improvement = true;
                }
            }
        }
    }
}

void CCOptimizer::applyDoubleBridge(std::vector<int>& tour, std::mt19937& rng) const {
    size_t n = tour.size();
    if (n < 8) return;
    size_t max_span = std::min(n / 4, static_cast<size_t>(600));
    std::uniform_int_distribution<size_t> start_dist(0, n - 1);
    size_t start_pos = start_dist(rng);
    std::vector<size_t> pos(4);
    std::uniform_int_distribution<size_t> offset_dist(2, std::max<size_t>(3, max_span / 4));
    pos[0] = start_pos;
    for (int i = 1; i < 4; i++) {
        pos[i] = (pos[i - 1] + offset_dist(rng)) % n;
    }
    std::sort(pos.begin(), pos.end());
    size_t a = pos[0], b = pos[1], c = pos[2], d = pos[3];
    std::vector<int> new_tour;
    new_tour.reserve(n);
    new_tour.insert(new_tour.end(), tour.begin(), tour.begin() + a + 1);
    new_tour.insert(new_tour.end(), tour.begin() + c + 1, tour.begin() + d + 1);
    new_tour.insert(new_tour.end(), tour.begin() + b + 1, tour.begin() + c + 1);
    new_tour.insert(new_tour.end(), tour.begin() + a + 1, tour.begin() + b + 1);
    new_tour.insert(new_tour.end(), tour.begin() + d + 1, tour.end());
    tour = std::move(new_tour);
}

[[nodiscard]] std::vector<int> CCOptimizer::generateInitialTour(const std::vector<std::vector<AttentionNode>>& sparse_att) const {
    size_t n = graph_.getNumCities();
    std::vector<int> tour; tour.reserve(n);
    std::vector<bool> visited(n, false);
    int current_city = 0;
    tour.push_back(current_city);
    visited[current_city] = true;
    for (size_t i = 1; i < n; ++i) {
        int next_city = -1; double max_att = -1.0;
        for (const auto& node : sparse_att[current_city]) {
            if (!visited[node.city_id] && node.weight > max_att) {
                max_att = node.weight; next_city = node.city_id;
            }
        }
        if (next_city == -1) {
            double min_dist = std::numeric_limits<double>::max();
            for (size_t j = 0; j < n; ++j) {
                if (!visited[j]) {
                    double dist = graph_.getDistance(current_city, j);
                    if (dist < min_dist) { min_dist = dist; next_city = j; }
                }
            }
        }
        tour.push_back(next_city); visited[next_city] = true; current_city = next_city;
    }
    return tour;
}

[[nodiscard]] std::vector<std::vector<double>> CCOptimizer::buildDenseAttention(const std::vector<std::vector<AttentionNode>>& sparse_att) const {
    size_t n = graph_.getNumCities();
    std::vector<std::vector<double>> dense_att(n, std::vector<double>(n, 1e-6));
    for (size_t i = 0; i < n; ++i) {
        for (const auto& node : sparse_att[i]) dense_att[i][node.city_id] = node.weight;
    }
    return dense_att;
}

[[nodiscard]] std::pair<std::vector<int>, double> CCOptimizer::run(size_t global_iterations) {
    size_t num_cities = graph_.getNumCities();
    Attention attention(graph_, std::min(num_cities - 1, static_cast<size_t>(50)));
    auto sparse_att = attention.computeAttentionWeights();
    auto dense_att = buildDenseAttention(sparse_att);
    std::vector<int> current_tour = generateInitialTour(sparse_att);
    applyTwoOpt(current_tour);
    double best_length = calculateTourLength(current_tour);
    std::cout << "  [System] Initial Tour Length (After 2-Opt): " << std::fixed << std::setprecision(2) << best_length << std::endl;
    std::cout << "-----------------------------------------------------" << std::endl;
    MMASOptimizer mmas(graph_, config_);
    std::vector<int> global_best_tour = current_tour;
    std::mt19937 rng(std::random_device{}());
    size_t stagnation_counter = 0;
    for (size_t iter = 0; iter < global_iterations; ++iter) {
        size_t min_len = config_.subpath_len_min;
        size_t max_len = config_.subpath_len_max;
        if (min_len > max_len) std::swap(min_len, max_len);
        std::uniform_int_distribution<size_t> dist_chunk(min_len, max_len);
        size_t chunk_size = std::min(dist_chunk(rng), num_cities);
        std::uniform_int_distribution<size_t> dist_shift(0, chunk_size - 1);
        size_t shift_offset = dist_shift(rng);
        if (shift_offset > 0) {
            std::rotate(current_tour.begin(), current_tour.begin() + shift_offset, current_tour.end());
        }
        std::vector<std::future<std::vector<int>>> futures;
        std::vector<std::vector<int>> chunks;
        for (size_t i = 0; i < num_cities; i += chunk_size) {
            size_t end_idx = std::min(i + chunk_size, num_cities);
            chunks.emplace_back(current_tour.begin() + i, current_tour.begin() + end_idx);
        }
        for (const auto& chunk : chunks) {
            futures.push_back(std::async(std::launch::async, [&mmas, &dense_att, chunk]() {
                return mmas.optimizeSubPath(chunk, dense_att);
                }));
        }
        std::vector<int> next_tour;
        next_tour.reserve(num_cities);
        for (auto& fut : futures) {
            auto optimized_chunk = fut.get();
            next_tour.insert(next_tour.end(), optimized_chunk.begin(), optimized_chunk.end());
        }
        current_tour = std::move(next_tour);
        if (shift_offset > 0) {
            std::rotate(current_tour.begin(), current_tour.end() - shift_offset, current_tour.end());
        }
        applyTwoOpt(current_tour);
        double current_length = calculateTourLength(current_tour);
        std::cout << "  [Iter " << std::setw(3) << iter + 1 << "/" << global_iterations
            << "] Current Length: " << std::fixed << std::setprecision(2) << current_length;
        if (current_length < best_length - 1e-5) {
            best_length = current_length;
            global_best_tour = current_tour;
            stagnation_counter = 0;
            std::cout << "  <-- NEW BEST!";
        }
        else {
            stagnation_counter++;
        }
        std::cout << std::endl;
        if (stagnation_counter > 0) {
            if (stagnation_counter % 30 == 0) {
                current_tour = global_best_tour;
                applyDoubleBridge(current_tour, rng);
                applyDoubleBridge(current_tour, rng);
                applyTwoOpt(current_tour);
                std::cout << "  [System] Severe Stagnation. Applied Heavy Perturbation." << std::endl;
            }
            else if (stagnation_counter % 10 == 0) {
                current_tour = global_best_tour;
                applyDoubleBridge(current_tour, rng);
                applyTwoOpt(current_tour);
                std::cout << "  [System] Deep Valley Detected. Applied Mild Perturbation." << std::endl;
            }
        }
    }
    return { global_best_tour, best_length };
}
