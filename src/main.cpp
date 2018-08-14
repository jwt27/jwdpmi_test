#include <iostream>
#include <deque>
#include <string>
#include <string_view>
#include <experimental/vector>
#include <jw/dpmi/irq.h>
#include <jw/io/ioport.h>
#include <jw/io/rs232.h>
#include <jw/dpmi/cpu_exception.h>
#include <jw/debug/debug.h>
#include <jw/dpmi/realmode.h>
#include <jw/thread/task.h>
#include <jw/thread/coroutine.h>
#include <jw/io/keyboard.h>
#include <jw/io/ps2_interface.h>
#include <jw/alloc.h>
#include <jw/vector.h>
#include <jw/matrix.h>
#include <jw/io/mpu401.h>
#include <jw/io/pci.h>
#include <jw/video/pixel.h>
#include <jw/video/vbe.h>
#include <jw/math.h>
#include <jw/io/gameport.h>
#include <cstdio>
#include <cstddef>
#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <csignal>

using namespace jw;

void vbe_test()
{
    using namespace std::literals;
    auto v = video::get_vbe_interface();
    auto info = v->get_vbe_info();

    std::cout << std::hex << info.vbe_version << '\n';
    std::cout << info.oem_vendor_name << '\n';
    std::cout << info.oem_product_name << '\n';
    std::cout << info.oem_string << '\n';

    const auto& modelist = v->get_modes();
    const auto& mode = [&modelist]
    {
        for (const auto& mode : modelist)
        {
            auto& m = mode.second;
            if (m.resolution_x == 640 &&
                m.resolution_y == 480 &&
                m.memory_model == video::vbe_mode_info::direct &&
                m.bits_per_pixel == 16 &&
                m.attr.lfb_mode_available &&
                m.linear_num_image_pages >= 2) return mode;
        }
        throw std::runtime_error("mode not found.");
    }();
    video::vbe_mode vmode { };
    vmode.mode = mode.first;
    vmode.use_lfb_mode = true;
    v->set_mode(vmode);

    using pixel = video::px16;
    std::size_t pixel_size = mode.second.bits_per_pixel / 8;
    std::size_t line_size = mode.second.linear_bytes_per_scanline / pixel_size;
    std::size_t lfb_size = line_size * mode.second.resolution_y * mode.second.linear_num_image_pages;
    dpmi::device_memory<pixel> screen_ptr { lfb_size, mode.second.physical_base_ptr, true };
    matrix<pixel> screen { line_size, mode.second.resolution_y, screen_ptr.get_ptr() };
    auto r = screen.range({ 0, 0 }, { mode.second.resolution_x, mode.second.resolution_y });
    auto r2 = screen.range({ 0, mode.second.resolution_y }, { mode.second.resolution_x, mode.second.resolution_y });

    //v->set_scanline_length(mode.second.resolution_x);
    std::size_t resx = mode.second.resolution_x;
    std::size_t resy = mode.second.resolution_y;

    matrix_container<video::pxf> bg { resx, resy };
    matrix_container<video::pxf> fg { resx, resy };

    fg.fill(video::pxf { 0,0,0,0 });

    //auto palette = v->get_palette();
    //v->set_palette_format(8);
    //v->set_palette(palette);

    chrono::setup::setup_pit(true, 0x1000);
    chrono::setup::setup_tsc(10000);
    using clock = std::chrono::high_resolution_clock;

    auto last_now = clock::now();
    float t = 0;
    //float color = 0;
    constexpr auto fpi = static_cast<float>(M_PI);

    auto start = clock::now();
    while (clock::now() < start + 10s)
    {
        std::swap(r, r2);

        auto now = clock::now();
        float dt = (now - last_now).count() / 1e9f;
        last_now = now;

        t += dt;

        asm("fill_b%=:":::);
        bg.fill(video::pxf { 0.25f + std::sin(t) / 4.0f, 0.25f + std::sin(t + fpi) / 4.0f, 0.25f + std::sin(t + fpi / 2) / 4.0f, 1 });
        asm("fill_e%=:":::);

        asm("clearfg_b%=:":::);
        auto fgr = fg.range({ resx / 2 - 160, resy / 2 - 120 }, { 320,240 });
        fgr.fill(video::pxf { 0,0,0,0 });
        asm("clearfg_e%=:":::);

        asm("drawfg_b%=:":::);
        for (auto y = 0; y < fgr.height(); ++y)
            for (auto x = 0; x < fgr.width(); ++x)
            {
                vector2f pos { x / 320.0f, y / 240.0f };
                pos *= 2 * fpi;
                fgr(x, y) = video::pxf { 0.5f + std::sin(t + pos.x) / 2.0f, 0.5f + std::sin(t + pos.y) / 2.0f, 0.5f + std::sin(t + pos.x + pos.y) / 2.0f,  0.5f + std::sin(t + pos.x * pos.y) / 2.0f };
            }
        asm("drawfg_e%=:":::);

        /*
        video::pxf value { };
        value.r = 0.5f + std::sin(t * 2.0f) / 2;
        value.b = 0.5f + std::sin(t * 3.0f) / 2;
        value.g = 0.5f + std::sin(t * 5.0f) / 2;
        value.a = 0.75f + std::sin(t * 7.0f) / 4;
        fg.at({ 320 + std::sin(t * 10) * t*5, 240 + std::cos(t * 10) * t*5 }) = value;
        */

        asm("alpha_b%=:":::);
        for (auto y = 0; y < 480; ++y)
            for (auto x = 0; x < 640; ++x)
                bg(x, y).blend(fg(x, y));
        asm("alpha_e%=:":::);
        if (v->get_scheduled_display_start_status())
        {
            for (auto y = 0; y < 480; ++y)
                for (auto x = 0; x < 640; ++x)
                    r(x, y) = std::move(bg(x, y));
            v->schedule_display_start(r.position());
        }
        asm("assign_e%=:":::);

        //v->schedule_display_start({80 + std::sin(t) * 40, 80 + std::cos(t) * 40 });
    }
}

void game()
{
    using namespace std::chrono_literals;
    chrono::setup::setup_pit(true, 0x1000);
    chrono::setup::setup_tsc(10000);
    using clock = jw::chrono::tsc;

    std::cout << "synchronizing timer...\n";
    thread::yield_for(2s);

    dpmi::mapped_dos_memory<video::text_char> screen_ptr { 80 * 50, dpmi::far_ptr16 { 0xB800, 0 } };
    matrix<video::text_char> screen { 80, 50, screen_ptr.get_ptr() };

    fixed_matrix<video::text_char, 80, 50> m { };
    m.fill(video::text_char { ' ', 0xf, 1 });

    auto r = m.range({ 5,5 }, { 70,40 });
    r.fill(video::text_char { ' ', 0, 2 });

    matrix_container<clock::time_point> time_points { r.size() };

    io::keyboard keyb { std::make_shared<io::ps2_interface>() };

    io::gameport::config gameport_cfg { };
    gameport_cfg.enable.z = false;
    gameport_cfg.enable.w = false;

    std::cout << "calibrate joystick, press fire when done." << std::endl;
    {
        io::gameport joystick { gameport_cfg };
        std::swap(gameport_cfg.calibration.max, gameport_cfg.calibration.min);
        do
        {
            auto raw = joystick.get_raw();
            for (auto i = 0; i < 4; ++i)
            {
                gameport_cfg.calibration.min[i] = std::min(gameport_cfg.calibration.min[i], raw[i]);
                gameport_cfg.calibration.max[i] = std::max(gameport_cfg.calibration.max[i], raw[i]);
            }
        } while (joystick.buttons().none());
    }

    gameport_cfg.strategy = io::gameport::poll_strategy::busy_loop;
    io::gameport joystick { gameport_cfg };

    std::cout <<
        " x0_min=" << gameport_cfg.calibration.min[0].count() <<
        " y0_min=" << gameport_cfg.calibration.min[1].count() <<
        " x0_max=" << gameport_cfg.calibration.max[0].count() <<
        " y0_max=" << gameport_cfg.calibration.max[1].count() << '\n';

    do
    {
        joystick.get_raw();
    } while (joystick.buttons().any());

    bool collision = false;
    bool friction = true;
    vector2f delta { 0,0 };
    vector2f player { 20,20 };
    vector2f last_player { player };

    callback<void(io::key_state_pair)> key_event { [&](auto k)
    {
        if (k.first == io::key::c && k.second.is_down())
        {
            collision = !collision;
            auto fill = collision ? video::text_char { ' ', 0xf, 4 } : video::text_char { ' ', 0xf, 1 };
            m.range({  0,0  }, { 80,5  }).fill(fill);
            m.range({  0,5  }, {  5,45 }).fill(fill);
            m.range({ 75,5  }, {  5,45 }).fill(fill);
            m.range({  5,45 }, { 75,5  }).fill(fill);
            player.x = remainder(player.x, r.width());
            player.y = remainder(player.y, r.height());
            if (player.x < 0) player.x += r.width();
            if (player.y < 0) player.y += r.height();
        }
        if (k.first == io::key::f && k.second.is_down()) friction = !friction;
    } };
    keyb.key_changed += key_event;

    auto last_now = clock::now();
    while (true)
    {
        auto now = clock::now();
        float dt = (now - last_now).count() / 1e9f;
        last_now = now;

        using namespace io;
        auto joy = joystick.get();
        vector2f new_delta { joy.x, joy.y };
        keyb.update();
        if (keyb[key::up])    new_delta += vector2f::up();
        if (keyb[key::down])  new_delta += vector2f::down();
        if (keyb[key::left])  new_delta += vector2f::left();
        if (keyb[key::right]) new_delta += vector2f::right();
        if (keyb[key::space]) r.fill(video::text_char{' ', 0, 2});
        if (keyb[key::esc]) break;

        new_delta.clamp_magnitude(1);
        delta += new_delta * dt * 20;

        r.at(last_player) = video::text_char{' ', 7, 0};
        time_points.at(last_player) = now + std::chrono::seconds { 10 };
        for (auto y = 0; y < time_points.height(); ++y)
            for (auto x = 0; x < time_points.width(); ++x)
                if (time_points(x, y) <= now) r(x, y) = video::text_char{' ', 0, 2};

        player += delta * dt * 10;
        if (collision)
        {
            if (player.x <= 0) delta.x = -delta.x;
            if (player.x >= r.width()) delta.x = -delta.x;
            if (player.y <= 0) delta.y = -delta.y;
            if (player.y >= r.height()) delta.y = -delta.y;
            player.clamp(vector2i { 0,0 }, r.size());
        }
        r.at(player) = video::text_char{2, 0xf, 0};
        last_player = player;
        if (friction) delta -= delta * dt * 2;

        std::stringstream fps { };
        fps << "FPS: " << 1 / dt << " buttons=" << joystick.buttons() << " joy=" << joy << " delta=" << delta;
        auto* i { m.data() };
        for (auto c : fps.str()) *i++ = c;
        for (; i < m.data() + m.width();) *i++ = ' ';

        screen.assign(m);
        thread::yield();
    }
}

void ref_test(auto&& var)
{
    using T = decltype(var);
    std::cout << var;
    if (std::is_const_v<std::remove_reference_t<T>>) std::cout << " (const)";
    if (std::is_lvalue_reference_v<T>) std::cout << " (lv ref)";
    if (std::is_rvalue_reference_v<T>) std::cout << " (rv ref)";
    if (std::is_pointer_v<std::remove_reference_t<T>>) std::cout << " (ptr)";
    std::cout << '\n';
}

struct com_port : public io::pci_device
{
    com_port() : pci_device(io::pci_device::class_tag { }, 0x07, { 0x00 }, 0x02) { }

    void print_addr()
    {
        std::cout << std::hex;
        std::cout << "base0: 0x" << base0.read() << "\n";
        std::cout << "base1: 0x" << base1.read() << "\n";
        std::cout << "base2: 0x" << base2.read() << "\n";
        std::cout << "base3: 0x" << base3.read() << "\n";
        std::cout << "base4: 0x" << base4.read() << "\n";
        std::cout << "base5: 0x" << base5.read() << "\n";
        std::cout << "IRQ: 0x" << static_cast<std::uint32_t>(bus_info.read().irq) << "\n";
    }
};

void enumerate_ports()
{
    std::vector<com_port> ports;
    while (true)
    {
        try { ports.emplace_back(); }
        catch (const io::pci_device::device_not_found&) { break; }
        catch (const io::pci_device::unsupported_function&) { std::cerr << "no PCI\n"; break; }
    }
    for (auto&& i : ports) i.print_addr();
}

int jwdpmi_main(std::deque<std::string_view>)
{
    std::cout << "Hello, World!" << std::endl;

    /*
    enumerate_ports();

    {
        io::rs232_config cfg { };
        cfg.set_com_port(io::com1);
        //cfg.flow_control = io::rs232_config::rts_cts;
        io::rs232_stream s { cfg };

        s << "hello world!\r\n" << std::flush;

        std::string str;
        do
        {
            std::getline(std::cin, str);
            s << str << std::endl;
            std::getline(s, str);
            std::cout << str << std::endl;
        } while (str != "quit");
    }
    return 0; */
/*
    dpmi::breakpoint();

    {
        dpmi::trap_mask no_trace { };
        std::cout << "not tracing here...\n";
        dpmi::breakpoint();
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

    std::cout << "trace enabled again!\n";
  */
    debug::breakpoint();

    game();
    vbe_test();
    return 0;

    while(true)
    {
        auto get_chars = thread::make_coroutine<char>([](const std::string& str)
        {
            auto yield = [](auto c) { thread::coroutine_yield<char>(c); };
            for (auto&& c : str) yield(c);
        });

        get_chars->start("hello world.");
        while (get_chars->try_await()) std::cout << get_chars->await() << std::flush;
        std::clog << get_chars->pending_exceptions() << '\n';
    }

    //game();

    std::raise(SIGHUP);

    //vbe_test();

    /*
    auto test_thread = []()
    {
        while (true)
        {
            std::cout << "hello!\n";
            dpmi::breakpoint();
            thread::yield();
        }
    };

    std::function f { test_thread };
    thread::task thr1 { f };
    thread::task thr2 { f };

    thr1->start();
    thr2->start();
    thr1->await();
    thr2->await();
    */

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

    io::rs232_config cfg { };
    cfg.set_com_port(io::com1);
    //cfg.flow_control = io::rs232_config::rts_cts;
    auto s = io::make_rs232_stream(cfg);
    s << "hello world!\r\n" << std::flush;

    std::mutex m;
    auto chat = [&m](std::istream& in, std::ostream& out)
    {
        std::string input { };
        while (in.good())
        {
            std::getline(in, input);
            {
                std::scoped_lock<std::mutex> { m };
                out << "you said: " << input << "\r\n" << std::flush;
                thread::yield();
            }
            if (input.substr(0, 4) == "quit") break;
            thread::yield();
        }
    };
    thread::task t1 { chat };
    thread::task t2 { chat };

    t1->start(s, std::cout);
    t2->start(std::cin, s);
    t1->await();
    t2->await();

    return 0;
}
