#pragma once

#include <format>
#include <source_location>
#include <iostream>
#include <cstdint>

namespace microlog {

// x-macro technique
#define MICROLOG_FOREACH_LOG_LEVEL(f) \
    f(trace) \
    f(debug) \
    f(info) \
    f(critical) \
    f(warning) \
    f(error) \
    f(fatal)

    enum class log_level : std::uint8_t {
#define FUNCTION(name) name,
        MICROLOG_FOREACH_LOG_LEVEL(FUNCTION)
#undef FUNCTION
    };

    namespace details {

#if defined(__linux__) || defined(__APPLE__)
        inline char level_ansi_colors[static_cast<std::uint8_t>(log_level::fatal) + 1][6] = {
                "37m",
                "35m",
                "32m",
                "34m",
                "33m",
                "31m",
                "31;1m",
        };

#define MICROLOG_IF_HAS_ANSI_COLOR(x) x
#else
#define MICROLOG_IF_HAS_ANSI_COLOR(x)
#endif

        inline std::string log_level_name(log_level level) {    // TODO: must be inline!!!
            switch (level) {
#define FUNCTION(name) case log_level::name: return #name;
                MICROLOG_FOREACH_LOG_LEVEL(FUNCTION)
#undef FUNCTION
            }
            return "unknown";
        }

        inline log_level log_level_name(std::string_view level) {
#define FUNCTION(name) if (level == #name) return log_level::name;
            MICROLOG_FOREACH_LOG_LEVEL(FUNCTION)
#undef FUNCTION
        return log_level::info;
        }

//static log_level max_level = log_level::trace;
        inline log_level g_max_level = log_level::debug;      // TODO: static or inline? must be inline!!!

        template<typename T>
        struct with_source_location {
        private:
            T inner;
            std::source_location loc;

        public:
            template<class U>
            requires std::constructible_from<T, U>
            consteval with_source_location(U &&inner_, std::source_location loc_ = std::source_location::current())
                    : inner(std::forward<U>(inner_)), loc(loc_) {}

            [[nodiscard]]
            constexpr T const &format() const { return inner; }

            [[nodiscard]]
            constexpr std::source_location const &location() const { return loc; }
        };


        template<typename... Args>
        void generic_log(log_level lev, with_source_location<std::format_string<Args...>> fmt, Args &&...args) {
            if (lev >= g_max_level) {
                auto const &loc = fmt.location();
                std::cout  MICROLOG_IF_HAS_ANSI_COLOR( << "\E[" << level_ansi_colors[static_cast<std::uint8_t>(lev)])
                << loc.file_name() << ':' << loc.line() << "[" << log_level_name(lev) << "] " <<
                          std::vformat(fmt.format().get(), std::make_format_args(args...))
                MICROLOG_IF_HAS_ANSI_COLOR(<< "\E[m") << '\n';
            }
        }

    } // namespace details

    // MICROLOG_LEVEL=trace ./a.out
    inline log_level g_max_level = [] () -> log_level {     // TODO: set max_level to env var
        if (auto env = std::getenv("MICROLOG_LEVEL"); env) {
            return details::log_level_name(env);
        }
        return log_level::info;
    }();

    inline void set_max_level(log_level level) {
        details::g_max_level = level;
    }

#define FUNCTION(name) \
template <typename... Args> \
void log_##name(details::with_source_location<std::format_string<Args...>> fmt, Args &&...args) { \
    details::generic_log(log_level::name, std::move(fmt), std::forward<Args>(args)...); \
}

    MICROLOG_FOREACH_LOG_LEVEL(FUNCTION)

#undef FUNCTION

#define MICROLOG_P(x) ::minilog::log_debug(#x " = {}", x)   // TODO: add ::microlog::

} // namespace microlog
