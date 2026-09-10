#include <iostream>
#include <chrono>
#include <string>
#include <exception>
#include <iomanip>
#include "Config.h"
#include "Graph.h"
#include "CollaborativeOptimizer.h"
int main() {
    std::string dataset_path = "dataset_path";
    size_t global_iterations = 1000;
    try {
        std::cout << "=====================================================" << std::endl;
        std::cout << "             CAACO-TSP Optimization Engine           " << std::endl;
        std::cout << "=====================================================" << std::endl;
        Config config;
        std::cout << "[Target] Loading dataset : " << dataset_path << std::endl;
        Graph graph(dataset_path);
        std::cout << "[Target] Total cities    : " << graph.getNumCities() << std::endl;
        std::cout << "=====================================================" << std::endl;
        auto start_time = std::chrono::high_resolution_clock::now();
        CCOptimizer optimizer(graph, config);
        auto [best_tour, best_length] = optimizer.run(global_iterations);
        auto end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed_seconds = end_time - start_time;
        std::cout << "=====================================================" << std::endl;
        std::cout << "                 Optimization Finished!              " << std::endl;
        std::cout << "=====================================================" << std::endl;
        std::cout << "   Best Path Length : " << std::fixed << std::setprecision(2) << best_length << std::endl;
        std::cout << "    Total Time Cost  : " << elapsed_seconds.count() << " seconds" << std::endl;
        std::cout << "=====================================================" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "\n[Fatal Error] " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
