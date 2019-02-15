#pragma once

#include <seasocks/Logger.h>
#include "logging.hpp"

#include <cstdio>

namespace seasocks {

class PlogLogger : public Logger {
public:
    explicit PlogLogger() {}

    ~PlogLogger() = default;

    virtual void log(Level level, const char* message) override {
      switch(level) {
        case Level::Debug:
          LOG_DEBUG << message;
          break;
        case Level::Access:
        case Level::Info:
          LOG_INFO << message;
          break;
        case Level::Warning:
          LOG_WARNING << message;
          break;
        case Level::Error:
        case Level::Severe:
          LOG_ERROR << message;
          break;
      }
    }
};

} // namespace seasocks
