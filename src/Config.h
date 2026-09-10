#pragma once
#include <cstddef>
struct Config {
    size_t num_cities = 0;
    size_t num_ants = 20;
    size_t max_iter = 15;
    double alpha = 1.0;
    double beta = 3.0;
    double rho = 0.05;
    double tau_max = 10.0;
    double tau_min = 0.01;
    size_t subpath_len_min = 120;
    size_t subpath_len_max = 280;
};
