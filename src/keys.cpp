#include <iostream>
#include <vector>
#include <string_view>
#include <jw/io/keyboard.h>
#include <jw/thread/thread.h>
#include <jw/chrono/chrono.h>
#include <jw/video/ansi.h>

int jwdpmi_main(const std::vector<std::string_view>&)
{
    using namespace jw;
    using namespace jw::io;
    using namespace jw::video::ansi;
    using namespace std::literals;
    using clock = jw::chrono::pit;
    chrono::setup::setup_pit(true);

    bool pressed_esc { false };
    bool use_ansi = true;// { install_check() };
    clock::time_point last_input_time;

    keyboard kb { };
    auto* ps2 = jw::io::ps2_interface::instance().get();

    auto print_scancode_set = [&ps2]
    {
        std::cout << "Keyboard reports scancode set " << static_cast<unsigned>(ps2->current_scancode_set) << ".\n";
    };

    callback key_event { [&](key k, key_state state)
    {
        last_input_time = clock::now();
        switch (state)
        {
        case key_state::up:     std::cout << "- "; break;
        case key_state::down:   std::cout << "+ "; break;
        case key_state::repeat: std::cout << "* "; break;
        }
        if (use_ansi)
        {
            if ((k & 0xff00) != 0x0200)
            {
                switch (state)
                {
                case key_state::up:     std::cout << fg(black) + bold(true);  break;
                case key_state::down:   std::cout << fg(white) + bold(true);  break;
                case key_state::repeat: std::cout << fg(white) + bold(false); break;
                }
            }
            else
            {
                if (state.is_up())   std::cout << fg(yellow) + bold(false);
                if (state.is_down()) std::cout << fg(yellow) + bold(true);
            }
        }

        std::cout << k.name();
        if (use_ansi) std::cout << '\r' << cursor_right(20) + reset();
        else std::cout << '\t';
        std::cout << "key code: 0x" << std::hex << std::setfill('0') << std::setw(4) << k;
        char c = k.to_ascii(kb);
        std::cout << "\t ascii: ";
        std::cout << "0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned>(c) << ' ';
        if      (c == '\n') std::cout << "\'\\n\'";
        else if (c == '\b') std::cout << "\'\\b\'";
        else                std::cout << "\'" << c << "\'";
        std::cout << std::endl;
        int set = 0;
        if (state.is_up())
        {
            if (k == key::esc) pressed_esc = true;
            if (k == key::f1) set = 1;
            if (k == key::f2) set = 2;
            if (k == key::f3) set = 3;
        }
        if (set != 0)
        {
            std::cout << "Selecting scancode set " << set << ". ";
            ps2->set_scancode_set(set);
            print_scancode_set();
        }
        return true;
    } };
    kb.key_changed += key_event;
    kb.auto_update(true);

    last_input_time = clock::now();
    std::cout << "Event handler registered.\n";
    print_scancode_set();
    std::cout << "Use F1 / F2 / F3 to select scancode set.\n";
    std::cout << "Press ESC for the next test, or wait 10 seconds to terminate." << std::endl;
    thread::yield_while([&] { return not pressed_esc and clock::now() < last_input_time + 10s; });
    if (not pressed_esc)
    {
        std::cerr << "Timeout.";
        return 1;
    }

    kb.key_changed -= key_event;
    kb.redirect_cin();
    std::cin.exceptions(std::ios::badbit | std::ios::failbit | std::ios::eofbit);

    thread::task t { [&]
    {
        std::string input { };
        do
        {
            std::cout << "> ";
            std::cin >> input;
            last_input_time = clock::now();
            std::cout << "< \"";
            for (unsigned char c : input)
            {
                if (c != '\b') std::cout << c;
                if (c < 0x20 or c > 0x7f)
                {
                    if (use_ansi) std::cout << fg(black) + bold(true);
                    std::cout << "[" << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned>(c) << ']';
                    if (use_ansi) std::cout << reset();
                }
            }
            std::cout << '\"' << std::endl;
        } while (input != "exit");
    } };

    std::cout << "Testing std::cin, type \"exit\" to end, or wait 30 seconds to terminate." << std::endl;
    last_input_time = clock::now();
    t->start();
    thread::yield_while([&] { return t->is_running() and clock::now() < last_input_time + 30s; });
    if (t->is_running())
    {
        std::cerr << "Timeout.";
        return 1;
    }
    t->await();

    return 0;
}
