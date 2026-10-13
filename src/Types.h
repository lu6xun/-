#pragma once

namespace snake {

struct Cell {
    int x = 0;
    int y = 0;
};

constexpr bool operator==(Cell a, Cell b) { return a.x == b.x && a.y == b.y; }
constexpr bool operator!=(Cell a, Cell b) { return !(a == b); }

enum class Direction { Up, Down, Left, Right };
enum class GameState { Title, Playing, Paused, GameOver, Won };

constexpr Cell offset(Direction direction) {
    switch (direction) {
    case Direction::Up: return {0, -1};
    case Direction::Down: return {0, 1};
    case Direction::Left: return {-1, 0};
    case Direction::Right: return {1, 0};
    }
    return {0, 0};
}

constexpr bool opposite(Direction a, Direction b) {
    const Cell first = offset(a);
    const Cell second = offset(b);
    return first.x == -second.x && first.y == -second.y;
}

} // namespace snake
