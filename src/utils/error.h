/**
 * @file error.h
 * @brief 错误处理
 */

#ifndef CRKIT_UTILS_ERROR_H
#define CRKIT_UTILS_ERROR_H

#include "crkit_api.h"
#include <string>
#include <mutex>

namespace crkit {

class ErrorHandler {
public:
    static ErrorHandler& instance();

    void setLastError(CRKitStatus status, const std::string& message);
    const char* getLastError() const;
    CRKitStatus getLastStatus() const;

private:
    ErrorHandler() = default;
    ~ErrorHandler() = default;
    ErrorHandler(const ErrorHandler&) = delete;
    ErrorHandler& operator=(const ErrorHandler&) = delete;

    mutable std::mutex mutex_;
    std::string last_error_;
    CRKitStatus last_status_ = CRKIT_SUCCESS;
};

// 宏定义，简化错误设置
#define SET_ERROR(status, msg) \
    crkit::ErrorHandler::instance().setLastError(status, msg)

#define SET_ERROR_AND_RETURN(status, msg) \
    do { \
        crkit::ErrorHandler::instance().setLastError(status, msg); \
        return status; \
    } while(0)

} // namespace crkit

#endif // CRKIT_UTILS_ERROR_H
