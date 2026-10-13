#pragma once

#include "Types.h"

#include <deque>
#include <optional>
#include <random>

namespace snake::board {

inline constexpr int Columns = 24;
inline constexpr int Rows = 20;
inline constexpr int CellCount = Columns * Rows;

constexpr bool contains(Cell cell) {
    return cell.x >= 0 && cell.x < Columns && cell.y >= 0 && cell.y < Rows;
}

// A finite list of empty cells also handles a full board without retry loops.
std::optional<Cell> makeFood(const std::deque<Cell>& snake, std::mt19937& random);

} // namespace snake::board
