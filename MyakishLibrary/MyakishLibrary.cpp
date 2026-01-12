#include <iostream>
#include <tuple>
#include <utility>
#include <format>
#include <print>
#include <random>
#include <string>

#include <MyakishLibrary/Any.hpp>


#include <MyakishLibrary/Meta.hpp>

#include <MyakishLibrary/Streams/Common.hpp>

#include <MyakishLibrary/Utility.hpp>

#include <MyakishLibrary/Ranges/Bit.hpp>

#include <MyakishLibrary/HvTree/HvTree.hpp>
#include <MyakishLibrary/HvTree/Build.hpp>
#include <MyakishLibrary/HvTree/Parser/Parser.hpp>

#include <MyakishLibrary/DependencyGraph/Graph.hpp>

#include <MyakishLibrary/BinarySerializationSuite/BinarySerializationSuite.hpp>

#include <MyakishLibrary/Algebraic/Algebraic.hpp>


namespace st2 = myakish::streams;
namespace hv = myakish::tree;
namespace dg = myakish::dependency_graph;
namespace bss = myakish::binary_serialization_suite;
namespace meta = myakish::meta;
namespace hof = myakish::functional;
namespace alg = myakish::algebraic;

using namespace myakish::literals;
using namespace std::literals;
using namespace hof::shorthands;

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
        /*auto& first = prov.Acquire<int>(tree["first"_sk].Acquire<std::string>());
        auto& second = prov.Acquire<int>(tree["second"_sk].Acquire<std::string>());
        return myakish::Any::Create<pes>(first, tree["str"_sk], second);*/
    }
};


struct IntOrNotFunctor : hof::ExtensionMethod
{
    template<typename Type>
    void operator()(Type value) const
    {
        if (std::integral<Type>) std::println("int");
        else std::println("not int");
    }
};
inline constexpr IntOrNotFunctor IntOrNot;


int main()
{
    //streams
    {
        // streams basics
        {
            std::vector<std::byte> data(1024);

            //auto out = st2::ContiguousStream(data.data(), data.data() + data.size());
            auto out = data | st2::WriteToRange;

            out | st2::WriteTrivial[1337];
            st2::WriteTrivial(out, 228);

            auto in = data | st2::ReadFromRange | st2::Polymorphize;

            auto i1 = st2::ReadTrivial<int>(in);
            auto i2 = st2::ReadTrivial<int>(in);

            std::println("{} {}", i1, i2);
        }

        // streams Read
        {
            auto in = std::views::iota(0) | st2::ReadFromRange;

            auto i1 = st2::ReadTrivial<std::uint16_t>(in);
            auto i2 = st2::ReadTrivial<std::uint16_t>(in);

            for (int i = 0; i < 10; i++)
            {
                std::print("{} ", in | st2::ReadTrivial<std::uint16_t>);
                in.Seek(4);
            }

            std::println("{} {}", i1, i2);

        }

        // streams File
        {
            auto in = std::views::iota(0) | st2::ReadFromRange;
            auto out = st2::FileOutputStream("test.bin");

            auto test = st2::Copy[out, 32];

            in | st2::Copy[out, 32];
        }

    }

    //Misc
    {
        // AsBytePtr
        {
            int i = 1;
            auto b = myakish::AsBytePtr(&i);
            const int* const p = &i;
            auto b2 = myakish::AsBytePtr(&p);
        }


        //bit ranges
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

        //bit ranges 2
        {
            std::array<int, 10> test;
            std::ranges::fill(test, 0);

            for (auto&& bit : test | myakish::ranges::Bits | std::views::drop(4) | std::views::stride(32))
            {
                bit = true;
            }

            std::println();
        }

        //Collatz-Weyl PRNG
        {
            for (auto test : myakish::CollatzWeylPRNG(0i64, 0i64, 1i64) | std::views::take(100))
            {
                std::print("{:b}", test);
            }

            std::println();
        }

        //DependencyGraph
        {
            /*PesData data;
            hv::Descriptor test(data);

            test["first"_sk] = "int1"s;
            test["second"_sk] = "int2"s;
            test["str"_sk] = "apapes"s;

            dg::Graph<std::string, decltype(test)> graph;

            graph.EmplaceNode<int>("int1", 1);
            graph.EmplaceNode<int>("int2", 2);

            graph.SetDependencies("pes", test);


            auto apa = graph.Acquire<pes>("pes");

            std::println("");*/

        }

    }

    //BSS
    {
        // basics
        {
            std::vector<std::byte> data(1024);

            //auto out = st2::ContiguousStream(data.data(), data.data() + data.size());
            auto out = data | st2::WriteToRange;
            auto in = data | st2::ReadFromRange;

            out | st2::WriteTrivial[1337];

            auto rule = bss::Int;

            int val;

            rule(in, val);

            std::println("{}", val);

        }

        // VariantParser
        {
            std::vector<std::byte> data(1024);

            auto out = data | st2::WriteToRange;
            auto in = data | st2::ReadFromRange;

            alg::Variant<int, float> val = 20.5f;

            constexpr auto rule = bss::VariantParser(bss::Int, bss::Trivial<float>, bss::Trivial<long double>);

            //using Attribute = decltype(rule)::Attribute;

            rule(out | st2::WriteOnly, val);

            alg::Variant<int, float> val2;

            rule(in, val2);

            std::println("{}", val2 | alg::GetByType<float>);

        }

        // RepeatParser
        {
            std::vector<std::byte> data(1024);

            auto out = data | st2::WriteToRange;
            auto in = data | st2::ReadFromRange;

            std::vector<int> srcVec = { 1, 2, 3, 4, 5 };

            constexpr auto rule = bss::RepeatParser(bss::Int);

            rule(out | st2::WriteOnly, srcVec);

            std::vector<int> dstVec;

            rule(in, dstVec);

            std::println();
        }
    }

    // meta
    {
        // basics
        {
            using l1 = meta::TypeList<int, float>;
            using l2 = meta::TypeList<double, char>;
            using l3 = meta::TypeList<std::string, float, long>;

            using test = meta::First<std::is_integral, l3>::type;

            using c = meta::Concat<l1, l2, l3>::type;
            using z = meta::Zip<l1, l2>::type;

            using integers = meta::QuotedFilter<meta::Not<std::is_void>, c>::type;

            using args = meta::TypeList<int, std::string>;
            using fns = meta::TypeList<float(int), double(std::string)>;

            using zipped = meta::Zip<fns, args>::type;
            using invokeResultQ = meta::Quote<std::invoke_result>;
            using fn = meta::LeftCurry<meta::QuotedInvoke, invokeResultQ>;

            using results = meta::QuotedMap<fn, zipped>::type;

            using constFloat = meta::CopyQualifiers<const int&&, float>::type;

            constexpr auto djkfgd = std::is_const_v<const int&>;
        }

        //unique
        {
            using list = meta::TypeList<std::string, float&, long, float&, int&&, const int&, std::string>;

            using unique = meta::Unique<list>::type;

            unique dflgk{};

            std::println();

        }

        // ForEach
        {
            using List = meta::TypeList<int, float>;

            auto Func = []<typename Type>(meta::TypeValueType<Type>)
            {
                if (std::integral<Type>) std::println("int");
                else std::println("not int");
            };

            meta::ForEach<List>(Func);

        }
    }

    // algebraic
    {
        // basics
        {
            std::variant<int, float, long> var = 2l;

            auto intToStr = [](int i) { return 'a'; };
            auto floatToInt = [](float f) { return 3; };
            auto transformLong = [](long l) { l = 4; return l; };

            auto zipWith3 = [](auto arg) { return std::tuple(arg, 3); };

            auto result = var | alg::Map[intToStr, floatToInt, transformLong] | alg::Map[zipWith3];

            auto h = result | alg::Get<2>;

            std::println();
        }

       auto test = [](auto arg)
            {
                if (std::integral<decltype(arg)>) std::println("int {}", arg);
                else std::println("not int {}", arg);
            };

        // Visit
        {
            float f = 10.f;
            float f2 = 15.f;

            alg::Variant<float&, int> sum(f);

            alg::Visit(sum, test);

            f = 20.f;

            sum = 1;

            sum | alg::Visit[test];

            sum = f2;

            auto val = sum | alg::Get<0>;

            std::println("{}", val);
        }
      
        // Flatten Tuple
        {
            float f = 10.f;

            alg::Tuple<int, float&> tuple(1, f);
            alg::Tuple<int, float&> tuple2(tuple);

            alg::Tuple<alg::Tuple<int, float&>, std::string, alg::Tuple<int, float&>> tupleOfTuples(tuple, "psink", tuple2);


            auto flat = tupleOfTuples | alg::Join;

            auto t1 = flat | alg::Get<0>;
            auto t2 = flat | alg::Get<1>;
            auto t3 = flat | alg::Get<2>;
            auto t4 = flat | alg::Get<3>;
            auto t5 = flat | alg::Get<4>;

            std::println("");
        }

        // Flatten Variant
        {
            float f = 10.f;

            alg::Variant<int&, alg::Variant<float&, std::string>> var(f);


            auto flat = var | alg::Join;

            auto& t = flat | alg::GetByType<float&>;

            std::println("");
        }

        // Unique
        {
            float f = 10.f;

            alg::Variant<int&, alg::Variant<float&, std::string, int&>> var(f);


            auto flat = var | alg::Join | alg::Unique | alg::Values;

            auto t = flat | alg::GetByType<float>;

            std::println("");
        }

        // Is
        {
            alg::Variant<int, float, long> var(2l);

            auto t1 = var | alg::Is<long>;
            auto t2 = var | alg::Is<long&>;
            auto t3 = var | alg::Is<long, std::is_same>;
            auto t4 = var | alg::Is<long&, std::is_same>;
            auto t5 = var | alg::Is<int, std::is_same>;

            std::println();
        }

        // First*
        {
            std::variant<int, std::string> var = 2l;

            auto intToStr = [](int i) { return 'a'; };

            auto result = var | alg::FirstMap[intToStr, hof::Identity];

            auto eval = result | alg::Evaluate;

            std::println();
        }

        //Maybe
        {
            std::optional<int> var = 1337;
            std::optional<int> var2{};

            auto intToStr = [](int i) { return std::format("{}", i); };

            auto r1 = var  | alg::FirstMap[intToStr, hof::Identity] | alg::Maybe<std::string>["Empty"];
            auto r2 = var2 | alg::FirstMap[intToStr, hof::Identity] | alg::Maybe<std::string>["Empty"];

            std::println();
        }
    }
    

    // functional
    {
        // composition
        {
            auto func1 = [](int i) -> float { return i * 2.f; };
            auto func2 = [](float f) -> double { return f * 2.0; };

            auto comp = func1 >> hof::Identity >> func2;

            //auto val = comp(2);

            std::println();
        }

        // folds
        {
            constexpr auto test = hof::RightFold(hof::Plus, 1);
            constexpr auto test3 = hof::RightFold(hof::Plus, 1, 2, 3);
        }

        // lambdas
        {
            int i = 2;

            auto fgh = hof::ConstantExpression(3);
            
            

            auto l = hof::Identity[hof::Identity[$0]];
            auto l2 = hof::Identity[$0];
            auto l3 = hof::LambdaClosure(hof::Identity, hof::detail::MakeExpression($0));

            auto lkdhf = l(1);

            using dfgl = hof::UnwrappedLikeT<const hof::IdentityFunctor&, int&&>;

            using dsgf = hof::UnwrappedT<meta::LikeT<int&&, hof::WrappedT<const hof::IdentityFunctor&>>>;
            using dsgf2 = meta::LikeT<int&&, hof::WrappedT<const hof::IdentityFunctor&>>;

            meta::TypeList<dsgf2> asd;

            using dsgf = hof::UnwrappedT<dsgf2>;

            std::println();
        }
        {
            auto l = hof::RightFold[hof::Plus, $0a];


            std::tuple t(1, 2, 3, 4);
            std::tuple t2(5, 6, 7, 8);

            auto fgldkh = l(t);

            auto l2 = hof::Plus[$0i2, $1i3];

            auto df = alg::Get<0>(t);

            auto gfkdj = l2(t, t2);
            //auto gfkdj = l2(t);

            std::println();
        }
    }


    // HvTree
    {
        // basics
        {
            auto source = "apa"_tree * (4 | hv::Via[bss::U64])
                / ("pes"_tree * hv::RawData(3))
                / ("aps"_tree * hv::RawData(4))
                / ("hvosti"_tree * hv::RawData(5));

            auto storage = hv::Build(source);

            auto source2 = "psink"_tree * hv::RawData(6)
                / (storage % "sobanchik"_tree)
                / (source % "sobanchik2"_tree)
                / ("psinki"_tree * hv::RawData(7));

            auto storage2 = hv::Build(source2);

            static_assert(hv::TreeConcept<hv::Storage<std::string>::EntryHandle>);

            auto pes = storage2.Root() | hv::At["sobanchik"] | hv::At["hvosti"];
            auto hv = hv::Acquire<int>(pes);


            std::println();
        }

        // parsing
        {
            auto file = myakish::ReadTextFile("myakishParserTest.hvr");

            auto test1 = hv::parse::grammar::Parse(file, boost::parser::trace::off);
            auto entries = hv::parse::ast::Parse(file);

            hv::parse::ast::DebugPrint(entries);

            auto storage = hv::Build(hv::parse::EntriesSource(entries, hv::parse::IntParser));
            
            auto storage2 = hv::parse::Parse(file, hv::parse::IntParser);

            std::println();
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
