#include <iostream>
#include <deque>
#include <string>
#include <string_view>
#include <experimental/vector>
#include <jw/dpmi/irq.h>
#include <jw/io/ioport.h>
#include <jw/io/rs232.h>
#include <jw/dpmi/cpu_exception.h>
#include <jw/dpmi/debug.h>
#include <jw/dpmi/realmode.h>
#include <jw/thread/task.h>
#include <jw/thread/coroutine.h>
#include <jw/io/keyboard.h>
#include <jw/io/ps2_interface.h>
#include <jw/chrono/chrono.h>
#include <jw/alloc.h>
#include <jw/vector2.h>
#include <jw/matrix.h>
#include <jw/io/mpu401.h>
#include <jw/io/pci.h>
#include <jw/video/pixel.h>
#include <jw/video/vbe.h>
#include <jw/math.h>
#include <cstdio>
#include <cstddef>

using namespace jw;

void vbe_test()
{

    //auto v = new video::vbe2 { };
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
    std::size_t resx = 640;// mode.second.resolution_x;
    std::size_t resy = 480;// mode.second.resolution_y;

    matrix_container<video::pxf> bg { resx, resy };
    matrix_container<video::pxf> fg { resx, resy };

    fg.fill(video::pxf { 0,0,0,0 });

    //auto palette = v->get_palette();
    //v->set_palette_format(8);
    //v->set_palette(palette);

    chrono::chrono::setup_pit(true, 0x1000);
    chrono::chrono::setup_tsc(10000);
    using clock = chrono::tsc;

    auto last_now = clock::now();
    float t = 0;
    //float color = 0;
    constexpr auto fpi = static_cast<float>(M_PI);

    while (true)
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
    chrono::chrono::setup_pit(true, 0x1000);
    chrono::chrono::setup_tsc(10000);
    using clock = chrono::tsc;

    dpmi::mapped_dos_memory<video::text_char> screen_ptr { 80 * 50, dpmi::far_ptr16 { 0xB800, 0 } };
    matrix<video::text_char> screen { 80, 50, screen_ptr.get_ptr() };

    fixed_matrix<video::text_char, 80, 50> m { };
    m.fill(video::text_char { ' ', 0xf, 1 });

    auto r = m.range({ 5,5 }, { 70,40 });
    r.fill(video::text_char { ' ', 0, 2 });

    matrix_container<clock::time_point> time_points { r.size() };

    io::keyboard keyb { std::make_shared<io::ps2_interface>() };
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
        vector2f new_delta { 0,0 };
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
        fps << "FPS: " << 1 / dt;
        auto* i { m.data() };
        for (auto c : fps.str()) *i++ = c;
        for (; i < m.data() + m.width();) *i++ = ' ';

        screen.assign(m);
        thread::yield();
    }
}

int jwdpmi_main(std::deque<std::string_view>)
{
    std::cout << "Hello, World!" << std::endl;
    dpmi::breakpoint();

    thread::coroutine<char(const std::string&)> get_chars { [](auto& self, const std::string& str)
    {
        for (auto c : str) self.yield(c);
    } };

    get_chars->start("hello world.");
    while (get_chars->try_await()) std::cout << get_chars->await();

    game();
    vbe_test();
    
    
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

    std::function f { test_thread };
    thread::task thr1 { f };
    thread::task thr2 { f };

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

    io::rs232_config cfg { };
    cfg.set_com_port(io::com2);
    //cfg.flow_control = io::rs232_config::rts_cts;
    io::rs232_stream s { cfg };
    s << "hello world!\r\n" << std::flush;


    auto chat = [](std::istream& in, std::ostream& out)
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
    thread::task t1 { chat };
    thread::task t2 { chat };

    t1->start(s, std::cout);
    t2->start(std::cin, s);
    t1->await();
    t2->await();

    return 0;
}
