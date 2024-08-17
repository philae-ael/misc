#include "core/core.h"
#include "core/log.h"
#include "core/platform.h"

using namespace core::literals;

int main(int argc, char *argv[]) {
  core::log_register_global_formatter(core::log_fancy_formatter, nullptr);
  LOG_TRACE("trace %d", 84);
  LOG_DEBUG("debug %d", 84);
  LOG_INFO("info %d", 84);
  LOG_WARNING("warning %d", 84);
  LOG_ERROR("error %d", 84);

  LOG_INFO("setting log_level to info");
  core::log_set_global_level(core::LogLevel::Info);

  LOG_TRACE("trace %d", 84);
  LOG_DEBUG("debug %d", 84);
  LOG_INFO("info %d", 84);
  LOG_BUILDER(core ::LogLevel ::Warning,
              with_stacktrace()
                  .push(from_hstr8<core::Os>("windows"_hs))
                  .pushf(" warning %d", 84));
  LOG_ERROR("error %d", 84);

  return 0;
}
