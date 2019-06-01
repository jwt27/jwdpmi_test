#include <iostream>
#include <deque>
#include <string_view>
#include <jw/dpmi/ring0.h>

using namespace jw;

int jwdpmi_main(std::deque<std::string_view>)
{
    std::cout << "cs = 0x" << std::hex << dpmi::get_cs() << '\n';
    std::uint64_t tsc;
    {
        dpmi::ring0_privilege ring0 { };
        asm("rdmsr":"=A" (tsc) : "c"(0x10));
    }
    std::cout << tsc << '\n';

    std::cin.get();

    try
    {
        dpmi::ring0_privilege ring0 { };
        throw std::runtime_error { "unwinding" };
    }
    catch(const std::exception& e)
    {
        print_exception(e);
        std::cout << "cs = 0x" << std::hex << dpmi::get_cs() << '\n';
    }

    /*
    std::cin.get();

    try
    {
        dpmi::ring0_privilege ring0 { };
        volatile int* i = 0;
        *i = 1;
    }
    catch (const std::exception& e)
    {
        print_exception(e);
    }
    */

    return 0;
}
