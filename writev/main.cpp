#include <array>
#include <cstring>
#include <span>
#include <stdexcept>
#include <stdlib.h>
#include <string_view>

#include <sys/uio.h>

template <typename... Args> void log(std::string_view fmt, Args... args) {
  constexpr std::size_t ARG_COUNT = sizeof...(args);
  constexpr std::size_t MAX_PARTS = 2 * ARG_COUNT + 1;
  std::array<std::string_view, MAX_PARTS> parts;

  std::array<char, 1024> buffer{};
  std::span<char> buf(buffer);
  std::memset(buf.data(), 0, buf.size());

  std::size_t i = 0;
  std::size_t arg_index = 0;

  std::array<std::string_view, ARG_COUNT> arg_array;
  const auto extract_args = [&](auto arg) -> std::string_view {
    if constexpr (std::is_convertible_v<decltype(arg), std::string_view>) {
      return std::string_view(arg);
    } else if constexpr (std::is_same_v<decltype(arg), double>) {
      int len = snprintf(buf.data(), buf.size(), "%f", double(arg));
      if (len < 0 || static_cast<std::size_t>(len) >= buf.size()) {
        throw std::runtime_error("Argument formatting error");
      }
      const auto result = std::string_view(buf.data(), len);
      buf = buf.subspan(len);
      return result;
    } else if constexpr (std::is_convertible_v<decltype(arg), int>) {
      int len = snprintf(buf.data(), buf.size(), "%d", int(arg));
      if (len < 0 || static_cast<std::size_t>(len) >= buf.size()) {
        throw std::runtime_error("Argument formatting error");
      }
      const auto result = std::string_view(buf.data(), len);
      buf = buf.subspan(len);
      return result;
    } else {
      return "<unsupported type>";
    }
  };
  const auto pack_args = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
    ((arg_array[Is] = extract_args(args)), ...);
  };
  pack_args(std::make_index_sequence<ARG_COUNT>{});

  while (i < MAX_PARTS) {
    auto pos = fmt.find("{}");
    if (pos == std::string_view::npos) {
      parts[i++] = fmt;
      break;
    }
    const auto substr = fmt.substr(0, pos);
    if (!substr.empty())
      parts[i++] = substr;

    fmt.remove_prefix(pos + 2);

    if (arg_index > ARG_COUNT)
      throw std::runtime_error("Too many arguments provided for format string");
    parts[i++] = arg_array[arg_index++];
  }

  std::array<struct iovec, MAX_PARTS> iov;
  for (std::size_t j = 0; j < i; ++j) {
    iov[j].iov_base = const_cast<char *>(parts[j].data());
    iov[j].iov_len = parts[j].size();
  }
  writev(1, iov.data(), i);
  fflush(stdout);
}

int main() {
  log("Hello, {}, doing that is easy {} {} {}!", "world", 5, 7, 3.1415);
}
