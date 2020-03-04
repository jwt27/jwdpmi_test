#include <iostream>
#include <vector>
#include <string_view>
#include <jw/dpmi/ring0.h>
#include <jw/dpmi/cpuid.h>

using namespace jw;

int jwdpmi_main(const std::vector<std::string_view>&)
{
    std::cout << "cpu: " << jw::dpmi::cpuid::vendor() << '\n';
    std::cout << "features: 0x" << std::hex << jw::dpmi::cpuid::leaf(1).edx << '\n';

    std::cout << "cs = 0x" << std::hex << dpmi::get_cs() << '\n';
    std::uint64_t tsc;
    {
        dpmi::ring0_privilege ring0 { };
        asm("rdmsr" : "=A" (tsc) : "c"(0x10));
    }
    std::cout << tsc << std::endl;

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

    std::cin.get();

    std::uint32_t cr3;
    {
        dpmi::ring0_privilege ring0 { };
        asm volatile ("mov %0, cr3" : "=r" (cr3));
    }
    std::cout << "cr3 = 0x" << std::hex << cr3 << '\n';

    /*

    std::cin.get();

    {
        std::uint32_t scratch;
        dpmi::ring0_privilege ring0 { };
        asm volatile ("int 0x31" : "=a" (scratch) : "a" (0x0902) : "cc");
    }

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
