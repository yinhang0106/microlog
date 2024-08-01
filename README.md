# microlog - 纯头文件日志库

非常感谢小彭老师的精彩教学，这个项目是基于小彭老师的C++课程中的日志库的实现，我在这个基础上进行了一些修改和优化。

如果对小彭老师的教学感兴趣，可以关注小彭老师的B站视频[【自己动手实现纯头文件日志库】](https://www.bilibili.com/video/BV1t94y1r72E?vd_source=1441b5bd793a9efda4bf62153bcca888)

项目开发环境是在`Ubuntu 22.04.3 LTS`、编译器`gcc 13.2.0`

在开发中使用`<format>`库中的`std::format`来定制日志库中的相关字符串生成。`<format>`库是C++20中的标准库，所以需要在编译时加上`-std=c++20`选项，同时编译器也要支持C++20标准。`std::format`的使用方法可以参考[网址](https://zh.cppreference.com/w/cpp/utility/format/format)。

日志库中涉及到对操作文件中行号、函数名、文件名的获取，这在`c++20`后，可以使用标准库`<source_location>`来实现，实现的方法更加简单：
    
```cpp
#include <source_location>
#include <iostream>

int main() {
    std::source_location loc = std::source_location::current();
    
    std::cout << "File: " << loc.file_name() << std::endl;
    std::cout << "Function: " << loc.function_name() << std::endl;
    std::cout << "Line: " << loc.line() << std::endl;
}
```
打印出的结果如下：
```bash
File: /home/ubuntu/dev/microlog/docs/test_doc.cpp
Function: int main()
Line: 5
Column: 61
```

如果我们将`loc`作为参数传给函数，那么返回的文件名等信息是在调用函数的地方，而不是函数声明的地方。这样就可以实现在日志库中获取调用日志的文件名、函数名、行号等信息。

```cpp
#include <source_location>
#include <iostream>

void log(std::string const &msg, std::source_location loc = std::source_location::current()) {
    std::cout << loc.file_name() << ": " << loc.line() << " " << msg << std::endl;
}

int main() {
    log("hello world");
    return 0;
}
```
打印出的结果如下：
```bash
/home/ubuntu/dev/microlog/docs/test_doc.cpp: 9 hello world
```
可以看到，打印的文件名、行号是在`log`函数调用的地方，第9行。

上述`log`函数实际上就是我们日志库的原始实现。下一步的工作是将`log`函数中的参数`msg`改为可以像`std::format`一样的格式化字符串，这样就可以实现定制化的日志输出。

我们考虑`std::format`在`<format>中的基本实现，

```cpp
template<typename... _Args>
[[nodiscard]]
inline string
format(format_string<_Args...> __fmt, _Args&&... __args)
{ return std::vformat(__fmt.get(), std::make_format_args(__args...)); }
```

所以我们可以仿照`std::format`的实现，实现我们的日志库函数`log`。

```cpp
template <typename... Args>
void log_info(std::format_string<Args...> fmt, Args&&... args, std::source_location loc = std::source_location::current()) {
    std::cout << loc.file_name() << ": " << loc.line() << " " << std::vformat(fmt.get(), std::make_wformat_args(args...)) << std::endl;
}
```

但这样是不行的`Args&&... args`必须为函数中的最后一个参数，所以不能再有后边的默认参数。

如何解决这个问题呢？我们将默认参数包装到`fmt`中来实现。

```c++
template <typename T>
struct with_source_location {
private:
    T inner;
    std::source_location loc;
public:
    template<class U> requires std::constructible_from<T, U>
    consteval with_source_location(U &&inner_, std::source_location loc_ = std::source_location::current())
    : inner(std::forward<U>(inner_)), loc(loc_) {}
    constexpr T const &format() const { return inner; }
    constexpr std::source_location const &location() const { return loc; }
};

template <typename... Args>
void log_info(with_source_location<std::format_string<Args...>> fmt, Args&&... args) {
    auto const &loc = fmt.location();
    std::cout << loc.file_name() << ": " << loc.line() << " " << 
        std::vformat(fmt.format().get(), std::make_format_args(args...)) << std::endl;
}
```
注意，这里面的`with_source_location`构造函数必须是`consteval`的，我们需要在编译时就将`inner`初始化为`a constant expression`，这一点是格式化字符串的要求，必须在编译时就确定。

这样的话，在调用`log_info`函数的位置，`with_source_location`的构造函数就会被调用，将`std::source_location::current()`传给`loc`，这样就可以实现在调用`log_info`函数的地方获取文件名、函数名、行号等信息。**这是一种解决`Arg&&... args`后仍有默认参数的方案。**

现在我们使用`x-macro`技术来实现日志库的级别控制。

我们希望利用`enum`来表示日志的级别，后续根据不同的级别来输出不同的日志。我们可以这样定义`enum`：

```cpp
enum class log_level : std::uint8_t {
        trace,
        debug,
        info,
        critical,
        warn,
        error,
        fatal,
    };
```
在利用`level`生成级别名字时，我们需要这样编写函数：
    
```cpp
inline std::string log_level_name(log_level lev) {
    switch (lev) {
        case log_level::trace: return "trace";
        case log_level::debug: return "debug";
        case log_level::info: return "info";
        case log_level::critical: return "critical";
        case log_level::warn: return "warn";
        case log_level::error: return "error";
        case log_level::fatal: return "fatal";
    }
    return "unknown";
}
```
这里出现了大量的重复代码，我们可以使用`x-macro`技术来解决这个问题。

```cpp
#define LOG_LEVELS(X) \
    X(trace) \
    X(debug) \
    X(info) \
    X(critical) \
    X(warn) \
    X(error) \
    X(fatal)

enum class log_level : std::uint8_t {
#define FUNCTION(name) name,
    LOG_LEVELS(FUNCTION)
#undef FUNCTION
        
inline std::string log_level_name(log_level lev) {
    switch (lev) {
#define FUNCTION(name) case log_level::name: return #name;
        LOG_LEVELS(FUNCTION)
#undef FUNCTION
    }
    return "unknown";
}
```
是不是非常的简洁，而且易于维护。这里可以用`x-macro`技术来生成函数名称中带有`log_level`的函数。

```c++
#define _FUNCTION(name) \
template <typename... Args> \
void log_##name(details::with_source_location<std::format_string<Args...>> fmt, Args &&...args) { \
    return generic_log(log_level::name, std::move(fmt), std::forward<Args>(args)...); \
}
    MICROLOG_FOREACH_LOG_LEVEL(_FUNCTION)
#undef _FUNCTION
```

在设置日志库的全局变量时，我们可以通过代码直接设置，也可以通过环境变量来设置。

```cpp
namespace details {

inline log_level g_max_level = [] () -> log_level {
    if (auto lev = std::getenv("MICROLOG_LEVEL")) {
        return details::log_level_from_name(lev);
    }
    return log_level::info;
} ();

inline std::ofstream g_log_file = [] () -> std::ofstream {
    if (auto path = std::getenv("MICROLOG_FILE")) {
        return std::ofstream(path, std::ios::app);
    }
    return {};
} ();

}   // namespace details

inline void set_log_file(const std::string& path) {
    details::g_log_file = std::ofstream(path, std::ios::app);
}

inline void set_log_level(log_level lev) {
    details::g_max_level = lev;
}
```








































