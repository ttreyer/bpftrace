#pragma once

#include <cassert>
#include <iostream>
#include <optional>
#include <sstream>
#include <unordered_map>

#include "ast/location.h"

namespace bpftrace {

namespace LogColor {
constexpr const char* RESET = "\033[0m";
constexpr const char* RED = "\033[31m";
constexpr const char* YELLOW = "\033[33m";
constexpr const char* DEFAULT = "";
} // namespace LogColor

// clang-format off
enum class LogType
{
  DEBUG,
  V1,
  HINT,
  WARNING,
  ERROR,
  BUG,
};
// clang-format on

class Log {
public:
  static Log& get();
  void take_input(LogType type,
                  std::optional<ast::Location> loc,
                  std::ostream& out,
                  std::string&& input);
  inline void set_source(std::string_view filename, std::string_view source)
  {
    src_ = source;
    filename_ = filename;
  }
  inline const std::string& get_source()
  {
    return src_;
  }
  const std::string get_source_line(unsigned int n);

  // Can only construct with get()
  Log(const Log& other) = delete;
  Log& operator=(const Log& other) = delete;

  inline void enable(LogType type)
  {
    enabled_map_[type] = true;
  }
  inline void disable(LogType type)
  {
    assert(type != LogType::BUG && type != LogType::ERROR);
    enabled_map_[type] = false;
  }
  inline bool is_enabled(LogType type)
  {
    return enabled_map_[type];
  }

  inline void set_colorize(bool is_colorize)
  {
    is_colorize_ = is_colorize;
  }

private:
  Log();
  ~Log() = default;
  std::string src_;
  std::string filename_;
  void log_with_location(LogType,
                         ast::Location,
                         std::ostream&,
                         const std::string&);
  std::string log_format_output(LogType, std::string&&);
  std::unordered_map<LogType, bool> enabled_map_;
  bool is_colorize_ = false;
};

class LogStream {
public:
  LogStream(const std::string& file,
            int line,
            LogType type,
            std::ostream& out = std::cerr);
  LogStream(const std::string& file,
            int line,
            LogType type,
            ast::Location loc,
            std::ostream& out = std::cerr);
  template <typename T>
  LogStream& operator<<(const T& v)
  {
    if (sink_.is_enabled(type_))
      buf_ << v;
    return *this;
  }
  virtual ~LogStream();

protected:
  std::string internal_location();

  Log& sink_;
  LogType type_;
  std::optional<ast::Location> loc_;
  std::ostream& out_;
  std::string log_file_;
  int log_line_;
  std::ostringstream buf_;
};

class LogStreamBug : public LogStream {
public:
  LogStreamBug(const std::string& file,
               int line,
               __attribute__((unused)) LogType,
               std::ostream& out = std::cerr)
      : LogStream(file, line, LogType::BUG, out) {};
  [[noreturn]] ~LogStreamBug() override;
};

// Usage examples:
// 1. LOG(WARNING) << "this is a " << "warning!"; (this goes to std::cerr)
// 2. LOG(DEBUG, std::cout) << "this is a " << " message.";
// 3. LOG(ERROR, call.loc, std::cerr) << "this is a semantic error";
// Note: LogType::DEBUG will prepend __FILE__ and __LINE__ to the debug message

// clang-format off
#define LOGSTREAM_COMMON(...) bpftrace::LogStream(__FILE__, __LINE__, __VA_ARGS__)
#define LOGSTREAM_DEBUG(...) LOGSTREAM_COMMON(__VA_ARGS__)
#define LOGSTREAM_V1(...) LOGSTREAM_COMMON(__VA_ARGS__)
#define LOGSTREAM_HINT(...) LOGSTREAM_COMMON(__VA_ARGS__)
#define LOGSTREAM_WARNING(...) LOGSTREAM_COMMON(__VA_ARGS__)
#define LOGSTREAM_ERROR(...) LOGSTREAM_COMMON(__VA_ARGS__)
#define LOGSTREAM_BUG(...) bpftrace::LogStreamBug(__FILE__, __LINE__, __VA_ARGS__)
// clang-format on

#define LOG(type, ...) LOGSTREAM_##type(bpftrace::LogType::type, ##__VA_ARGS__)

#define DISABLE_LOG(type) bpftrace::Log::get().disable(LogType::type)
#define ENABLE_LOG(type) bpftrace::Log::get().enable(LogType::type)

}; // namespace bpftrace
