/**
 * @file timer.h
 * @brief 高精度计时器
 */

#ifndef CRKIT_UTILS_TIMER_H
#define CRKIT_UTILS_TIMER_H

#include <chrono>

namespace crkit {

class Timer {
public:
    Timer() {
        reset();
    }

    void reset() {
        start_ = std::chrono::high_resolution_clock::now();
    }

    // 返回经过的毫秒数
    float elapsed() const {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float, std::milli> duration = end - start_;
        return duration.count();
    }

    // 返回经过的微秒数
    int64_t elapsed_us() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();
    }

    // 获取当前时间戳(毫秒)
    static int64_t timestamp() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_;
};

// RAII风格的作用域计时器
class ScopedTimer {
public:
    explicit ScopedTimer(float* output_ms) : output_(output_ms) {
        timer_.reset();
    }

    ~ScopedTimer() {
        if (output_) {
            *output_ = timer_.elapsed();
        }
    }

private:
    Timer timer_;
    float* output_;
};

} // namespace crkit

#endif // CRKIT_UTILS_TIMER_H
