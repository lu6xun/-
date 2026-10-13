#include "Renderer.h"

#include "raylib.h"

#include <algorithm>
#include <string>

namespace snake {
namespace {

constexpr Color Background{14, 22, 31, 255};
constexpr Color Tile{19, 31, 41, 255};
constexpr Color AlternateTile{21, 34, 44, 255};
constexpr Color Grid{31, 46, 56, 255};
constexpr Color Ink{231, 241, 242, 255};
constexpr Color Muted{149, 174, 185, 255};
constexpr Color Head{126, 236, 175, 255};
constexpr Color Body{56, 172, 130, 255};
constexpr Color Food{255, 133, 105, 255};

void centered(const std::string& text, int y, int size, Color color) {
    DrawText(text.c_str(), (Renderer::Width - MeasureText(text.c_str(), size)) / 2, y, size, color);
}

void panel(int top, int height) {
    DrawRectangle(0, Renderer::StatusHeight, Renderer::Width,
                  Renderer::Height - Renderer::StatusHeight, Color{7, 13, 20, 160});
    DrawRectangle(38, top, Renderer::Width - 76, height, Color{19, 31, 42, 246});
    DrawRectangleLines(38, top, Renderer::Width - 76, height, Grid);
}

} // namespace

void Renderer::draw(const Game& game, int bestScore, bool saveFailed) const {
    ClearBackground(Background);
    drawBoard(game);
    drawStatus(game, bestScore, saveFailed);
    drawOverlay(game, bestScore);
}

void Renderer::drawBoard(const Game& game) const {
    for (int y = 0; y < board::Rows; ++y) {
        for (int x = 0; x < board::Columns; ++x) {
            DrawRectangle(x * CellPixels, StatusHeight + y * CellPixels, CellPixels, CellPixels,
                          (x + y) % 2 == 0 ? Tile : AlternateTile);
        }
    }
    for (int x = 0; x <= board::Columns; ++x) {
        DrawLine(x * CellPixels, StatusHeight, x * CellPixels, Height, Grid);
    }
    for (int y = 0; y <= board::Rows; ++y) {
        DrawLine(0, StatusHeight + y * CellPixels, Width, StatusHeight + y * CellPixels, Grid);
    }

    if (game.food()) {
        const Cell food = *game.food();
        const int x = food.x * CellPixels + CellPixels / 2;
        const int y = StatusHeight + food.y * CellPixels + CellPixels / 2;
        DrawCircle(x, y, 8.0F, Food);
        DrawRectangle(x + 1, y - 10, 3, 5, Head);
    }

    bool isHead = true;
    for (const Cell cell : game.body()) {
        DrawRectangle(cell.x * CellPixels + 2, StatusHeight + cell.y * CellPixels + 2,
                      CellPixels - 4, CellPixels - 4, isHead ? Head : Body);
        isHead = false;
    }
    const Cell head = game.body().front();
    const Cell facing = offset(game.direction());
    const int eyeX = head.x * CellPixels + CellPixels / 2 + facing.x * 5;
    const int eyeY = StatusHeight + head.y * CellPixels + CellPixels / 2 + facing.y * 5;
    DrawCircle(eyeX - facing.y * 4, eyeY + facing.x * 4, 2.0F, Background);
    DrawCircle(eyeX + facing.y * 4, eyeY - facing.x * 4, 2.0F, Background);

    if (game.collision()) {
        // A wall impact is projected onto the edge cell; a body impact marks its cell.
        const Cell hit = *game.collision();
        const int x = std::clamp(hit.x, 0, board::Columns - 1) * CellPixels;
        const int y = StatusHeight + std::clamp(hit.y, 0, board::Rows - 1) * CellPixels;
        DrawRectangleLinesEx(Rectangle{static_cast<float>(x + 1), static_cast<float>(y + 1),
                                     22.0F, 22.0F}, 2.0F, Food);
        DrawLine(x + 7, y + 7, x + 17, y + 17, Food);
        DrawLine(x + 17, y + 7, x + 7, y + 17, Food);
    }
}

void Renderer::drawStatus(const Game& game, int bestScore, bool saveFailed) const {
    DrawRectangle(0, 0, Width, StatusHeight, Background);
    DrawText("GRID SNAKE", 18, 12, 20, Head);
    const std::string score = "SCORE " + std::to_string(game.score());
    const std::string best = "BEST " + std::to_string(bestScore);
    DrawText(score.c_str(), 218, 14, 18, Ink);
    DrawText(best.c_str(), 405, 14, 18, Ink);
    const std::string difficulty = "LEVEL " + std::to_string(game.level()) + "  /  " +
        std::to_string(static_cast<int>(game.stepSeconds() * 1000.0 + 0.5)) + " ms";
    DrawText(difficulty.c_str(), 18, 43, 16, Muted);
    DrawText("P / SPACE pause    ESC exit", 284, 43, 16, Muted);
    if (saveFailed) {
        DrawText("Best score is kept in memory (save unavailable)", 18, 66, 10, Food);
    }
    DrawLine(0, StatusHeight - 1, Width, StatusHeight - 1, Body);
}

void Renderer::drawOverlay(const Game& game, int bestScore) const {
    switch (game.state()) {
    case GameState::Title:
        panel(156, 328);
        centered("GRID SNAKE", 185, 40, Head);
        centered("A CLASSIC, ONE CELL AT A TIME", 238, 16, Muted);
        centered("ARROWS / WASD   Move", 286, 20, Ink);
        centered("P / SPACE   Pause & resume", 318, 20, Ink);
        centered("Eat food. Grow. Avoid walls & yourself.", 362, 16, Muted);
        centered("ENTER TO START", 405, 26, Head);
        centered("BEST " + std::to_string(bestScore), 450, 16, Muted);
        break;
    case GameState::Paused:
        panel(245, 160);
        centered("PAUSED", 273, 34, Ink);
        centered("P / SPACE TO RESUME", 340, 20, Head);
        break;
    case GameState::GameOver:
    case GameState::Won:
        // A compact result card leaves most of the final board visible.
        panel(108, 153);
        centered(game.state() == GameState::Won ? "BOARD COMPLETE!" : "GAME OVER", 126, 30,
                 game.state() == GameState::Won ? Head : Food);
        centered("SCORE " + std::to_string(game.score()) + "    BEST " + std::to_string(bestScore),
                 171, 20, Ink);
        centered("R  Play again     ENTER  Title", 219, 18, Muted);
        break;
    case GameState::Playing:
        break;
    }
}

} // namespace snake
