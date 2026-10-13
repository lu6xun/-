#include "ScoreStorage.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

namespace {

#define CHECK(...) do { if (!(__VA_ARGS__)) { \
    throw std::runtime_error(std::string("line ") + std::to_string(__LINE__) + ": " + #__VA_ARGS__); \
} } while (false)

class TemporaryDirectory {
public:
    TemporaryDirectory() {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        for (int attempt = 0; attempt < 100; ++attempt) {
            path = fs::temp_directory_path() / ("grid-snake-tests-" + std::to_string(stamp) + "-" + std::to_string(attempt));
            if (fs::create_directory(path)) { return; }
        }
        throw std::runtime_error("Could not create an isolated test directory");
    }
    ~TemporaryDirectory() {
        std::error_code ignored;
        fs::remove_all(path, ignored); // Only the newly created test directory is removed.
    }
    TemporaryDirectory(const TemporaryDirectory&) = delete;
    TemporaryDirectory& operator=(const TemporaryDirectory&) = delete;
    fs::path path;
};

} // namespace

int main() {
    try {
        TemporaryDirectory temporary;
        const auto file = temporary.path / "records" / "highscore.txt";
        snake::ScoreStorage storage(file);
        CHECK(storage.load() == 0);
        CHECK(storage.save(120));
        CHECK(storage.load() == 120);
        CHECK(storage.save(240));
        CHECK(snake::ScoreStorage(file).load() == 240); // A new instance reads the persisted record.
        CHECK(!storage.save(-1));
        CHECK(storage.load() == 240);
        CHECK(storage.save(0));
        CHECK(storage.load() == 0);

        for (const std::string content : {"", " \t\n", "-10", "hello", "10oops", "10 20", "1.5",
                                          "9999999999999999999999999999999999"}) {
            { std::ofstream output(file); output << content; }
            CHECK(storage.load() == 0);
        }
        { std::ofstream output(file); output << "  90\n\t "; }
        CHECK(storage.load() == 90);

        snake::ScoreStorage unreadable(temporary.path); // A directory cannot be read as a score file.
        CHECK(unreadable.load() == 0);
        CHECK(!unreadable.save(50));
        const auto blocker = temporary.path / "regular-file";
        { std::ofstream output(blocker); output << "block directory creation"; }
        snake::ScoreStorage impossible(blocker / "highscore.txt");
        CHECK(!impossible.save(10));
        CHECK(impossible.load() == 0);
        snake::ScoreStorage unavailable{fs::path{}};
        CHECK(unavailable.load() == 0);
        CHECK(!unavailable.save(10));
        const auto defaultPath = snake::ScoreStorage::defaultPath();
        CHECK(defaultPath.empty() || defaultPath.is_absolute());
        std::cout << "[PASS] score storage: missing, valid, corrupt, overflow, read/write failure\n";
        std::cout << "Default score file: " << defaultPath.u8string() << '\n';
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] score storage: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
