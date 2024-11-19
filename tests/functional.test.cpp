#include <catch2/catch_test_macros.hpp>
#include <ferrugo/core/functional.hpp>

#include "matchers.hpp"

using namespace ferrugo;

namespace
{
struct test_struct
{
    std::string name;
};
}  // namespace

TEST_CASE("applied", "[functions]")
{
    REQUIRE_THAT(
        core::applied([](int lhs, char mid, const std::string& rhs)
                      { return std::to_string(lhs) + std::string(1, mid) + rhs; })(std::tuple{ 123, '-', "ABC" }),
        matchers::equal_to("123-ABC"));
}

TEST_CASE("proj", "[functions]")
{
    REQUIRE_THAT(
        core::proj(std::plus<>{}, &test_struct::name)(test_struct{ "ABC" }, test_struct{ "xyz" }),
        matchers::equal_to("ABCxyz"));
}

TEST_CASE("apply", "[functions]")
{
    std::string text = ".";
    REQUIRE_THAT(
        text |= core::apply(
            [](std::string& v) { v += "abc"; },
            [](std::string& v)
            {
                for (auto& ch : v)
                {
                    ch = std::toupper(ch);
                }
            }),
        matchers::equal_to(".ABC"));
    REQUIRE_THAT(text, matchers::equal_to(".ABC"));
}

TEST_CASE("with", "[functions]")
{
    const std::string text = ".";
    REQUIRE_THAT(
        text |= core::with(
            [](std::string& v) { v += "abc"; },
            [](std::string& v)
            {
                for (auto& ch : v)
                {
                    ch = std::toupper(ch);
                }
            }),
        matchers::equal_to(".ABC"));
    REQUIRE_THAT(text, matchers::equal_to("."));
}
