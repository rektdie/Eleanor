#pragma once
#include <chrono>

class Stopwatch {
private:
    std::chrono::steady_clock::time_point start = 
        std::chrono::steady_clock::now();
public:
    void Restart() {
        start = std::chrono::steady_clock::now();
    }

    int GetElapsedMS() {
        auto current = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(current - start).count();
    }

    double GetElapsedSec() {
        auto current = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<
            std::chrono::duration<double>>(current - start).count();
    }
};