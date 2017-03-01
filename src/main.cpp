#include <iostream>
#include <deque>
#include <string>
#include <jw/dpmi/irq.h>
#include <jw/io/ioport.h>
#include <cstdio>

volatile int count { 0 };

void f(auto ack)
{ asm volatile ("irqhandler2_%=:":::); ++count; ack(); }

int jwdpmi_main(std::deque<std::string> args)
{
    std::cout << "Hello, World!" << std::endl;
    jw::dpmi::irq_handler h { [&](auto ack) [[gnu::used]] { asm volatile ("irqhandler%=:":::); ++count; ack(); }, jw::dpmi::irq_config_flags { } };
    h.set_irq(1);
    h.enable();
    std::cout << "enabling..." << std::endl;
    jw::io::out_port<byte>{0x43}.write(0x34);
    jw::io::out_port<byte>{0x40}.write(0xFF);
    jw::io::out_port<byte>{0x40}.write(0xFF);
    int tmp = 0;
    std::cout << "go!" << std::endl;
    while (std::cin.good())
    {
        asm volatile("":::"memory");
        if (tmp != count)
        {
            std::cout << '!';
            tmp = count;
        }
    }
    std::cout << "end." << std::endl;
}
