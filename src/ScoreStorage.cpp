#include "ScoreStorage.h"

#include <cstdlib>
#include <fstream>
#include <system_error>
#include <utility>

namespace snake {
namespace {

std::filesystem::path environmentPath(const char* name) {
#ifdef _WIN32
    // Read Windows environment paths as UTF-16, including non-ASCII user names.
    std::wstring wideName;
    for (const char* ch = name; *ch; ++ch) {
        wideName.push_back(static_cast<wchar_t>(*ch));
    }
    const wchar_t* value = _wgetenv(wideName.c_str());
#else
    const char* value = std::getenv(name);
#endif
    if (value) {
        const std::filesystem::path path(value);
        if (path.is_absolute()) {
            return path;
        }
    }
    return {};
}

} // namespace

ScoreStorage::ScoreStorage(std::filesystem::path file) : file_(std::move(file)) {}

std::filesystem::path ScoreStorage::defaultPath() noexcept {
    try {
#ifdef _WIN32
        auto base = environmentPath("LOCALAPPDATA");
        if (base.empty()) {
            const auto profile = environmentPath("USERPROFILE");
            if (!profile.empty()) {
                base = profile / "AppData" / "Local";
            }
        }
        if (!base.empty()) {
            return base / "GridSnake" / "highscore.txt";
        }
#elif defined(__APPLE__)
        const auto userHome = environmentPath("HOME");
        if (!userHome.empty()) {
            return userHome / "Library" / "Application Support" / "GridSnake" / "highscore.txt";
        }
#else
        auto base = environmentPath("XDG_DATA_HOME");
        if (base.empty()) {
            const auto userHome = environmentPath("HOME");
            if (!userHome.empty()) {
                base = userHome / ".local" / "share";
            }
        }
        if (!base.empty()) {
            return base / "GridSnake" / "highscore.txt";
        }
#endif
    } catch (...) {
        // No writable user location: keep records in memory, never in the source tree.
    }
    return {};
}

int ScoreStorage::load() const noexcept {
    try {
        std::ifstream input(file_);
        int score = 0;
        if (!(input >> score) || score < 0) {
            return 0;
        }
        input >> std::ws;
        if (!input.eof() || input.bad()) {
            return 0; // Reject trailing garbage or a second number.
        }
        return score;
    } catch (...) {
        return 0;
    }
}

bool ScoreStorage::save(int score) const noexcept {
    if (file_.empty() || score < 0) {
        return false;
    }
    try {
        std::error_code error;
        if (!file_.parent_path().empty()) {
            std::filesystem::create_directories(file_.parent_path(), error);
            if (error) {
                return false;
            }
        }
        std::ofstream output(file_, std::ios::trunc);
        output << score << '\n';
        output.close();
        return !output.fail();
    } catch (...) {
        return false;
    }
}

} // namespace snake
