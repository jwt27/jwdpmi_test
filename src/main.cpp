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
#include <cstdio>

using namespace jw;

dpmi::far_ptr32 [[gnu::noinline]] get_ptr() { return dpmi::far_ptr32 { 0x1234, 0x12345678 }; }

int jwdpmi_main(std::deque<std::string>)
{
    std::cout << "Hello, World!" << std::endl;
/*
    volatile int count = 0;
    jw::dpmi::irq_handler k { [&](auto ack) INTERRUPT { --count; ack(); }, jw::dpmi::always_chain };
    jw::dpmi::irq_handler t { [&](auto ack) INTERRUPT { ++count; ack(); }, jw::dpmi::always_chain | jw::dpmi::no_reentry };
    k.set_irq(1);
    k.enable();
    t.set_irq(0);
    t.enable();
    jw::io::out_port<byte>{0x43}.write(0x34);
    jw::io::out_port<byte>{0x40}.write(0x00);
    jw::io::out_port<byte>{0x40}.write(0x01);
    int tmp = 0;
    while (std::cin.good())
    {
        if (tmp != count)
        {
            if (tmp < count) std::cout << '!' << std::flush;
            else std::cout << '.' << std::flush;
            tmp = count;
        }
    }*/
     /*
    dpmi::exception_handler exc0d { 0x0d, [](auto* frame, bool)
    {
        std::cerr << "EXC 0D at " << std::hex << frame->fault_address.segment << ':' << frame->fault_address.offset << '\n';
        std::cerr << "stack  at " << std::hex << frame->stack.segment << ':' << frame->stack.offset << '\n';
        std::string input;
        std::cin >> input;
        //frame->flags.trap = true;
        return false;
    } };

    dpmi::exception_handler exc0e { 0x0e, [](auto* frame, bool)
    {
        std::cerr << "EXC 0E at " << std::hex << frame->fault_address.segment << ':' << frame->fault_address.offset << '\n';
        std::cerr << "stack  at " << std::hex << frame->stack.segment << ':' << frame->stack.offset << '\n';
        std::string input;
        std::cin >> input;
        //frame->flags.trap = true;
        return false;
    } };*/

    thread::coroutine<char()> asdf { [](auto& self)
    {
        std::string hello { "Hello, World!" };
        for (auto c : hello)
        {
            self.yield(c);
            throw std::runtime_error("asdf");
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

    {
        jw::thread::task<void()> orphan { [&]()
        {
            try { while(true) thread::yield(); }
            catch (const std::exception& e) { std::cerr << e.what() << std::endl; }
        } };
        orphan->start();
    }

    thread::yield();

    io::keyboard keyb { std::make_shared<io::ps2_interface>() };

    callback<void(io::keyboard&, io::key_state_pair)> kb_event {[](auto&, auto k) 
    { 
        std::cout << k.first.name() << '=' << k.second << '\n'; 
    }};

    keyb.key_changed += kb_event;
    keyb.auto_update(true);

    dpmi::exception_handler exc03 { 3, [](auto*, bool)
    {
        volatile long double x = 1.0;
        volatile long double y = 0.5;
        std::cerr << x + y << '\n';
        //std::cerr << "!";
        //frame->flags.trap = true;
        return true;
    } };

    std::string input;

    asm("int 3;");
    asm("int 3;");
    asm("int 3;");
    
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
        if (input.substr(0, 3) == "kys") break;
    }

    return 0;
}
