#include "log.h"

namespace bpftrace {

std::string logtype_str(LogType t)
{
  switch (t) {
      // clang-format off
    case LogType::DEBUG   : return "";
    case LogType::V1      : return "";
    case LogType::HINT    : return "HINT: ";
    case LogType::WARNING : return "WARNING: ";
    case LogType::ERROR   : return "ERROR: ";
    case LogType::BUG     : return "BUG: ";
      // clang-format on
  }

  return {}; // unreached
}

Log::Log()
{
  enabled_map_[LogType::DEBUG] = true;
  enabled_map_[LogType::V1] = false;
  enabled_map_[LogType::HINT] = true;
  enabled_map_[LogType::WARNING] = true;
  enabled_map_[LogType::ERROR] = true;
  enabled_map_[LogType::BUG] = true;
}

Log& Log::get()
{
  static Log log;
  return log;
}

void Log::take_input(LogType type,
                     std::optional<ast::Location> loc,
                     std::ostream& out,
                     std::string&& input)
{
  if (loc) {
    if (src_.empty()) {
      std::cerr << "Log: cannot resolve location before calling set_source()."
                << std::endl;
      out << log_format_output(type, std::move(input));
    } else if (loc->line_range_.first == 0 || loc->line_range_.second == 0) {
      std::cerr << "Log: invalid location." << std::endl;
      out << log_format_output(type, std::move(input));
    } else if (loc->line_range_.first > loc->line_range_.second) {
      std::cerr << "Log: loc.begin > loc.end: " << loc->line_range_.first << ":"
                << loc->line_range_.second << std::endl;
      out << log_format_output(type, std::move(input));
    } else {
      log_with_location(type, *loc, out, input);
    }
  } else {
    out << log_format_output(type, std::move(input));
  }
}

std::string Log::log_format_output(LogType type, std::string&& input)
{
  if (!is_colorize_) {
    return logtype_str(type) + std::move(input) + '\n';
  }
  std::string color;
  switch (type) {
    case LogType::ERROR:
      color = LogColor::RED;
      break;
    case LogType::WARNING:
      color = LogColor::YELLOW;
      break;
    default:
      return logtype_str(type) + std::move(input) + '\n';
  }
  return color + logtype_str(type) + std::move(input) + LogColor::RESET + '\n';
}

const std::string Log::get_source_line(unsigned int n)
{
  // Get the Nth source line (N is 0-based). Return an empty string if it
  // doesn't exist
  std::string line;
  std::stringstream ss(src_);
  for (unsigned int idx = 0; idx <= n; idx++) {
    std::getline(ss, line);
    if (ss.eof() && idx == n)
      return line;
    if (!ss)
      return "";
  }
  return line;
}

void Log::log_with_location(LogType type,
                            ast::Location l,
                            std::ostream& out,
                            const std::string& m)
{
  const char* color_begin = LogColor::DEFAULT;
  const char* color_end = LogColor::DEFAULT;
  if (is_colorize_) {
    switch (type) {
      case LogType::ERROR:
        color_begin = LogColor::RED;
        color_end = LogColor::RESET;
        break;
      case LogType::WARNING:
        color_begin = LogColor::YELLOW;
        color_end = LogColor::RESET;
        break;
      default:
        break;
    }
  }
  out << color_begin;
  if (filename_.size()) {
    out << filename_ << ":";
  }

  std::string msg(m);
  const std::string& typestr = logtype_str(type);

  if (!msg.empty() && msg.back() == '\n') {
    msg.pop_back();
  }

  // For a multi line error only the line range is printed:
  //     <filename>:<start_line>-<end_line>: ERROR: <message>
  if (l.line_range_.first < l.line_range_.second) {
    out << l.line_range_.first << "-" << l.line_range_.second << ": " << typestr
        << msg << std::endl
        << color_end;
    return;
  }

  // For a single line error the format is:
  //
  // <filename>:<line>:<start_col>-<end_col>: ERROR: <message>
  // <source line>
  // <marker>
  //
  // E.g.
  //
  // file.bt:1:10-20: error: <message>
  // i:s:1   /1 < "str"/
  //         ~~~~~~~~~~
  out << l.line_range_.first << ":" << l.column_range_.first << "-"
      << l.column_range_.second;
  out << ": " << typestr << msg << std::endl << color_end;

  // for bpftrace::position, valid line# starts from 1
  std::string srcline = get_source_line(l.line_range_.first - 1);

  if (srcline == "") {
    return;
  }

  // To get consistent printing all tabs will be replaced with 4 spaces
  for (auto c : srcline) {
    if (c == '\t')
      out << "    ";
    else
      out << c;
  }
  out << std::endl;

  for (unsigned int x = 0;
       x < srcline.size() &&
       x < (static_cast<unsigned int>(l.column_range_.second) - 1);
       x++) {
    char marker = (x < (static_cast<unsigned int>(l.column_range_.first) - 1))
                      ? ' '
                      : '~';
    if (srcline[x] == '\t') {
      out << std::string(4, marker);
    } else {
      out << marker;
    }
  }
  out << std::endl;
}

LogStream::LogStream(const std::string& file,
                     int line,
                     LogType type,
                     std::ostream& out)
    : sink_(Log::get()),
      type_(type),
      loc_(std::nullopt),
      out_(out),
      log_file_(file),
      log_line_(line)
{
}

LogStream::LogStream(const std::string& file,
                     int line,
                     LogType type,
                     ast::Location loc,
                     std::ostream& out)
    : sink_(Log::get()),
      type_(type),
      loc_(std::ref(loc)),
      out_(out),
      log_file_(file),
      log_line_(line)
{
}

LogStream::~LogStream()
{
  if (sink_.is_enabled(type_)) {
    auto msg = buf_.str();
    if (type_ == LogType::DEBUG)
      msg = internal_location() + msg;

    sink_.take_input(type_, loc_, out_, std::move(msg));
  }
}

std::string LogStream::internal_location()
{
  std::ostringstream ss;
  ss << "[" << log_file_ << ":" << log_line_ << "] ";
  return ss.str();
}

[[noreturn]] LogStreamBug::~LogStreamBug()
{
  sink_.take_input(type_, loc_, out_, internal_location() + buf_.str());
  abort();
}

}; // namespace bpftrace
