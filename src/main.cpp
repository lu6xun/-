#include "Game.h"
#include "Renderer.h"
#include "ScoreStorage.h"

#include "raylib.h"

#include <chrono>
#include <cstdint>
#include <exception>
#include <iostream>
#include <optional>
#include <random>

namespace {

class Window {
public:
    Window() {
        InitWindow(snake::Renderer::Width, snake::Renderer::Height, "Grid Snake");
        SetTargetFPS(60);
        SetExitKey(KEY_ESCAPE);
    }
    ~Window() { if (IsWindowReady()) { CloseWindow(); } }
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
};

std::uint32_t randomSeed() {
    try {
        return std::random_device{}();
    } catch (...) {
        return static_cast<std::uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    }
}

std::optional<snake::Direction> directionForKey(int key) {
    switch (key) {
    case KEY_UP: case KEY_W: return snake::Direction::Up;
    case KEY_DOWN: case KEY_S: return snake::Direction::Down;
    case KEY_LEFT: case KEY_A: return snake::Direction::Left;
    case KEY_RIGHT: case KEY_D: return snake::Direction::Right;
    default: return std::nullopt;
    }
}

// Returns true on a state transition: that frame's old elapsed time is discarded.
bool handleInput(snake::Game& game) {
    bool transitioned = false;
    // GetKeyPressed preserves event order even when multiple keys arrive in one frame.
    for (int key = GetKeyPressed(); key != 0; key = GetKeyPressed()) {
        const auto state = game.state();
        if (state == snake::GameState::Title && key == KEY_ENTER) {
            game.start();
            transitioned = true;
        } else if (state == snake::GameState::GameOver || state == snake::GameState::Won) {
            if (key == KEY_R) {
                game.start();
                transitioned = true;
            } else if (key == KEY_ENTER) {
                game.returnToTitle();
                transitioned = true;
            }
        } else if ((state == snake::GameState::Playing || state == snake::GameState::Paused) &&
                   (key == KEY_P || key == KEY_SPACE)) {
            game.togglePause();
            transitioned = true;
        } else if (const auto direction = directionForKey(key)) {
            game.queueDirection(*direction);
        }
    }
    return transitioned;
}

} // namespace

int main() {
    try {
        snake::ScoreStorage storage(snake::ScoreStorage::defaultPath());
        int bestScore = storage.load();
        bool saveFailed = storage.path().empty();
        snake::Game game(randomSeed());
        const snake::Renderer renderer;
        const Window window;
        if (!IsWindowReady()) {
            std::cerr << "Could not create the game window.\n";
            return 1;
        }
        while (!WindowShouldClose()) {
            const double elapsed = static_cast<double>(GetFrameTime());
            if (!handleInput(game)) {
                game.update(elapsed);
            }
            if (game.score() > bestScore) {
                bestScore = game.score();
                saveFailed = !storage.save(bestScore);
            }
            BeginDrawing();
            renderer.draw(game, bestScore, saveFailed);
            EndDrawing();
        }
        // One final retry is harmless if a transient save failure occurred.
        if (saveFailed && bestScore > 0) {
            storage.save(bestScore);
        }
    } catch (const std::exception& error) {
        std::cerr << "Grid Snake: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
