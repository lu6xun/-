#pragma once

#include "Game.h"

namespace snake {

class Renderer {
public:
    static constexpr int CellPixels = 24;
    static constexpr int StatusHeight = 88;
    static constexpr int Width = board::Columns * CellPixels;
    static constexpr int Height = StatusHeight + board::Rows * CellPixels;

    void draw(const Game& game, int bestScore, bool saveFailed) const;

private:
    void drawBoard(const Game& game) const;
    void drawStatus(const Game& game, int bestScore, bool saveFailed) const;
    void drawOverlay(const Game& game, int bestScore) const;
};

} // namespace snake
