#pragma once
#include <crt0.h>
#include <jw/common.h>

namespace jw
{
    namespace chrono
    {
        struct pit;
        struct tsc;
        struct rtc;
    }
    namespace config
    {
        // Additional startup flags for the djgpp runtime library.
        // See http://www.delorie.com/djgpp/doc/libc/libc_124.html
        constexpr int user_crt0_startup_flags = 0;

        // Initial stack size for IRQ handlers.
        constexpr std::size_t interrupt_initial_stack_size = 1_MB;

        // Minimum stack size for IRQ handlers. Tries to resize when the stack size drops below this amount.
        constexpr std::size_t interrupt_minimum_stack_size = 64_KB;

        // Total memory allocated to store fpu contexts.
        constexpr std::size_t interrupt_fpu_context_pool = 128_KB;

        // Initial memory pool for operator new() in interrupt context.
        constexpr std::size_t interrupt_initial_memory_pool = 1_MB;

        // Total stack size for exception handlers. Remote debugging requires a lot of stack space.
        constexpr std::size_t exception_stack_size = 1_MB;

        // Default stack size for threads.
        constexpr std::size_t thread_default_stack_size = 64_KB;

        // Set up cpu exception handlers to throw C++ exceptions instead.
        constexpr bool enable_throwing_from_cpu_exceptions = true;

        // Enable interrupts while the gdb interface is active
        constexpr bool enable_gdb_interrupts = true;

        // Enable debug messages from the gdb interface
        constexpr bool enable_gdb_debug_messages = false;

        // Display raw packet data from serial gdb interface
        constexpr bool enable_gdb_protocol_dump = false;

        // Clock used for gameport timing.
        using gameport_clock = jw::chrono::tsc;

        // Clock used for midi timestamps.
        using midi_clock = jw::chrono::tsc;

        // Default clock used by thread::yield_for() and thread::yield_while_for().
        using thread_clock = jw::chrono::pit;
    }
}
