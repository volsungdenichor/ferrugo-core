#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <ferrugo/core/ostream_utils.hpp>
#include <ferrugo/core/pipe.hpp>

#include "matchers.hpp"

using namespace ferrugo;

TEST_CASE("pipe", "[functional]")
{
    static const auto inc = [](int x) { return x + 1; };
    static const auto mul = [](int x) { return 3 * x; };
    static const auto s = [](int x) { return core::str("<", x, ">"); };
    static const auto inv = [](std::string v)
    {
        std::reverse(std::begin(v), std::end(v));
        return v;
    };
    static const auto p = core::pipe(inc, mul, s, inv);
    REQUIRE(p(1) == ">6<");
    REQUIRE(p(3) == ">21<");
    REQUIRE(p(10) == ">33<");
    REQUIRE(p(11) == ">63<");
}

TEST_CASE("compose", "[functional]")
{
    static const auto inc = [](int x) { return x + 1; };
    static const auto mul = [](int x) { return 3 * x; };
    static const auto s = [](int x) { return core::str("[", x, "]"); };
    static const auto p = core::compose(s, inc, mul);
    REQUIRE(p(1) == "[4]");
    REQUIRE(p(3) == "[10]");
}
