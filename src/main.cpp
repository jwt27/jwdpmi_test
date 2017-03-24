#include <iostream>
#include <deque>
#include <string>
#include <jw/dpmi/irq.h>
#include <jw/io/ioport.h>
#include <jw/io/rs232.h>
#include <jw/dpmi/cpu_exception.h>
#include <jw/dpmi/debug.h>
#include <jw/thread/task.h>
#include <jw/thread/coroutine.h>
#include <jw/io/keyboard.h>
#include <jw/io/ps2_interface.h>
#include <jw/alloc.h>
#include <cstdio>
#include <cstddef>

using namespace jw;


int jwdpmi_main(std::deque<std::string>)
{
    std::cout << "Hello, World!" << std::endl;
    dpmi::breakpoint();

    dpmi::memory<int> mem { 4 };

    for (int i = 0; i < 4; ++i) mem[i] = i;
    for (int i = 0; i < 4; ++i) std::cout << mem[i] << ' ';
    std::cout << '\n';

    mem.resize(10);

    for (int i = 0; i < 10; ++i) mem[i] = 10 - i;
    for (int i = 0; i < 10; ++i) std::cout << mem[i] << ' ';
    std::cout << '\n';


    return 0;
    /*
    {
        dpmi::trap_mask no_trace { };
        std::cout << "not tracing here...\n";
        //dpmi::breakpoint();
    }
    std::cout << "trace enabled again!\n";
    try
    {
        dpmi::breakpoint();
        {
            dpmi::trap_mask no_trace { };
            std::cout << "testing...\n";
            //std::cout << 1 / 0;
            std::cout << "nothing happened?\n";
        }
    }
    catch (...) { }

    std::cout << "trace enabled again!\n";*/

    
    auto test_thread = []()
    {
        while (true)
        {
            std::cout << "hello!\n";
            dpmi::breakpoint();
            thread::yield();
        }
    };

    thread::task<void()> thr1 { test_thread };
    thread::task<void()> thr2 { test_thread };

    thr1->start();
    thr2->start();
    thr1->await();
    thr2->await();

    std::string input { };
     

    /*
    dpmi::exception_handler exc0d { 0x0d, [](auto*, auto* frame, bool)
    {
        std::cerr << "EXC 0D at " << std::hex << frame->fault_address.segment << ':' << frame->fault_address.offset << '\n';
        std::cerr << "stack  at " << std::hex << frame->stack.segment << ':' << frame->stack.offset << '\n';
        std::string input;
        std::cin >> input;
        //frame->flags.trap = true;
        return false;
    } };
    dpmi::exception_handler exc0e { 0x0e, [](auto* frame, bool, auto*)
    {
        std::cerr << "EXC 0E at " << std::hex << frame->fault_address.segment << ':' << frame->fault_address.offset << '\n';
        std::cerr << "stack  at " << std::hex << frame->stack.segment << ':' << frame->stack.offset << '\n';
        std::string input;
        std::cin >> input;
        //frame->flags.trap = true;
        return false;
    } };*/
    /*
    struct alignas(0x10) aligned_t
    {
        int x;
    };

    for (int i = 0; i < 10; ++i)
    {
        auto p = new aligned_t { };
        new byte { };
        std::clog << std::hex << (unsigned)p << "\n";
    } */

    io::keyboard keyb { std::make_shared<io::ps2_interface>() };

    callback<void(io::key_state_pair)> kb_event { [&keyb](auto k)
    {
        if (k.second.is_down())
            std::cout << "You pressed ";
        else
            std::cout << "You released ";

        std::cout << k.first.name();
        std::cout << " (ascii: " << k.first.to_ascii(keyb) << ")\n";
        dpmi::breakpoint();
    } };

    keyb.key_changed += kb_event;
    keyb.auto_update(true);
    //keyb.redirect_cin();

    while (true) { thread::yield(); }
    return 0;

    io::rs232_config cfg { };
    cfg.set_com_port(io::com2);
    //cfg.flow_control = io::rs232_config::rts_cts;
    io::rs232_stream s { cfg };
    s << "hello world!\r\n" << std::flush;


    auto chat = [](auto& in, auto& out)
    {
        std::string input { };
        while (in.good())
        {
            std::getline(in, input);
            out << "you said: " << input << "\r\n" << std::flush;
            if (input.substr(0, 4) == "quit") break;
            thread::yield();
        }
    };
    thread::task<void(std::istream& in, std::ostream& out)> t1 { chat };
    thread::task<void(std::istream& in, std::ostream& out)> t2 { chat };

    t1->start(s, std::cout);
    t2->start(std::cin, s);
    t1->await();
    t2->await();

    return 0;
}
