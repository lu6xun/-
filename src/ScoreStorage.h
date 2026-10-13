#pragma once

#include <filesystem>

namespace snake {

class ScoreStorage {
public:
    explicit ScoreStorage(std::filesystem::path file);

    static std::filesystem::path defaultPath() noexcept;
    int load() const noexcept;
    bool save(int score) const noexcept;
    const std::filesystem::path& path() const { return file_; }

private:
    std::filesystem::path file_;
};

} // namespace snake
