#include <fmt/format.h>
#include "common.hpp"

std::vector<cargo::dataset>
prepare_datasets(const std::string& pattern, size_t n) {
    std::vector<cargo::dataset> datasets;
    datasets.reserve(n);
    for(size_t i = 0; i < n; ++i) {
        datasets.emplace_back(fmt::format(fmt::runtime(pattern), i));
    }

    return datasets;
}
