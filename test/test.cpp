#include <microlog.h>


int main() {
    microlog::set_log_level(microlog::log_level::info);
    microlog::log_info("hello {}!", 42);
    microlog::log_warn("warning {}!", 43);
    return 0;
}