#include <format>
#include <source_location>
#include <iostream>

template <typename T>
struct with_source_location {
private:
    T inner;
    std::source_location loc;

public:
    template<class U> requires std::constructible_from<T, U>
    consteval explicit with_source_location(U &&inner_, std::source_location loc_ = std::source_location::current())
    : inner(std::forward<U>(inner_)), loc(loc_) {}

    [[nodiscard]]
    constexpr T const &format() const { return inner; }

    [[nodiscard]]
    constexpr std::source_location const &location() const { return loc; }
};

template <typename... Args>
void log_info(with_source_location<std::format_string<Args...>> fmt, Args &&...args) {
    auto const &loc = fmt.location();
    std::cout << loc.file_name() << ':' << loc.line() << "[Info] " <<
    std::vformat(fmt.format().get(), std::make_format_args(args...)) << '\n';
}

int main() {
    std::format("Hello, {:d}!", 42);
    return 0;
}