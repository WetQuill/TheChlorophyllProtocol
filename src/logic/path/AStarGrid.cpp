#include "AStarGrid.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <map>
#include <set>
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

    std::set<GridCoord> openSet;
    openSet.insert(start);

    std::set<GridCoord> closedSet;
    std::map<GridCoord, GridCoord> cameFrom;
    std::map<GridCoord, std::int32_t> gScore;
    std::map<GridCoord, std::int32_t> fScore;

    gScore[start] = 0;
    fScore[start] = manhattan(start, goal);

    while (!openSet.empty()) {
        GridCoord current{};
        bool hasCurrent = false;
        std::int32_t bestF = std::numeric_limits<std::int32_t>::max();
        for (const auto& node : openSet) {
            const auto it = fScore.find(node);
            const auto nodeF = (it == fScore.end())
                                   ? std::numeric_limits<std::int32_t>::max()
                                   : it->second;
            if (!hasCurrent || nodeF < bestF ||
                (nodeF == bestF && node < current)) {
                current = node;
                bestF = nodeF;
                hasCurrent = true;
            }
        }

        if (!hasCurrent) {
            return {};
        }

        if (current == goal) {
            std::vector<GridCoord> path;
            path.push_back(current);
            while (cameFrom.find(current) != cameFrom.end()) {
                current = cameFrom[current];
                path.push_back(current);
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        openSet.erase(current);
        closedSet.insert(current);

        const auto currentG = gScore[current];
        for (const auto& nb : neighbors(current)) {
            if (!inBounds(nb, bounds) || blocked.find(nb) != blocked.end()) {
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
                cameFrom[nb] = current;
                gScore[nb] = tentativeG;
                fScore[nb] = tentativeG + manhattan(nb, goal);
                openSet.insert(nb);
            }
        }
    }

    return {};
}

}  // namespace tcp::logic::path
