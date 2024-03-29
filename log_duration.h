#pragma once

#include <chrono>
#include <iostream>

#define PROFILE_CONCAT_INTERNAL(X, Y) X##Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
#define LOG_DURATION(x) LogDuration UNIQUE_VAR_NAME_PROFILE(x)

#define LOG_DURATION_STREAM(x, cout) LogDuration UNIQUE_VAR_NAME_PROFILE(x, cout)

class LogDuration {
public:
    // заменим имя типа std::chrono::steady_clock
    // с помощью using для удобства
    using Clock = std::chrono::steady_clock;

    LogDuration(const std::string& id, std::ostream& out = std::cerr)
            : id_(id),
              out_(out)
    {
    }

    ~LogDuration() {
        using namespace std::chrono;
        using namespace std::literals;

        const auto end_time = Clock::now();
        const auto dur = end_time - start_time_;
        out_ << "Operation time: "s << duration_cast<milliseconds>(dur).count() << " ms"s << std::endl;
    }

private:
    const std::string id_;
    std::ostream& out_;
    const Clock::time_point start_time_ = Clock::now();
};

/**#pragma once

#include <chrono>
#include <iostream>

using namespace std;
using namespace chrono;
using namespace literals;

#define PROFILE_CONCAT_INTERNAL(X, Y) X##Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
#define LOG_DURATION(x) LogDuration UNIQUE_VAR_NAME_PROFILE(x)

class LogDuration {
public:
    LogDuration(const std::string& id) : id_(id) {
    }

    ~LogDuration() {
        const auto end_time = steady_clock::now();
        const auto dur = end_time - start_time_;
        cerr << id_ << ": "s << duration_cast<milliseconds>(dur).count() << " ms"s << endl;
    }

private:
    const std::string id_;
    const steady_clock::time_point start_time_ = steady_clock::now();
};
*/