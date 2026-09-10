#include "MMAS.h"
#include <cmath>
#include <limits>
#include <stdexcept>
#include <numeric>
#include <algorithm>

MMASOptimizer::MMASOptimizer(const Graph& graph, const Config& config)
    : graph_(graph), config_(config) {}

void MMASOptimizer::applyLocalTwoOpt(Ant& ant, const std::vector<int>& subNodes) const {
    size_t n = ant.path.size();
    if (n < 4) return;
    bool improvement = true;
    while (improvement) {
        improvement = false;
        for (size_t i = 1; i < n - 2; ++i) {
            for (size_t j = i + 1; j < n - 1; ++j) {
                int g_i_minus_1 = subNodes[ant.path[i - 1]];
                int g_i = subNodes[ant.path[i]];
                int g_j = subNodes[ant.path[j]];
                int g_j_plus_1 = subNodes[ant.path[j + 1]];
                double d_old = graph_.getDistance(g_i_minus_1, g_i) + graph_.getDistance(g_j, g_j_plus_1);
                double d_new = graph_.getDistance(g_i_minus_1, g_j) + graph_.getDistance(g_i, g_j_plus_1);
                if (d_new < d_old - 1e-5) {
                    std::reverse(ant.path.begin() + i, ant.path.begin() + j + 1);
                    improvement = true;
                }
            }
        }
    }
}

[[nodiscard]] std::vector<int> MMASOptimizer::optimizeSubPath(
    const std::vector<int>& subNodes,
    const std::vector<std::vector<double>>& attentionWeights) {
    const size_t sub_size = subNodes.size();
    if (sub_size <= 3) return subNodes;
    std::vector<std::vector<double>> local_eta(sub_size, std::vector<double>(sub_size, 0.0));
    std::vector<std::vector<double>> local_attention(sub_size, std::vector<double>(sub_size, 0.0));
    for (size_t i = 0; i < sub_size; ++i) {
        for (size_t j = 0; j < sub_size; ++j) {
            if (i == j) continue;
            int global_i = subNodes[i];
            int global_j = subNodes[j];
            double dist = graph_.getDistance(global_i, global_j);
            double raw_eta = 1.0 / (dist + 1e-6);
            local_eta[i][j] = std::pow(raw_eta, config_.beta);
            local_attention[i][j] = attentionWeights[global_i][global_j];
        }
    }
    Ant global_best_ant;
    global_best_ant.reset(sub_size);
    for (size_t i = 0; i < sub_size; ++i) {
        global_best_ant.path.push_back(static_cast<int>(i));
        global_best_ant.visited[i] = true;
    }
    applyLocalTwoOpt(global_best_ant, subNodes);
    global_best_ant.path_length = 0.0;
    for (size_t i = 0; i < sub_size - 1; ++i) {
        global_best_ant.path_length += graph_.getDistance(
            subNodes[global_best_ant.path[i]], subNodes[global_best_ant.path[i + 1]]);
    }
    double tau_max_init = 1.0 / (config_.rho * global_best_ant.path_length);
    std::vector<std::vector<double>> local_pheromone(sub_size, std::vector<double>(sub_size, tau_max_init));
    std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
    size_t actual_ants = config_.num_ants;
    std::vector<Ant> ants(actual_ants);
    for (size_t iter = 0; iter < config_.max_iter; ++iter) {
        Ant iteration_best_ant;
        iteration_best_ant.path_length = std::numeric_limits<double>::max();
        for (auto& ant : ants) {
            ant.reset(sub_size);
            int current_local_idx = 0;
            ant.path.push_back(current_local_idx);
            ant.visited[current_local_idx] = true;
            ant.visited[sub_size - 1] = true;
            for (size_t step = 1; step < sub_size - 1; ++step) {
                std::vector<double> transition_probs(sub_size, 0.0);
                double prob_sum = 0.0;
                for (size_t next_local_idx = 0; next_local_idx < sub_size; ++next_local_idx) {
                    if (!ant.visited[next_local_idx]) {
                        double tau = local_pheromone[current_local_idx][next_local_idx];
                        double att = local_attention[current_local_idx][next_local_idx];
                        double eta = local_eta[current_local_idx][next_local_idx];
                        double pow_tau = (config_.alpha == 1.0) ? tau : std::pow(tau, config_.alpha);
                        double numerator = att * pow_tau * eta;
                        transition_probs[next_local_idx] = numerator;
                        prob_sum += numerator;
                    }
                }
                int next_city_idx = -1;
                if (prob_sum > 1e-15) {
                    double rand_val = prob_dist(rng) * prob_sum;
                    double cumulative_prob = 0.0;
                    for (size_t j = 0; j < sub_size; ++j) {
                        if (!ant.visited[j]) {
                            cumulative_prob += transition_probs[j];
                            if (cumulative_prob >= rand_val) {
                                next_city_idx = j;
                                break;
                            }
                        }
                    }
                }
                if (next_city_idx == -1) {
                    for (size_t j = 0; j < sub_size; ++j) {
                        if (!ant.visited[j]) { next_city_idx = j; break; }
                    }
                }
                ant.path.push_back(next_city_idx);
                ant.visited[next_city_idx] = true;
                current_local_idx = next_city_idx;
            }
            int end_local_idx = sub_size - 1;
            ant.path.push_back(end_local_idx);
            ant.path_length = 0.0;
            for (size_t i = 0; i < sub_size - 1; ++i) {
                ant.path_length += graph_.getDistance(subNodes[ant.path[i]], subNodes[ant.path[i + 1]]);
            }
            if (ant.path_length < iteration_best_ant.path_length) {
                iteration_best_ant = ant;
            }
        }
        applyLocalTwoOpt(iteration_best_ant, subNodes);
        iteration_best_ant.path_length = 0.0;
        for (size_t i = 0; i < sub_size - 1; ++i) {
            iteration_best_ant.path_length += graph_.getDistance(subNodes[iteration_best_ant.path[i]], subNodes[iteration_best_ant.path[i + 1]]);
        }
        if (iteration_best_ant.path_length < global_best_ant.path_length) {
            global_best_ant = iteration_best_ant;
        }
        double tau_max = 1.0 / (config_.rho * global_best_ant.path_length);
        double tau_min = tau_max / (2.0 * sub_size);
        for (size_t i = 0; i < sub_size; ++i) {
            for (size_t j = 0; j < sub_size; ++j) {
                local_pheromone[i][j] *= (1.0 - config_.rho);
            }
        }
        double delta_tau = 1.0 / global_best_ant.path_length;
        for (size_t step = 0; step < sub_size - 1; ++step) {
            int u = global_best_ant.path[step];
            int v = global_best_ant.path[step + 1];
            local_pheromone[u][v] += delta_tau;
            local_pheromone[v][u] += delta_tau;
        }
        for (size_t i = 0; i < sub_size; ++i) {
            for (size_t j = 0; j < sub_size; ++j) {
                if (local_pheromone[i][j] > tau_max) local_pheromone[i][j] = tau_max;
                else if (local_pheromone[i][j] < tau_min) local_pheromone[i][j] = tau_min;
            }
        }
    }
    std::vector<int> optimized_sub_nodes(sub_size);
    for (size_t i = 0; i < sub_size; ++i) {
        optimized_sub_nodes[i] = subNodes[global_best_ant.path[i]];
    }
    return optimized_sub_nodes;
}
