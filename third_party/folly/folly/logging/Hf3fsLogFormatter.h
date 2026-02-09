#pragma once

#include <folly/Range.h>
#include <folly/logging/LogFormatter.h>

namespace folly {
class Hf3fsLogFormatter : public LogFormatter {
 public:
  std::string formatMessage(const LogMessage &message, const LogCategory *) override;
};

}  // namespace folly
