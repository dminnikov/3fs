/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <folly/logging/Hf3fsLogFormatter.h>
#include <folly/logging/LogMessage.h>
#include <folly/system/ThreadId.h>
#include <folly/system/ThreadName.h>

namespace folly {
namespace {
std::string_view getLevelName(LogLevel level) {
  if (level < LogLevel::INFO) {
    return "DEBUG";
  } else if (level < LogLevel::WARN) {
    return "INFO";
  } else if (level < LogLevel::ERR) {
    return "WARNING";
  } else if (level < LogLevel::CRITICAL) {
    return "ERROR";
  } else if (level < LogLevel::DFATAL) {
    return "CRITICAL";
  }
  return "FATAL";
}

auto localtime(std::chrono::system_clock::time_point ts) {
  return fmt::localtime(std::chrono::system_clock::to_time_t(ts));
}
}  // namespace

std::string Hf3fsLogFormatter::formatMessage(const LogMessage &message, const LogCategory * /*handlerCategory*/) {
  // this implementation (except the thread_local optimization) takes from GlogStyleFormatter.
  thread_local auto cachedSeconds =
      std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::time_point().time_since_epoch());
  thread_local std::string cachedDateTimeStr;
  thread_local uint64_t cachedThreadId = 0;
  thread_local std::string cachedThreadIdStr;
  thread_local Optional<std::string> currentThreadName;
  thread_local auto currentOsThreadId = getOSThreadID();

  auto timestamp = message.getTimestamp();

  // NOTE: assume nobody will change the time zone in runtime
  static const auto timeZoneStr = fmt::format("{:%Ez}", localtime(timestamp));

  // Get the local time info
  auto timeSinceEpoch = timestamp.time_since_epoch();
  auto epochSeconds = std::chrono::duration_cast<std::chrono::seconds>(timeSinceEpoch);
  auto nsecs = std::chrono::duration_cast<std::chrono::nanoseconds>(timeSinceEpoch) - epochSeconds;

  if (cachedSeconds != epochSeconds) {
    cachedSeconds = epochSeconds;
    cachedDateTimeStr = fmt::format("{:%FT%H:%M:%S}", localtime(timestamp));
  }

  std::string_view threadName;
  {
    if (!currentThreadName) {
      currentThreadName = getCurrentThreadName();
    }

    auto tid = message.getThreadID();
    if (tid == currentOsThreadId && currentThreadName) {
      threadName = *currentThreadName;
    }

    if (tid != cachedThreadId) {
      cachedThreadId = tid;
      cachedThreadIdStr = fmt::format("{:5d}", tid);
    }
  }

  auto basename = message.getFileBaseName();
  auto header = fmt::format("[{}.{:09d}{} {}:{} {}:{} {}] ",
                            cachedDateTimeStr,
                            nsecs.count(),
                            timeZoneStr,
                            threadName,
                            cachedThreadIdStr,
                            basename,
                            message.getLineNumber(),
                            getLevelName(message.getLevel()));

  // Format the data into a buffer.
  std::string buffer;
  StringPiece msgData{message.getMessage()};
  if (message.containsNewlines()) {
    // If there are multiple lines in the log message, add a header
    // before each one.

    buffer.reserve(((header.size() + 1) * message.getNumNewlines()) + msgData.size());

    size_t idx = 0;
    while (true) {
      auto end = msgData.find('\n', idx);
      if (end == StringPiece::npos) {
        end = msgData.size();
      }

      buffer.append(header);
      auto line = msgData.subpiece(idx, end - idx);
      buffer.append(line.data(), line.size());
      buffer.push_back('\n');

      if (end == msgData.size()) {
        break;
      }
      idx = end + 1;
    }
  } else {
    buffer.reserve(header.size() + msgData.size());
    buffer.append(header);
    buffer.append(msgData.data(), msgData.size());
    buffer.push_back('\n');
  }

  return buffer;
}
}  // namespace folly
