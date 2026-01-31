#pragma once
#include <chrono>

class Timer {
private:
    std::chrono::high_resolution_clock::time_point startTime;
    std::chrono::high_resolution_clock::time_point endTime;

public:
    void start() {
        startTime = std::chrono::high_resolution_clock::now();
    }

    double end() {
        endTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = endTime - startTime;
        return elapsed.count();
    }
};