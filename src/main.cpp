#include <iostream>
#include <deque>
#include <string>
#include <jw/dpmi/irq.h>
#include <jw/io/ioport.h>
#include <jw/io/rs232.h>
#include <cstdio>

using namespace jw;

int jwdpmi_main(std::deque<std::string> args)
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
    io::rs232_config cfg { };
    cfg.set_com_port(io::com1);
    //cfg.flow_control = io::rs232_config::rts_cts;
    io::rs232_stream s { cfg };
    s << "hello world!\r\n" << std::flush;
    std::string input;
    while (s.good())
    {
        std::getline(s, input);
        s << "you said: " << input << "\r\n"; // << std::flush;
        std::cout << "you said: " << input << std::endl;
        if (input.substr(0, 3) == "kys") break;
    }
}
