#pragma once
#include <vector>
#include <string>
#include <utility>

class Graph {
public:
    [[nodiscard]] std::pair<double, double> getCoord(size_t i) const;
    explicit Graph(const std::string& filepath);
    Graph(const Graph&) = delete;
    Graph& operator=(const Graph&) = delete;
    Graph(Graph&&) noexcept = default;
    Graph& operator=(Graph&&) noexcept = default;
    [[nodiscard]] double getDistance(size_t i, size_t j) const;
    [[nodiscard]] size_t getNumCities() const;
private:
    size_t num_cities_;
    std::vector<std::pair<double, double>> coords_;
    std::vector<std::vector<double>> dist_matrix_;
    void parseFile(const std::string& filepath);
    void computeDistanceMatrix();
};
