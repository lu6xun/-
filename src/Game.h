#pragma once

#include "Board.h"

#include <cstdint>
#include <deque>
#include <optional>
#include <random>

namespace snake {

class Game {
public:
    explicit Game(std::uint32_t seed);

    void start();
    void returnToTitle();
    void togglePause();
    bool queueDirection(Direction direction);
    void update(double elapsedSeconds);

    GameState state() const { return state_; }
    const std::deque<Cell>& body() const { return body_; }
    Direction direction() const { return direction_; }
    std::optional<Cell> food() const { return food_; }
    std::optional<Cell> collision() const { return collision_; }
    int score() const { return foodsEaten_ * 10; }
    int level() const;
    double stepSeconds() const;

private:
    // Tests can arrange rare board states without exposing runtime cheat setters.
    friend struct GameTestAccess;

    void resetRound();
    void moveOneStep();

    std::mt19937 random_;
    std::deque<Cell> body_; // Front is the head; back is the tail.
    std::deque<Direction> turns_;
    Direction direction_ = Direction::Right;
    GameState state_ = GameState::Title;
    std::optional<Cell> food_;
    std::optional<Cell> collision_;
    int foodsEaten_ = 0;
    double accumulator_ = 0.0;
};

} // namespace snake
