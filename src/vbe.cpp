#include <iostream>
#include <deque>
#include <string_view>
#include <jw/alloc.h>
#include <jw/vector.h>
#include <jw/grid.h>
#include <jw/video/pixel.h>
#include <jw/video/vbe.h>
#include <jw/video/ansi.h>
#include <chrono>

using namespace jw;

template<std::size_t bits_per_pixel = 8, typename render_type = float>
void vbe_test()
{
    using namespace std::literals;
    auto v = video::get_vbe_interface();
    auto info = v->get_vbe_info();

    std::cout << std::hex << "Detected VBE version:" << info.vbe_version << '\n';
    std::cout << info.oem_vendor_name << '\n';
    std::cout << info.oem_product_name << '\n';
    std::cout << info.oem_string << '\n';

    const auto& modelist = v->get_modes();
    const auto& mode = [&modelist]
    {
        for (const auto& mode : modelist)
        {
            auto& m = mode.second;
            if (m.resolution_x == 320 and
                m.resolution_y == 240 and
                (m.memory_model == video::vbe_mode_info::direct or m.memory_model == video::vbe_mode_info::packed_pixel) and
                m.bits_per_pixel == bits_per_pixel and
                m.attr.lfb_mode_available and
                m.linear_num_image_pages >= 2) return mode;
        }
        throw std::runtime_error("No suitable video mode found.");
    }();
    video::vbe_mode vmode { };
    vmode.mode = mode.first;

    std::cout << "Using video mode: 0x" << std::hex << mode.first << std::flush;
    thread::yield_for(4s);

    vmode.use_lfb_mode = true;
    v->set_mode(vmode);

    if (mode.second.bits_per_pixel == 8)
    {
        auto pal = video::generate_px8n_palette();
        v->set_palette(pal);
    }

    using pixel = std::conditional_t<bits_per_pixel == 8, video::px8n,
                  std::conditional_t<bits_per_pixel == 16, video::px16,
                  std::conditional_t<bits_per_pixel == 24, video::px24, void>>>;

    std::size_t pixel_size = mode.second.bits_per_pixel / 8;
    std::size_t line_size = mode.second.linear_bytes_per_scanline / pixel_size;
    std::size_t lfb_size = line_size * mode.second.resolution_y * mode.second.linear_num_image_pages;
    dpmi::device_memory<pixel> screen_ptr { lfb_size, mode.second.physical_base_ptr, true };
    grid<pixel> screen { line_size, mode.second.resolution_y, screen_ptr.get_ptr() };
    auto r = screen.range({ 0, 0 }, { mode.second.resolution_x, mode.second.resolution_y });
    //auto r2 = screen.range({ 0, mode.second.resolution_y }, { mode.second.resolution_x, mode.second.resolution_y });

    //v->set_scanline_length(mode.second.resolution_x);
    std::size_t resx = mode.second.resolution_x;
    std::size_t resy = mode.second.resolution_y;

    grid_container<std::conditional_t<std::is_floating_point_v<render_type>, video::pxfn, video::px32n>> bg { resx, resy };
    grid_container<std::conditional_t<std::is_floating_point_v<render_type>, video::pxf, video::px32a>> fg { resx, resy };

    fg.fill_nowrap(video::pxf { 0,0,0,0 });

    //auto palette = v->get_palette();
    //v->set_palette_format(8);
    //v->set_palette(palette);

    using clock = jw::chrono::pit;

    auto last_now = clock::now();
    float t = 0;
    //float color = 0;
    constexpr auto fpi = static_cast<float>(M_PI);

    auto start = clock::now();
    while (clock::now() < start + 15s)
    {
        //std::swap(r, r2);

        auto now = clock::now();
        float dt = (now - last_now).count() / 1e9f;
        last_now = now;

        t += dt;

        asm("fill_b%=:":::);
        bg.fill_nowrap(video::pxf { 0.25f + std::sin(t) / 4.0f, 0.25f + std::sin(t + fpi) / 4.0f, 0.25f + std::sin(t + fpi / 2) / 4.0f, 1 });
        asm("fill_e%=:":::);

        asm("clearfg_b%=:":::);
        auto fgr = fg.range({ resx / 2 - 80, resy / 2 - 60 }, { 160, 120 });
        fgr.fill_nowrap(video::pxf { 0,0,0,0 });
        asm("clearfg_e%=:":::);

        asm("drawfg_b%=:":::);
        for (auto y = 0; y < fgr.height(); ++y)
            for (auto x = 0; x < fgr.width(); ++x)
            {
                vector2f pos { x / 160.0f, y / 120.0f };
                pos *= 2 * fpi;
                fgr.nowrap(x, y) = video::pxf { 0.5f + std::sin(t + pos.x()) / 2.0f,
                                                0.5f + std::sin(t + pos.y()) / 2.0f,
                                                0.5f + std::sin(t + pos.x() + pos.y()) / 2.0f,
                                                0.5f + std::sin(t + pos.x() * pos.y()) / 2.0f }.premultiply_alpha();
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
        for (std::size_t y = 0; y < resy; ++y)
            for (std::size_t x = 0; x < resx; ++x)
                bg.nowrap(x, y).blend(fg(x, y));

        asm("alpha_e%=:":::);
        if (v->get_scheduled_display_start_status())
        {
            r.assign_nowrap(bg);
            //v->schedule_display_start(r.position());
        }
        asm("assign_e%=:":::);

        //v->schedule_display_start({80 + std::sin(t) * 40, 80 + std::cos(t) * 40 });
    }
}

int jwdpmi_main(std::deque<std::string_view>)
{
    using namespace jw::video::ansi;
    using namespace std::literals;

    chrono::setup::setup_pit(true, 0x1000);
    chrono::setup::setup_tsc(10000);

    std::cout << reset() + set_80x50_mode() << "8bpp, floating-point render target\n";
    thread::yield_for(1s);
    vbe_test<8, float>();
    std::cout << reset() + set_80x50_mode() << "16bpp, floating-point render target\n";
    thread::yield_for(1s);
    vbe_test<16, float>();

    std::cout << reset() + set_80x50_mode() << "8bpp, integer render target\n";
    thread::yield_for(1s);
    vbe_test<8, int>();
    std::cout << reset() + set_80x50_mode() << "16bpp, integer render target\n";
    thread::yield_for(1s);
    vbe_test<16, int>();

    std::cout << reset() + set_80x50_mode() << std::flush;
    return 0;
}
