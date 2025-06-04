#include <iostream>
#include <tuple>
#include <utility>
#include <format>
#include <print>
#include <random>

#include <MyakishLibrary/Any.hpp>

#include <MyakishLibrary/Meta/Concepts.hpp>
#include <MyakishLibrary/Meta/Functions.hpp>
#include <MyakishLibrary/Meta/Pack.hpp>
#include <MyakishLibrary/Meta/Tag.hpp>

#include <MyakishLibrary/Streams/Concepts.hpp>
#include <MyakishLibrary/Streams/Common.hpp>

#include <MyakishLibrary/Functional/Pipeline.hpp>

#include <MyakishLibrary/Utility.hpp>

#include <MyakishLibrary/Ranges/Bit.hpp>

namespace st2 = myakish::streams2;

using namespace myakish::functional::operators;

int main()
{
    {
        std::vector<std::byte> data(1024);
 
        //auto out = st2::ContiguousStream(data.data(), data.data() + data.size());
        auto out = data | st2::WriteToRange;

        st2::Write(out, 1337); 
        st2::Write(out, 228);

        auto in = data | st2::ReadFromRange | st2::Polymorphize;

        auto i1 = st2::Read<int>(in);
        auto i2 = st2::Read<int>(in);

        std::println("{} {}", i1, i2);
    }

    {
        int i = 1;
        auto b = myakish::AsBytePtr(&i);
        const int* const p = &i;
        auto b2 = myakish::AsBytePtr(&p);
    }
    
    {
        auto in = std::views::iota(0) | st2::ReadFromRange;

        auto i1 = st2::Read<std::uint16_t>(in);
        auto i2 = st2::Read<std::uint16_t>(in);

        for (int i = 0; i < 10; i++)
        {
            std::print("{} ", in | st2::Read<std::uint16_t>);
            in.Seek(4);
        }

        std::println("{} {}", i1, i2);

    }
    
    {
        auto in = std::views::iota(0) | st2::ReadFromRange;  
        auto out = st2::FileOutputStream("test.bin");
        
        st2::Copy(in, out, 32);
    }

    {
        myakish::ranges::BitDescriptor desc{};
        int test = 0;

        auto u1 = desc.AbsoluteIndex();

        desc.Reference(myakish::AsBytePtr(&test), 0);
        desc = true;
        bool t1 = desc;

        auto u2 = desc.AbsoluteIndex();

        desc.Bit(4);
        desc = true;
        bool t2 = desc;

        auto u3 = desc.AbsoluteIndex();
        auto d1 = u3 - u2;

        desc.Bit(0);
        desc = false;
        bool t3 = desc;

        auto u4 = desc.AbsoluteIndex();

        desc.Byte(desc.Byte() + 1);
        desc = true;

        auto u5 = desc.AbsoluteIndex();
        auto d2 = u5 - u4;

        std::println("{}", test);
    }

    {
        std::array<int, 10> test;
        std::ranges::fill(test, 0);

        for (auto&& bit : test | myakish::ranges::Bits | std::views::drop(4) | std::views::stride(32))
        {
            bit = true;
        }

        std::println();
    }

    {
        std::tuple a(1, 2.f, 3.l, 4u);

        auto [f, s] = myakish::SliceTuple<4>(a);
    }

    {
        for (auto test : myakish::CollatzWeylPRNG(0i64, 0i64, 1i64) | std::views::take(100))
        {
            std::print("{:b}", test);
        }
    }
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
