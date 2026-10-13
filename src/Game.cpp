#include "Game.h"

#include <algorithm>
#include <cmath>
#include <iterator>

namespace snake {

Game::Game(std::uint32_t seed) : random_(seed) {
    resetRound();
}

void Game::resetRound() {
    const Cell center{board::Columns / 2, board::Rows / 2};
    body_ = {center, {center.x - 1, center.y}, {center.x - 2, center.y}};
    direction_ = Direction::Right;
    turns_.clear();
    foodsEaten_ = 0;
    accumulator_ = 0.0;
    collision_.reset();
    food_ = board::makeFood(body_, random_);
}

void Game::start() {
    resetRound();
    state_ = GameState::Playing;
}

void Game::returnToTitle() {
    resetRound();
    state_ = GameState::Title;
}

void Game::togglePause() {
    if (state_ != GameState::Playing && state_ != GameState::Paused) {
        return;
    }
    state_ = state_ == GameState::Playing ? GameState::Paused : GameState::Playing;
    // Discard both pre-pause turns and partial time, so resume is predictable.
    turns_.clear();
    accumulator_ = 0.0;
}

bool Game::queueDirection(Direction direction) {
    if (state_ != GameState::Playing || turns_.size() >= 2) {
        return false;
    }
    // Validate against the last PLANNED direction, not just the current heading.
    // Right -> Up -> Left is legal over two steps; Right -> Up -> Down is not.
    const Direction planned = turns_.empty() ? direction_ : turns_.back();
    if (direction == planned || opposite(direction, planned)) {
        return false;
    }
    turns_.push_back(direction);
    return true;
}

int Game::level() const {
    return 1 + std::min(foodsEaten_ / 5, 8);
}

double Game::stepSeconds() const {
    return static_cast<double>(150 - (level() - 1) * 10) / 1000.0;
}

void Game::update(double elapsedSeconds) {
    if (state_ != GameState::Playing || !std::isfinite(elapsedSeconds) || elapsedSeconds <= 0.0) {
        return;
    }

    // Render frames deposit time; movement withdraws complete fixed steps.
    // Cap a stalled frame at 250 ms to avoid a long burst after dragging/debugging.
    accumulator_ += std::min(elapsedSeconds, 0.25);
    while (state_ == GameState::Playing) {
        const double interval = stepSeconds();
        if (accumulator_ + 1e-9 < interval) {
            break;
        }
        accumulator_ = std::max(0.0, accumulator_ - interval);
        moveOneStep();
    }
}

void Game::moveOneStep() {
    if (!turns_.empty()) {
        direction_ = turns_.front();
        turns_.pop_front(); // At most one buffered turn per actual movement.
    }
    const Cell delta = offset(direction_);
    const Cell next{body_.front().x + delta.x, body_.front().y + delta.y};
    const bool eating = food_ && next == *food_;

    // On a non-growing step, the old tail leaves simultaneously with the head.
    // When growing, include that tail in the collision check.
    const auto collisionEnd = eating ? body_.end() : std::prev(body_.end());
    if (!board::contains(next) || std::find(body_.begin(), collisionEnd, next) != collisionEnd) {
        collision_ = next;
        state_ = GameState::GameOver;
        accumulator_ = 0.0;
        turns_.clear();
        return; // Keep the final valid board for the result screen.
    }

    body_.push_front(next);
    if (eating) {
        ++foodsEaten_;
        food_ = board::makeFood(body_, random_);
        if (!food_) {
            state_ = GameState::Won;
            accumulator_ = 0.0;
            turns_.clear();
        }
    } else {
        body_.pop_back();
    }
}

} // namespace snake
