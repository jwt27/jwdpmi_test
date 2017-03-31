#include <iostream>
#include <deque>
#include <string>
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
#include <cstdio>
#include <cstddef>

using namespace jw;
/*
template<typename T> struct matrix;
template<typename M> struct matrix_iterator_y;

enum direction
{
    up, down, left, right
};

template<direction dir, typename M>
struct matrix_iterator
{
    using T = typename M::value_type;
    constexpr matrix_iterator(M& matrix, T* ptr) : m(matrix), p(ptr) { }

    constexpr auto* operator->() noexcept { return p; }
    constexpr auto& operator*() noexcept { return *p; }

    constexpr auto operator[](std::ptrdiff_t n) const
    {
        switch (dir)
        {
        case up:    return matrix_iterator<right, M> { *this + n };
        case down:  return matrix_iterator<left, M>  { *this + n };
        case left:  return matrix_iterator<up, M>    { *this + n };
        case right: return matrix_iterator<down, M>  { *this + n };
        }
    }

    constexpr auto& operator+=(std::ptrdiff_t n)
    {
        switch (dir)
        {
        case up:    p -= n * m.width(); break;
        case down:  p += n * m.width(); break;
        case left:  p -= n; break;
        case right: p += n; break;
        }
        return *this;
    }

    constexpr auto& operator-=(std::ptrdiff_t n) { return operator+=(-n); }

    M& m;
    T* p;
};

template<typename T>
struct matrix
{
    using value_type = T;
    constexpr matrix(T* ptr, std::size_t width, std::size_t height) : data(ptr), w(width), h(height) { }

    template<direction dir>
    constexpr auto get_iterator();

    constexpr auto& operator()(std::size_t x, std::size_t y) { return *(data + x + w * y); }
    constexpr auto width() const noexcept { return w; }
    constexpr auto height() const noexcept { return h; }
    constexpr auto size() const noexcept { return w * h; }

protected:
    T* data;
    std::size_t w, h;
};*/

int jwdpmi_main(std::deque<std::string>)
{
    std::cout << "Hello, World!" << std::endl;
    dpmi::breakpoint();
    
    io::keyboard keyb { std::make_shared<io::ps2_interface>() };

    chrono::chrono::setup_pit(true, 0x1000);
    chrono::chrono::setup_tsc(1000);
    using clock = chrono::tsc;

    dpmi::mapped_dos_memory<std::uint16_t> screen_ptr { 80 * 50, dpmi::far_ptr16 { 0xB800, 0 } };
    matrix<std::uint16_t> screen { 80, 50, screen_ptr.get_ptr() };
    matrix_container<std::uint16_t> m { 80, 50 };
    m.fill(0x1000);

    auto r = m.make_range({ 5,5 }, { 70,40 });
    r.fill(0x2000);

    vector2f delta { 0,0 };
    vector2f player { 20,20 };
    auto last_now = clock::now();
    while (true)
    {
        auto now = clock::now();
        float dt = (now - last_now).count() / 1e9f;
        last_now = now;

        using namespace io;
        keyb.update();
        vector2f new_delta { 0,0 };
        if (keyb[key::up])      new_delta += vector2f::up();
        if (keyb[key::down])    new_delta += vector2f::down();
        if (keyb[key::left])    new_delta += vector2f::left();
        if (keyb[key::right])   new_delta += vector2f::right();
        if (keyb[key::esc]) break;
        delta += new_delta * dt * .2;
        if (delta.magnitude() > 1) delta.normalize();

        r(player) = 0;
        player += delta;
        r(player) = 0x0F02;
        delta -= delta * dt * 2;
        screen.assign(m);
        thread::yield();
    }
  /*  
    using clock = chrono::tsc;
    using ref = chrono::rtc;
    auto ref_now = ref::now();
    auto now = clock::now();
    auto last_now = now;
    auto t = now;
    double average = 0;
    for (auto i = 0; i < 1000000; ++i)
    {
        t += 100ms;
        //std::cout << "now=" << now.time_since_epoch().count() << "\t t=" << t.time_since_epoch().count() << '\n';
        thread::yield_until(t);
        //while (clock::now() < t) { }
        //thread::yield();
        last_now = now;
        auto last_ref = ref_now;
        now = clock::now();
        ref_now = ref::now();
        std::cout << "i=" << i << ", \terror=" << (now - t).count() << ", \tcycle=" << (ref_now - last_ref).count() << '\n';
        average += (ref_now - last_ref).count();
        std::cout << "average cycle=" << average / (i+1) << '\n';
    }
    std::cout << "cycle=" << (now - last_now).count() << "\terror=" << (now - t).count()/1000000 << '\n';
*/
    //dos_mem_test();

    return 0;
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

    thread::task<void()> thr1 { test_thread };
    thread::task<void()> thr2 { test_thread };

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

    while (true) { thread::yield(); }
    return 0;

    io::rs232_config cfg { };
    cfg.set_com_port(io::com2);
    //cfg.flow_control = io::rs232_config::rts_cts;
    io::rs232_stream s { cfg };
    s << "hello world!\r\n" << std::flush;


    auto chat = [](auto& in, auto& out)
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
    thread::task<void(std::istream& in, std::ostream& out)> t1 { chat };
    thread::task<void(std::istream& in, std::ostream& out)> t2 { chat };

    t1->start(s, std::cout);
    t2->start(std::cin, s);
    t1->await();
    t2->await();

    return 0;
}
