#include <iostream>
#include <deque>
#include <string_view>
#include <jw/io/keyboard.h>
#include <jw/thread/thread.h>

int jwdpmi_main(std::deque<std::string_view>)
{
    using namespace jw;
    using namespace jw::io;
    volatile bool running { true };
    keyboard kb { };
    callback key_event { [&](key_state_pair k)
    {
        if (k.first == key::esc and k.second.is_up()) running = false;
        switch (k.second)
        {
        case key_state::up:     std::cout << "- "; break;
        case key_state::down:   std::cout << "+ "; break;
        case key_state::repeat: std::cout << "* "; break;
        }
        std::cout << k.first.name();
        std::cout << "\t key code: 0x" << std::hex << std::setfill('0') << std::setw(4) << k.first;
        std::cout << "\t ascii: \'" << k.first.to_ascii(kb) << "\'" << std::endl;
        return true;
    } };
    kb.key_changed += key_event;
    kb.auto_update(true);

    thread::yield_while([&] { return running; });
    return 0;
}
