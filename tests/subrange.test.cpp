#include <catch2/catch_test_macros.hpp>
#include <ferrugo/core/subrange.hpp>

#include "matchers.hpp"

using namespace ferrugo;

TEST_CASE("subrange - constructor", "[subrange]")
{
    std::vector<int> v = { 10, 20, 30, 40 };
    REQUIRE_THAT(core::subrange(v.begin(), v.begin() + 2).size(), matchers::equal_to(2));
    REQUIRE_THAT(core::subrange(v).size(), matchers::equal_to(4));
    REQUIRE_THAT(core::subrange(v).front(), matchers::equal_to(10));
    REQUIRE_THAT(core::subrange(v).back(), matchers::equal_to(40));
}

TEST_CASE("subrange - slice", "[subrange]")
{
    std::vector<int> v = { 10, 20, 30, 40, 50, 60 };
    REQUIRE_THAT(v |= core::slice({}, {}), matchers::elements_are(10, 20, 30, 40, 50, 60));
    REQUIRE_THAT(v |= core::slice({}, 2), matchers::elements_are(10, 20));
    REQUIRE_THAT(v |= core::slice(2, 4), matchers::elements_are(30, 40));
    REQUIRE_THAT(v |= core::slice(4, {}), matchers::elements_are(50, 60));
    REQUIRE_THAT(v |= core::slice({}, -3), matchers::elements_are(10, 20, 30));
    REQUIRE_THAT(v |= core::slice(-3, {}), matchers::elements_are(40, 50, 60));
    REQUIRE_THAT(v |= core::slice(-4, -2), matchers::elements_are(30, 40));
}

TEST_CASE("subrange - out of range", "[subrange]")
{
    std::vector<int> v = { 10, 20, 30, 40, 50, 60 };
    REQUIRE_THAT(v |= core::slice(5, 2), matchers::is_empty());
    REQUIRE_THAT(v |= core::slice(-100, 2), matchers::elements_are(10, 20));
    REQUIRE_THAT(v |= core::slice(50, 100), matchers::is_empty());
    REQUIRE_THAT(v |= core::slice({}, 100), matchers::elements_are(10, 20, 30, 40, 50, 60));
}

TEST_CASE("subrange - reverse", "[subrange]")
{
    std::vector<int> v = { 10, 20, 30, 40, 50, 60 };
    REQUIRE_THAT(v |= core::reverse, matchers::elements_are(60, 50, 40, 30, 20, 10));
    REQUIRE_THAT(v |= core::reverse |= core::reverse, matchers::elements_are(10, 20, 30, 40, 50, 60));
}

TEST_CASE("subrange - take_while", "[subrange]")
{
    std::vector<int> v = { 10, 20, 30, 40, 50, 60 };
    REQUIRE_THAT(v |= core::take_while([](int x) { return x < 30; }), matchers::elements_are(10, 20));
    REQUIRE_THAT(v |= core::take_while([](int) { return false; }), matchers::is_empty());
    REQUIRE_THAT(v |= core::take_while([](int) { return true; }), matchers::elements_are(10, 20, 30, 40, 50, 60));
}

TEST_CASE("subrange - drop_while", "[subrange]")
{
    std::vector<int> v = { 10, 20, 30, 40, 50, 60 };
    REQUIRE_THAT(v |= core::drop_while([](int x) { return x < 30; }), matchers::elements_are(30, 40, 50, 60));
    REQUIRE_THAT(v |= core::drop_while([](int) { return false; }), matchers::elements_are(10, 20, 30, 40, 50, 60));
    REQUIRE_THAT(v |= core::drop_while([](int) { return true; }), matchers::is_empty());
}
