#include "AStarGrid.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace tcp::logic::path {

namespace {

[[nodiscard]] std::int32_t manhattan(GridCoord a, GridCoord b) noexcept {
    const auto dx = (a.x > b.x) ? (a.x - b.x) : (b.x - a.x);
    const auto dy = (a.y > b.y) ? (a.y - b.y) : (b.y - a.y);
    return dx + dy;
}

[[nodiscard]] bool inBounds(GridCoord p, const GridBounds& b) noexcept {
    return p.x >= b.minX && p.x <= b.maxX && p.y >= b.minY && p.y <= b.maxY;
}

[[nodiscard]] std::array<GridCoord, 4> neighbors(GridCoord p) {
    return std::array<GridCoord, 4>{
        GridCoord{p.x + 1, p.y},
        GridCoord{p.x - 1, p.y},
        GridCoord{p.x, p.y + 1},
        GridCoord{p.x, p.y - 1},
    };
}

struct GridCoordHash {
    [[nodiscard]] std::size_t operator()(GridCoord c) const noexcept {
        return static_cast<std::size_t>(c.x) * 31U + static_cast<std::size_t>(c.y);
    }
};

}  // namespace

std::vector<GridCoord> findPathAStar(GridCoord start, GridCoord goal,
                                     const std::set<GridCoord>& blocked,
                                     const GridBounds& bounds) {
    if (!inBounds(start, bounds) || !inBounds(goal, bounds)) {
        return {};
    }

    if (blocked.find(goal) != blocked.end()) {
        return {};
    }

    if (start == goal) {
        return {start};
    }

    // Build flat blocked bool array for O(1) lookup
    const std::int32_t width = bounds.maxX - bounds.minX + 1;
    const std::int32_t height = bounds.maxY - bounds.minY + 1;
    std::vector<bool> blockedGrid(static_cast<std::size_t>(width * height), false);
    for (const auto& b : blocked) {
        if (inBounds(b, bounds)) {
            const auto idx = static_cast<std::size_t>((b.y - bounds.minY) * width + (b.x - bounds.minX));
            blockedGrid[idx] = true;
        }
    }
    const auto isBlocked = [&](GridCoord p) -> bool {
        if (!inBounds(p, bounds)) return true;
        return blockedGrid[static_cast<std::size_t>((p.y - bounds.minY) * width + (p.x - bounds.minX))];
    };

    using OpenEntry = std::pair<std::int32_t, GridCoord>;  // fScore, coord
    std::vector<OpenEntry> openHeap;
    std::unordered_set<GridCoord, GridCoordHash> openSet;

    std::unordered_set<GridCoord, GridCoordHash> closedSet;
    std::unordered_map<GridCoord, GridCoord, GridCoordHash> cameFrom;
    std::unordered_map<GridCoord, std::int32_t, GridCoordHash> gScore;
    std::unordered_map<GridCoord, std::int32_t, GridCoordHash> fScore;

    gScore[start] = 0;
    fScore[start] = manhattan(start, goal);
    openHeap.emplace_back(fScore[start], start);
    std::push_heap(openHeap.begin(), openHeap.end(), std::greater<>{});
    openSet.insert(start);

    while (!openHeap.empty()) {
        std::pop_heap(openHeap.begin(), openHeap.end(), std::greater<>{});
        const auto [currentF, node] = openHeap.back();
        openHeap.pop_back();
        openSet.erase(node);

        if (node == goal) {
            std::vector<GridCoord> path;
            GridCoord cur = node;
            path.push_back(cur);
            auto it = cameFrom.find(cur);
            while (it != cameFrom.end()) {
                cur = it->second;
                path.push_back(cur);
                it = cameFrom.find(cur);
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        closedSet.insert(node);

        const auto currentG = gScore[node];
        for (const auto& nb : neighbors(node)) {
            if (isBlocked(nb)) {
                continue;
            }

            if (closedSet.find(nb) != closedSet.end()) {
                continue;
            }

            const auto tentativeG = currentG + 1;
            const auto gIt = gScore.find(nb);
            const auto bestKnown =
                (gIt == gScore.end()) ? std::numeric_limits<std::int32_t>::max()
                                      : gIt->second;
            if (tentativeG < bestKnown) {
                cameFrom[nb] = node;
                gScore[nb] = tentativeG;
                const auto newF = tentativeG + manhattan(nb, goal);
                fScore[nb] = newF;

                if (openSet.find(nb) == openSet.end()) {
                    openSet.insert(nb);
                    openHeap.emplace_back(newF, nb);
                    std::push_heap(openHeap.begin(), openHeap.end(), std::greater<>{});
                }
                // If already in open set, we still push a duplicate entry with better fScore;
                // the stale entry will be skipped when popped (detected via closedSet).
            }
        }
    }

    return {};
}

}  // namespace tcp::logic::path
