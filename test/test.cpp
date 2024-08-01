#include <microlog.h>


int main() {
    microlog::set_max_level(microlog::log_level::info);
    microlog::log_info("hello {}!", 42);
    microlog::log_warning("warning {}!", 43);
    return 0;
}