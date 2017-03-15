#include <iostream>
#include <deque>
#include <string>
#include <jw/dpmi/irq.h>
#include <jw/io/ioport.h>
#include <jw/io/rs232.h>
#include <jw/dpmi/cpu_exception.h>
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

    std::string input;
     
    dpmi::exception_handler exc0d { 0x0d, [](auto*, auto* frame, bool)
    {
        std::cerr << "EXC 0D at " << std::hex << frame->fault_address.segment << ':' << frame->fault_address.offset << '\n';
        std::cerr << "stack  at " << std::hex << frame->stack.segment << ':' << frame->stack.offset << '\n';
        std::string input;
        std::cin >> input;
        //frame->flags.trap = true;
        return false;
    } };
    /*
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

    thread::coroutine<char()> asdf { [](auto& self)
    {
        std::string hello { "Hello, World!" };
        for (auto c : hello)
        {
            self.yield(c);
        }
    } };

    asdf->start();

    try
    {
        while (asdf->try_await())
        {
            std::cout << asdf->await();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "exception happened! " << e.what() << '\n';
    }
           /*
    {
        jw::thread::task<void()> orphan { [&]()
        {
            try { while(true) thread::yield(); }
            catch (const std::exception& e) { std::cerr << e.what() << std::endl; }
            throw std::runtime_error("test");
        } };
        orphan->start();
    }    */

    jw::thread::task<void(std::string)> hello { [&](auto s)
    {
        for (auto c : s)
        {
            std::cout << c;
            thread::yield();
        }
        //throw std::runtime_error("test");
    } };
    hello->start("asdfghjkl");

    hello->await();

    io::keyboard keyb { std::make_shared<io::ps2_interface>() };

    callback<void(io::key_state_pair)> kb_event {[&keyb](auto k)
    { 
        if (k.second.is_down())
            std::cout << "You pressed ";
        else
            std::cout << "You released ";

        std::cout << k.first.name();
        std::cout << " (ascii: " << k.first.to_ascii(keyb) << ")\n";
    }};

    //keyb.key_changed += kb_event;
    keyb.auto_update(true);
    keyb.redirect_cin();

    dpmi::exception_handler exc03 { 3, [](auto* reg, auto*, bool)
    {
        std::cerr << "trap!\n" << *reg;

        reg->eax = 0xffffffff;
        reg->ebx = 0xeeeeeeee;
        reg->ecx = 0xdddddddd;
        reg->edx = 0xcccccccc;
        reg->esi = 0xbbbbbbbb;
        reg->edi = 0xaaaaaaaa;
        return true;
    } };

    dpmi::cpu_registers reg { };
    reg.eax = 0x12345678;
    reg.ebx = 0x11111111;
    reg.ecx = 0x22222222;
    reg.edx = 0x33333333;
    reg.esi = 0x44444444;
    reg.edi = 0x55555555;

    std::cout << "pre-trap:\n"<< reg;

    asm("int 3;"
        : "+a" (reg.eax)
        , "+b" (reg.ebx)
        , "+c" (reg.ecx)
        , "+d" (reg.edx)
        , "+S" (reg.esi)
        , "+D" (reg.edi));

    std::cout << "post-trap:\n"<< reg;
    
    io::rs232_config cfg { };
    cfg.set_com_port(io::com1);
    //cfg.flow_control = io::rs232_config::rts_cts;
    io::rs232_stream s { cfg };
    s << "hello world!\r\n" << std::flush;
    /*
    while (t0->pending_exceptions()) try
    {
        t0->try_await();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what();
    } */
    while (s.good())
    {
        std::getline(s, input);
        s << "you said: " << input << "\r\n"; // << std::flush;
        std::cout << "you said: " << input << std::endl;
        if (input.substr(0, 3) == "quit") break;
    }

    return 0;
}
