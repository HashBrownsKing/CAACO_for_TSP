#include "Graph.h"

#include <fstream>
#include <sstream>
#include <cmath>
#include <stdexcept>
#include <filesystem>
#include <iostream>

Graph::Graph(const std::string& filepath) : num_cities_(0) {
    if (!std::filesystem::exists(filepath)) {
        throw std::runtime_error("Graph Error: TSPLIB file does not exist at " + filepath);
    }

    parseFile(filepath);
    computeDistanceMatrix();
}

[[nodiscard]] double Graph::getDistance(size_t i, size_t j) const {
    return dist_matrix_[i][j];
}

[[nodiscard]] size_t Graph::getNumCities() const {
    return num_cities_;
}

void Graph::parseFile(const std::string& filepath) {
    std::ifstream infile(filepath);
    if (!infile.is_open()) {
        throw std::runtime_error("Graph Error: Failed to open file " + filepath);
    }

    std::string line;
    bool node_section = false;

    while (std::getline(infile, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty()) continue;

        if (line.find("EOF") != std::string::npos) {
            break;
        }

        if (line.find("DIMENSION") != std::string::npos) {
            auto colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                num_cities_ = std::stoull(line.substr(colon_pos + 1));
                coords_.reserve(num_cities_);
            }
        }
        else if (line.find("NODE_COORD_SECTION") != std::string::npos) {
            node_section = true;
            if (num_cities_ == 0) {
                throw std::runtime_error("Graph Error: DIMENSION not found before NODE_COORD_SECTION");
            }
            continue;
        }

        if (node_section) {
            std::istringstream iss(line);
            size_t id;
            double x, y;
            if (iss >> id >> x >> y) {
                coords_.emplace_back(x, y);
            }
        }
    }

    if (coords_.size() != num_cities_) {
        throw std::runtime_error("Graph Error: Parsed node count does not match DIMENSION");
    }
}

void Graph::computeDistanceMatrix() {
    dist_matrix_.resize(num_cities_);
    for (size_t i = 0; i < num_cities_; ++i) {
        dist_matrix_[i].resize(num_cities_, 0.0);
    }

    for (size_t i = 0; i < num_cities_; ++i) {
        for (size_t j = i + 1; j < num_cities_; ++j) {
            double dx = coords_[i].first - coords_[j].first;
            double dy = coords_[i].second - coords_[j].second;

            double dist = std::round(std::hypot(dx, dy));

            dist_matrix_[i][j] = dist;
            dist_matrix_[j][i] = dist;
        }
    }
}

[[nodiscard]] std::pair<double, double> Graph::getCoord(size_t i) const {
    return coords_[i];
}
