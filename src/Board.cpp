#include "Board.h"

#include <array>
#include <vector>

namespace snake::board {

std::optional<Cell> makeFood(const std::deque<Cell>& snake, std::mt19937& random) {
    std::array<bool, CellCount> occupied{};
    for (const Cell cell : snake) {
        if (contains(cell)) {
            occupied[static_cast<std::size_t>(cell.y * Columns + cell.x)] = true;
        }
    }

    std::vector<Cell> empty;
    empty.reserve(CellCount);
    for (int y = 0; y < Rows; ++y) {
        for (int x = 0; x < Columns; ++x) {
            if (!occupied[static_cast<std::size_t>(y * Columns + x)]) {
                empty.push_back({x, y});
            }
        }
    }
    if (empty.empty()) {
        return std::nullopt;
    }
    std::uniform_int_distribution<std::size_t> pick(0, empty.size() - 1);
    return empty[pick(random)];
}

} // namespace snake::board
