#pragma once

#include <algorithm>
#include <vector>

namespace tcp::logic {

template <typename T>
void sortDeterministic(std::vector<T>& values) {
    std::sort(values.begin(), values.end());
}

}  // namespace tcp::logic
