﻿#include <iostream>
#include <span>
#include <string_view>
#include <set>
#include <jw/io/gameport.h>
#include <jw/thread.h>
#include <jw/dpmi/memory.h>
#include <jw/io/keyboard.h>
#include <jw/vector.h>
#include <jw/grid.h>
#include <jw/video/pixel.h>
#include <jw/video/ansi.h>

using namespace jw;

void game()
{
    namespace color = jw::video::bios_colors;
    using namespace std::chrono_literals;
    chrono::setup::setup_pit(true, 0x8000);
    chrono::setup::setup_tsc(0x4000);
    using clock = jw::chrono::tsc;

    std::cout << "synchronizing timer...\n";
    this_thread::yield_for(2s);

    dpmi::mapped_dos_memory<video::text_char> screen_ptr { 80 * 50, dpmi::far_ptr16 { 0xB800, 0 } };
    grid<video::text_char> screen { 80, 50, screen_ptr.get_ptr() };

    screen.fill(video::text_char { ' ', color::white, color::black });

    io::keyboard keyb { };
    std::optional<io::gameport> joystick;

    std::cout << video::ansi::set_cursor({ 0, 20 });
    std::cout << "    Calibrate joystick, press fire when done." << std::endl;
    std::cout << "    Press DEL to restart calibration, ESC to disable joystick." << std::endl;

    struct no_joystick_please { };
    try
    {
    retry_calibration:
        io::gameport::config gameport_cfg { };
        gameport_cfg.enable.z = false;
        gameport_cfg.enable.w = false;

        {
            io::gameport joystick { gameport_cfg };
            std::swap(gameport_cfg.calibration.max, gameport_cfg.calibration.min);
            vector2i pos { 40,25 };
            auto saved = screen.at(pos);
            do
            {
                auto raw = joystick.get_raw();
                vector4f value;
                for (auto i = 0; i < 4; ++i)
                {
                    auto& c = gameport_cfg.calibration;
                    c.min[i] = std::min(c.min[i], raw[i]);
                    c.max[i] = std::max(c.max[i], raw[i]);
                    value[i] = raw[i].count();
                    value[i] /= c.max[i].count() - c.min[i].count();
                }
                screen.at(pos) = saved;
                pos = { value[0] * 79, value[1] * 49 };
                pos.clamp({ 0,0 }, { 79,49 });
                saved = screen.at(pos);
                screen.at(pos) = 'X';
                keyb.update();
                if (keyb[io::key::del]) goto retry_calibration;
                if (keyb[io::key::esc]) throw no_joystick_please { };
            } while (joystick.buttons().none());
        }

        gameport_cfg.strategy = io::gameport::poll_strategy::busy_loop;
        joystick.emplace(gameport_cfg);

        std::cout <<
            " x0_min=" << gameport_cfg.calibration.min[0].count() <<
            " y0_min=" << gameport_cfg.calibration.min[1].count() <<
            " x0_max=" << gameport_cfg.calibration.max[0].count() <<
            " y0_max=" << gameport_cfg.calibration.max[1].count() << '\n';

        do { joystick->get_raw(); } while (joystick->buttons().any());
    }
    catch (const no_joystick_please&) { }

    do { keyb.update(); } while (keyb[io::key::esc]);

    screen.fill(video::text_char { ' ', color::white, color::blue });
    auto r = screen.range({ 5,5 }, { 70,40 });
    r.fill(video::text_char { ' ', color::black, color::green });

    fixed_grid<clock::time_point, 80, 50> mow_time;
    auto compare = [&mow_time](const vector2i& a, const vector2i& b) { return mow_time.nowrap(a) < mow_time.nowrap(b); };
    std::set<vector2i, decltype(compare)> mowed { compare };

    bool collision = false;
    bool friction = true;
    vector2f delta { 0,0 };
    vector2f player { 20,20 };
    vector2f last_player { player };

    callback key_event { [&](io::key k, io::key_state state)
    {
        if (k == io::key::c and state.is_down())
        {
            collision ^= true;
            auto fill = collision ? video::text_char { ' ', color::white, color::red } : video::text_char { ' ', color::white, color::blue };
            screen.range({  0,  0 }, { 80,  5 }).fill_nowrap(fill);
            screen.range({  0,  5 }, {  5, 45 }).fill_nowrap(fill);
            screen.range({ 75,  5 }, {  5, 45 }).fill_nowrap(fill);
            screen.range({  5, 45 }, { 75,  5 }).fill_nowrap(fill);
            player.wrap({ 0,0 }, r.size());
        }
        if (k == io::key::f and state.is_down()) friction ^= true;
        return true;
    } };
    keyb.key_changed += key_event;

    auto last_now = clock::now();
    while (true)
    {
        auto now = clock::now();
        if (now <= last_now)
        {
            std::clog << "now=" << now.time_since_epoch().count()
                      << ", last=" << last_now.time_since_epoch().count() << '\n';
            continue;
        }
        float dt = (now - last_now).count() / 1e9f;
        last_now = now;

        using namespace io;
        auto joy = joystick ? joystick->get() : vector4f { };
        vector2f new_delta { joy.x(), joy.y() };
        keyb.update();
        if (keyb[key::up])    new_delta += vector2f::up();
        if (keyb[key::down])  new_delta += vector2f::down();
        if (keyb[key::left])  new_delta += vector2f::left();
        if (keyb[key::right]) new_delta += vector2f::right();
        if (keyb[key::space]) r.fill_nowrap(video::text_char { ' ', 0, 2 });
        if (keyb[key::esc]) break;

        new_delta.clamp_magnitude(1);
        delta += new_delta * dt * 20;

        r.nowrap(last_player) = video::text_char { ' ', color::black, color::brown };

        for (auto i = mowed.begin(); i != mowed.end() and mow_time.nowrap(*i) <= now; i = mowed.erase(i))
            r.nowrap(*i) = video::text_char { ' ', color::black, color::green };

        player += delta * dt * 10;
        if (collision)
        {
            if (player.x() <= 0 or player.x() >= r.width() - 1) delta.x() *= -1;
            if (player.y() <= 0 or player.y() >= r.height() - 1) delta.y() *= -1;
            player.clamp(vector2i { 0,0 }, r.size());
        }
        player.wrap({ 0,0 }, r.size());
        r.nowrap(player) = video::text_char { 2, color::white, color::brown };
        last_player = player;
        if (friction) delta -= delta * dt * 2;

        mowed.erase(player);
        mow_time.nowrap(player) = now + 10s;
        mowed.emplace_hint(mowed.cend(), player);

        using namespace jw::video::ansi;
        std::cout << set_cursor({ 0, 0 });
        std::cout << "FPS: " << clear_line() << 1 / dt << set_cursor({ 15, 0 }) << "delta=" << delta << set_cursor({ 50, 0 }) << "pos=" << player << "\n";
        if (joystick) std::cout << "buttons=" << joystick->buttons() << set_cursor({ 30, 1 }) << "joy=" << clear_line() << joy << std::flush;

        this_thread::yield();
    }
}

int jwdpmi_main(std::span<std::string_view>)
{
    using namespace jw::video::ansi;

    if (not install_check())
    {
        std::cerr << "No ANSI driver detected.\n";
        return 1;
    }
    std::cout << set_80x50_mode();

    game();

    std::cout << reset() + set_80x50_mode();
    return 0;
}
