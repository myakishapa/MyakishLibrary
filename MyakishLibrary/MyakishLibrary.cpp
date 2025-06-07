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

#include <MyakishLibrary/Utility.hpp>

#include <MyakishLibrary/Ranges/Bit.hpp>

#include <MyakishLibrary/HvTree/HvTree.hpp>
#include <MyakishLibrary/HvTree/Conversion/Conversion.hpp>
#include <MyakishLibrary/HvTree/Handle/HierarchicalHandle.hpp>
#include <MyakishLibrary/HvTree/Handle/StringWrapper.hpp>
#include <MyakishLibrary/HvTree/Data/DedicatedAllocation.hpp>
#include <MyakishLibrary/HvTree/Array/Array.hpp>

#include <MyakishLibrary/DependencyGraph/Graph.hpp>

namespace st2 = myakish::streams;
namespace hv = myakish::tree;
namespace dg = myakish::dependency_graph;

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

        std::println();
    }

    using PesHandle = hv::handle::hierarchical::Dynamic<std::string>;
    using PesData = hv::data::DedicatedAllocationStorage<PesHandle>;
    
    {
        static_assert(hv::handle::HandleOf<hv::handle::hierarchical::Static<std::string, 1>, hv::handle::hierarchical::Family<std::string>>, "");
        static_assert(hv::handle::Wrapper<hv::handle::hierarchical::Static<std::string, 1>, hv::handle::hierarchical::Family<std::string>>, "");

        

        auto test = hv::handle::Resolve(hv::handle::hierarchical::Family<std::string>{}, hv::handle::hierarchical::Static<std::string, 2>{} );

        auto h = PesHandle(test);

        PesData data;
        hv::Descriptor tree(data);
        
        //auto pes = Resolve(hv::handle::FamilyTag<hv::handle::HandleFamily<PesHandle>>{}, 1_aa);

        tree["apa"_sk]["pes"_sk] = 1337;

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

        auto test = hv::array::MakeArrayIndex<PesData::HandleFamily>(0);

        for (auto [index, desc] : hv::array::Range(tree, 0, 20) | std::views::enumerate)
        {
            desc = int(index);
        }

        for (auto &&num : hv::array::Existing(tree) | std::views::transform(hv::Acquire<int>))
        {
            std::print("{} ", num);
        }
        
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
