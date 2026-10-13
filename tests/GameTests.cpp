#include "Game.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace snake {

struct GameTestAccess {
    static void arrange(Game& game, std::deque<Cell> body, Direction direction,
                        Cell food, int eaten = 0) {
        game.start();
        game.body_ = std::move(body);
        game.direction_ = direction;
        game.food_ = food;
        game.foodsEaten_ = eaten;
    }
};

} // namespace snake

namespace {

using namespace snake;

#define CHECK(...) do { if (!(__VA_ARGS__)) { \
    throw std::runtime_error(std::string("line ") + std::to_string(__LINE__) + ": " + #__VA_ARGS__); \
} } while (false)

Game straight() {
    Game game(12345);
    GameTestAccess::arrange(game, {{12, 10}, {11, 10}, {10, 10}}, Direction::Right, {0, 0});
    return game;
}

void step(Game& game) { game.update(game.stepSeconds()); }

void initialState() {
    Game game(12);
    CHECK(game.state() == GameState::Title);
    const auto body = game.body();
    game.update(1.0);
    CHECK(game.body() == body);
    CHECK(!game.queueDirection(Direction::Up));
    game.start();
    CHECK(game.body().size() == 3);
    CHECK(game.body().front() == Cell{12, 10});
    CHECK(game.direction() == Direction::Right);
    CHECK(game.score() == 0);
    CHECK(std::abs(game.stepSeconds() - 0.15) < 1e-9);
}

void normalMovement() {
    auto game = straight();
    step(game);
    CHECK(game.body() == std::deque<Cell>({{13, 10}, {12, 10}, {11, 10}}));
    CHECK(game.score() == 0);
    CHECK(game.state() == GameState::Playing);
}

void eating() {
    Game game(8);
    GameTestAccess::arrange(game, {{12, 10}, {11, 10}, {10, 10}}, Direction::Right, {13, 10});
    step(game);
    CHECK(game.body() == std::deque<Cell>({{13, 10}, {12, 10}, {11, 10}, {10, 10}}));
    CHECK(game.score() == 10);
    CHECK(game.food().has_value());
    CHECK(std::find(game.body().begin(), game.body().end(), *game.food()) == game.body().end());
}

void reverseAndRepeat() {
    auto game = straight();
    CHECK(!game.queueDirection(Direction::Left));
    CHECK(!game.queueDirection(Direction::Right));
    step(game);
    CHECK(game.body().front() == Cell{13, 10});
    CHECK(game.queueDirection(Direction::Up));
    CHECK(!game.queueDirection(Direction::Down));
    CHECK(!game.queueDirection(Direction::Up));
    step(game);
    CHECK(game.body().front() == Cell{13, 9});
}

void rapidTurns() {
    auto game = straight();
    CHECK(game.queueDirection(Direction::Up));
    CHECK(game.queueDirection(Direction::Left));
    CHECK(!game.queueDirection(Direction::Down)); // Capacity is two, not three.
    step(game);
    CHECK(game.body().front() == Cell{12, 9});
    CHECK(game.direction() == Direction::Up);
    step(game);
    CHECK(game.body().front() == Cell{11, 9});
    CHECK(game.direction() == Direction::Left);
    step(game);
    CHECK(game.body().front() == Cell{10, 9}); // Rejected third turn did not linger.
    CHECK(game.state() == GameState::Playing);
}

void plannedDirection() {
    auto game = straight();
    CHECK(game.queueDirection(Direction::Up));
    CHECK(game.queueDirection(Direction::Left));
    step(game);
    CHECK(!game.queueDirection(Direction::Right)); // Opposite to pending Left.
    CHECK(game.queueDirection(Direction::Down));
    step(game);
    CHECK(game.body().front() == Cell{11, 9});
    step(game);
    CHECK(game.body().front() == Cell{11, 10});
    CHECK(game.state() == GameState::Playing);
}

void walls() {
    const std::vector<std::pair<Cell, Direction>> cases{
        {{23, 10}, Direction::Right}, {{0, 10}, Direction::Left},
        {{12, 0}, Direction::Up}, {{12, 19}, Direction::Down}};
    for (const auto& entry : cases) {
        const Cell head = entry.first;
        const Cell delta = offset(entry.second);
        Game game(9);
        GameTestAccess::arrange(game,
            {head, {head.x - delta.x, head.y - delta.y}, {head.x - 2 * delta.x, head.y - 2 * delta.y}},
            entry.second, {2, 2});
        const auto before = game.body();
        step(game);
        CHECK(game.state() == GameState::GameOver);
        CHECK(game.body() == before);
        CHECK(game.collision() == std::optional<Cell>({head.x + delta.x, head.y + delta.y}));
        CHECK(!game.queueDirection(Direction::Up));
        game.update(0.25);
        CHECK(game.body() == before);
    }
}

void selfCollision() {
    Game game(7);
    GameTestAccess::arrange(game, {{5, 5}, {5, 6}, {4, 6}, {4, 5}, {4, 4}, {5, 4}},
                           Direction::Up, {0, 0}, 3);
    const auto before = game.body();
    CHECK(game.queueDirection(Direction::Left));
    step(game);
    CHECK(game.state() == GameState::GameOver);
    CHECK(game.collision() == std::optional<Cell>({4, 5}));
    CHECK(game.body() == before);
    CHECK(game.score() == 30);
}

void departingTail() {
    Game game(7);
    GameTestAccess::arrange(game, {{5, 5}, {5, 6}, {4, 6}, {4, 5}}, Direction::Up, {0, 0}, 1);
    CHECK(game.queueDirection(Direction::Left));
    step(game);
    CHECK(game.state() == GameState::Playing);
    CHECK(game.body() == std::deque<Cell>({{4, 5}, {5, 5}, {5, 6}, {4, 6}}));
    CHECK(game.score() == 10);
}

void foodPlacement() {
    std::mt19937 random(42);
    std::deque<Cell> almostFull;
    for (int y = 0; y < board::Rows; ++y) {
        for (int x = 0; x < board::Columns; ++x) {
            if (Cell{x, y} != Cell{7, 11}) { almostFull.push_back({x, y}); }
        }
    }
    for (int i = 0; i < 30; ++i) {
        CHECK(board::makeFood(almostFull, random) == std::optional<Cell>({7, 11}));
    }
    almostFull.push_back({7, 11});
    CHECK(!board::makeFood(almostFull, random));

    Game game(1234);
    for (int i = 0; i < 200; ++i) {
        game.start();
        CHECK(game.food().has_value());
        CHECK(board::contains(*game.food()));
        CHECK(std::find(game.body().begin(), game.body().end(), *game.food()) == game.body().end());
    }
}

void deterministicSeed() {
    Game first(400);
    Game second(400);
    for (int i = 0; i < 50; ++i) {
        first.start();
        second.start();
        CHECK(first.food() == second.food());
    }
}

void victory() {
    std::deque<Cell> body;
    // One continuous serpentine path covers every cell except (0,0).
    for (int y = 0; y < board::Rows; ++y) {
        for (int i = 0; i < board::Columns; ++i) {
            const int x = y % 2 == 0 ? i : board::Columns - 1 - i;
            if (Cell{x, y} != Cell{0, 0}) { body.push_back({x, y}); }
        }
    }
    Game game(1);
    GameTestAccess::arrange(game, body, Direction::Left, {0, 0}, board::CellCount - 4);
    step(game);
    CHECK(game.state() == GameState::Won);
    CHECK(game.body().size() == board::CellCount);
    CHECK(game.body().front() == Cell{0, 0});
    CHECK(game.score() == (board::CellCount - 3) * 10);
    CHECK(!game.food());
    const auto finalBoard = game.body();
    game.update(0.25);
    CHECK(game.body() == finalBoard);
    CHECK(!game.queueDirection(Direction::Up));
    game.start();
    CHECK(game.state() == GameState::Playing);
    CHECK(game.body().size() == 3);
    CHECK(game.food().has_value());
    CHECK(game.score() == 0);
}

void restart() {
    Game game(3);
    GameTestAccess::arrange(game, {{5, 5}, {4, 5}, {3, 5}, {2, 5}}, Direction::Right, {6, 5}, 9);
    step(game);
    CHECK(game.score() == 100);
    CHECK(game.queueDirection(Direction::Down));
    CHECK(game.queueDirection(Direction::Left));
    game.update(0.04);
    game.start();
    CHECK(game.state() == GameState::Playing);
    CHECK(game.score() == 0);
    CHECK(game.level() == 1);
    CHECK(game.body().size() == 3);
    CHECK(game.body().front() == Cell{12, 10});
    CHECK(!game.collision());
    game.update(0.11);
    CHECK(game.body().front() == Cell{12, 10});
    game.update(0.04);
    CHECK(game.body().front() == Cell{13, 10}); // No old turns or time leaked.

    GameTestAccess::arrange(game, {{23, 1}, {22, 1}, {21, 1}}, Direction::Right, {0, 0});
    step(game);
    CHECK(game.collision().has_value());
    game.start();
    CHECK(game.state() == GameState::Playing);
    CHECK(!game.collision());
    game.togglePause();
    game.start();
    CHECK(game.state() == GameState::Playing);
}

void pause() {
    auto game = straight();
    game.update(0.14);
    CHECK(game.queueDirection(Direction::Up));
    game.togglePause();
    const auto before = game.body();
    CHECK(game.state() == GameState::Paused);
    CHECK(!game.queueDirection(Direction::Down));
    for (int i = 0; i < 20; ++i) { game.update(0.25); }
    CHECK(game.body() == before);
    CHECK(game.score() == 0);
    game.togglePause();
    game.update(0.01);
    CHECK(game.body() == before);
    game.update(0.14);
    CHECK(game.body().front() == Cell{13, 10});
}

void titleTransition() {
    auto game = straight();
    game.queueDirection(Direction::Up);
    game.update(0.10);
    game.returnToTitle();
    const auto before = game.body();
    game.togglePause();
    game.update(0.25);
    CHECK(game.state() == GameState::Title);
    CHECK(game.body() == before);
    CHECK(!game.queueDirection(Direction::Left));
    game.start();
    game.update(0.05);
    CHECK(game.body() == before);
    game.update(0.10);
    CHECK(game.body().front() == Cell{13, 10});
}

void fixedTimeStep() {
    auto first = straight();
    auto second = straight();
    for (int i = 0; i < 60; ++i) { first.update(1.0 / 60.0); }
    for (int i = 0; i < 10; ++i) { second.update(0.1); }
    CHECK(first.body() == second.body());
    CHECK(first.body().front() == Cell{18, 10});
    auto third = straight();
    third.queueDirection(Direction::Up);
    third.queueDirection(Direction::Left);
    third.update(0.10);
    third.update(0.20);
    CHECK(third.body().front() == Cell{11, 9}); // Catch-up consumes one turn per step.
}

void invalidTime() {
    auto game = straight();
    const auto before = game.body();
    game.update(-1.0);
    game.update(0.0);
    game.update(std::numeric_limits<double>::infinity());
    game.update(std::numeric_limits<double>::quiet_NaN());
    CHECK(game.body() == before);
    game.update(60.0); // Stalled frames are capped at 250 ms.
    CHECK(game.body().front() == Cell{13, 10});
    CHECK(game.state() == GameState::Playing);
}

void difficulty() {
    for (int count : {0, 3, 4, 8, 9, 38, 39, 40, 80}) {
        Game game(17);
        GameTestAccess::arrange(game, {{12, 10}, {11, 10}, {10, 10}}, Direction::Right, {13, 10}, count);
        const double before = static_cast<double>(150 - std::min(count / 5, 8) * 10) / 1000.0;
        CHECK(std::abs(game.stepSeconds() - before) < 1e-9);
        step(game);
        CHECK(game.score() == (count + 1) * 10);
        const double after = static_cast<double>(150 - std::min((count + 1) / 5, 8) * 10) / 1000.0;
        CHECK(std::abs(game.stepSeconds() - after) < 1e-9);
        CHECK(game.stepSeconds() >= 0.07);
        game.update(after - 0.001);
        CHECK(game.body().front() == Cell{13, 10});
        game.update(0.001);
        CHECK(game.body().front() == Cell{14, 10});
    }
}

} // namespace

int main(int argc, char** argv) {
    const std::vector<std::pair<std::string, std::function<void()>>> tests{
        {"initial_state", initialState}, {"normal_movement", normalMovement}, {"eating", eating},
        {"reverse_and_repeat", reverseAndRepeat}, {"rapid_turns", rapidTurns},
        {"planned_direction", plannedDirection}, {"walls", walls}, {"self_collision", selfCollision},
        {"departing_tail", departingTail}, {"food_placement", foodPlacement},
        {"deterministic_seed", deterministicSeed}, {"victory", victory}, {"restart", restart},
        {"pause", pause}, {"title_transition", titleTransition}, {"fixed_timestep", fixedTimeStep},
        {"invalid_time", invalidTime}, {"difficulty", difficulty}};
    int failures = 0;
    int executed = 0;
    for (const auto& test : tests) {
        if (argc > 1 && test.first != argv[1]) { continue; }
        ++executed;
        try {
            test.second();
            std::cout << "[PASS] " << test.first << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "[FAIL] " << test.first << ": " << error.what() << '\n';
        }
    }
    if (executed == 0) { std::cerr << "Unknown test name.\n"; return 1; }
    return failures == 0 ? 0 : 1;
}
