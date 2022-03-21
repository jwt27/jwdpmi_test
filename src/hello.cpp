#include <span>
#include <string_view>
#include <fmt/core.h>

int jwdpmi_main(std::span<std::string_view>)
{
    fmt::print("Hello, World!");
    return 0;
}
