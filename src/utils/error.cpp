/**
 * @file error.cpp
 * @brief 错误处理实现
 */

#include "error.h"
#include "logger.h"

namespace crkit {

ErrorHandler& ErrorHandler::instance() {
    static ErrorHandler handler;
    return handler;
}

void ErrorHandler::setLastError(CRKitStatus status, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    last_status_ = status;
    last_error_ = message;

    if (status != CRKIT_SUCCESS) {
        LOG_ERROR("Error %d: %s", static_cast<int>(status), message.c_str());
    }
}

const char* ErrorHandler::getLastError() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_error_.c_str();
}

CRKitStatus ErrorHandler::getLastStatus() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_status_;
}

} // namespace crkit
