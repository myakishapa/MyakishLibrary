#include <iostream>
#include <tuple>
#include <utility>
#include <format>
#include <print>
#include <random>
#include <string>

#include <MyakishLibrary/Any.hpp>

#include <MyakishLibrary/Meta/Concepts.hpp>
#include <MyakishLibrary/Meta/Functions.hpp>
#include <MyakishLibrary/Meta/Pack.hpp>
#include <MyakishLibrary/Meta/Tag.hpp>

#include <MyakishLibrary/Streams/Concepts.hpp>
#include <MyakishLibrary/Streams/Common.hpp>

#include <MyakishLibrary/Functional/Pipeline.hpp>
#include <MyakishLibrary/Functional/Algebraic.hpp>

#include <MyakishLibrary/Utility.hpp>

#include <MyakishLibrary/Ranges/Bit.hpp>

#include <MyakishLibrary/HvTree/HvTree.hpp>
#include <MyakishLibrary/HvTree/Conversion/Conversion.hpp>
#include <MyakishLibrary/HvTree/Handle/HierarchicalHandle.hpp>
#include <MyakishLibrary/HvTree/Handle/StringWrapper.hpp>
#include <MyakishLibrary/HvTree/Data/DedicatedAllocation.hpp>
#include <MyakishLibrary/HvTree/Array/Array.hpp>

#include <MyakishLibrary/DependencyGraph/Graph.hpp>

#include <MyakishLibrary/BinarySerializationSuite/BinarySerializationSuite.hpp>


namespace st2 = myakish::streams;
namespace hv = myakish::tree;
namespace dg = myakish::dependency_graph;
namespace bst = myakish::binary_serialization_suite;

using namespace myakish::functional::operators;
using namespace hv::literals;
using namespace std::literals;

struct pes
{
    template<typename Tree>
    pes(int i1, Tree tree, int i2)
    {
        std::println("{} {}", i1 + i2, tree.Acquire<std::string>());
    }
};

template<>
struct dg::DefaultCreateTraits<pes>
{
    template<typename CreateArgs, typename DependencyProvider>
    static myakish::Any Create(CreateArgs tree, DependencyProvider&& prov)
    {
        auto& first = prov.Acquire<int>(tree["first"_sk].Acquire<std::string>());
        auto& second = prov.Acquire<int>(tree["second"_sk].Acquire<std::string>());
        return myakish::Any::Create<pes>(first, tree["str"_sk], second);
    }
};


int main()
{
    {
        std::vector<std::byte> data(1024);
 
        //auto out = st2::ContiguousStream(data.data(), data.data() + data.size());
        auto out = data | st2::WriteToRange;

        out | st2::Write(1337);
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
        
        in | st2::Copy(out, 32);
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

        std::println();
    }

    using PesHandle = hv::handle::hierarchical::Dynamic<std::string>;
    using PesData = hv::data::DedicatedAllocationStorage<PesHandle>;
    
    {
        static_assert(hv::handle::HandleOf<hv::handle::hierarchical::Static<std::string, 1>, hv::handle::hierarchical::Family<std::string>>, "");
        static_assert(hv::handle::Wrapper<hv::handle::hierarchical::Static<std::string, 1>, hv::handle::hierarchical::Family<std::string>>, "");
        static_assert(hv::handle::Wrapper<PesHandle, hv::handle::hierarchical::Family<std::string>>, "");

        
        static_assert(hv::handle::HandleOf<hv::handle::NullHandle<hv::handle::hierarchical::Family<std::string>>, hv::handle::hierarchical::Family<std::string>>, "");


        PesData data;
        hv::Descriptor tree(data);
        
        //auto pes = Resolve(hv::handle::FamilyTag<hv::handle::HandleFamily<PesHandle>>{}, 1_aa);

        auto t1 = tree[PesHandle("apa")];
        auto t2 = t1["pes"_sk];

        auto test = hv::handle::Resolve(hv::handle::hierarchical::Family<std::string>{}, t1.Base() / "pes"_sk);


        tree[PesHandle("apa")]["pes"_sk] = 1337;

        std::println("{}", tree["apa"_sk / "pes"_sk].Acquire<int>());
    }

    {
        PesData data;
        hv::Descriptor test(data);

        test["first"_sk] = "int1"s;
        test["second"_sk] = "int2"s;
        test["str"_sk] = "apapes"s;

        dg::Graph<std::string, decltype(test)> graph;
        
        graph.EmplaceNode<int>("int1", 1);
        graph.EmplaceNode<int>("int2", 2);

        graph.SetDependencies("pes", test);


        auto apa = graph.Acquire<pes>("pes");

        std::println("");

    }

    {
        PesData data;
        hv::Descriptor tree(data);
        
        
        constexpr auto asdf = myakish::meta::InstanceOf<myakish::tree::Descriptor>::template Func<decltype(tree)>::value;
        
        //auto test = hv::array::Range(0i64, 20i64)(tree);
        
        for (auto [index, desc] : tree | hv::array::Range(0i64, 20i64) | std::views::enumerate)
        {
            desc = int(index);
        }

        for (auto &&num : tree | hv::array::Existing | std::views::transform(hv::Acquire<int>))
        {
            std::print("{} ", num);
        }
        
        std::println();
    }

    {
        std::vector<std::string> strs = { "aaa", "aba", "baa", "aab", "bbb", "aa", "aaaa"};

        auto transform = [](auto str) { return str + (char)1; };
        
        auto transformed = strs | std::views::transform(transform) | std::ranges::to<std::vector>();

        strs.append_range(transformed);

        std::ranges::sort(strs);

        std::println();
    }

    {
        std::vector<std::byte> data(1024);

        //auto out = st2::ContiguousStream(data.data(), data.data() + data.size());
        auto out = data | st2::WriteToRange;
        auto in = data | st2::ReadFromRange;

        out | st2::Write(1337);

        auto rule = bst::Int;

        int val;

        rule.IO(in, val);

        std::println("{}", val);

    }

    {
        using namespace myakish::functional::algebraic;

        std::variant<int, float, long> a = 2l;

        auto zipWith3 = [](auto arg) { return std::tuple(arg, 3); };

        auto intToStr = [](int i) { return "kdfhgko"s; };
        auto floatToInt = [](float f) { return 3; };
        auto longToVec = [](long l) { return std::vector<int>{}; };

        auto t = a | Multitransform(intToStr, floatToInt, longToVec) | Transform(zipWith3);

        std::println();
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
