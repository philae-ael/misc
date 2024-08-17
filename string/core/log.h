#ifndef INCLUDE_CORE_LOG_H_
#define INCLUDE_CORE_LOG_H_

#include "fwd.h"
#include "string.h"

#include <cstdarg>
#include <source_location>

#define LOG_DEFAULT_GLOBAL_LEVEL core::LogLevel::Trace

namespace core {
enum class LogLevel {
  Trace,
  Debug,
  Info,
  Warning,
  Error,
};

template <> Maybe<LogLevel> from_hstr8(hstr8 s);

str8 to_str8(LogLevel level);

struct log_entry {
  LogLevel level;
  string_builder builder;
  std::source_location loc;
};

using log_formatter = str8 (*)(void *, Arena &, log_entry &);
void log_register_global_formatter(log_formatter, void *user);

using log_writer = void (*)(void *, str8);

void log_register_global_writer(log_writer, void *user);
void log_emit(Arena &, log_entry &);
bool log_filter(LogLevel level);
void log_set_global_level(LogLevel level);

struct log_builder {
  ArenaScratch arena;
  log_entry entry;

  bool collect_backtrace = false;

  template <class T> log_builder &push(T &&t) {
    entry.builder.push(*arena, FWD(t));
    return *this;
  }

  log_builder &pushf(const char *fmt, ...);
  log_builder &vpushf(const char *fmt, va_list ap);
  log_builder &push_str8(str8 msg);

  log_builder with_stacktrace();

  void emit();

  log_builder(LogLevel level,
              std::source_location loc = std::source_location::current());
};

core::str8 log_fancy_formatter(void *, core::Arena &arena,
                               core::log_entry &entry);

#define LOG_BUILDER(level, instr)                                              \
  (log_filter(level) ? core::log_builder(level).instr.emit() : (void)0)

#define LOG(level, fmt, ...)                                                   \
  LOG_BUILDER(level, pushf(fmt __VA_OPT__(, __VA_ARGS__)))

#define LOG_DEBUG(fmt, ...)                                                    \
  LOG(core::LogLevel::Debug, fmt __VA_OPT__(, __VA_ARGS__))

#define LOG_INFO(fmt, ...)                                                     \
  LOG(core::LogLevel::Info, fmt __VA_OPT__(, __VA_ARGS__))

#define LOG_TRACE(fmt, ...)                                                    \
  LOG(core::LogLevel::Trace, fmt __VA_OPT__(, __VA_ARGS__))

#define LOG_WARNING(fmt, ...)                                                  \
  LOG(core::LogLevel::Warning, fmt __VA_OPT__(, __VA_ARGS__))

#define LOG_ERROR(fmt, ...)                                                    \
  LOG(core::LogLevel::Error, fmt __VA_OPT__(, __VA_ARGS__))
} // namespace core

#endif // INCLUDE_CORE_LOG_H_
