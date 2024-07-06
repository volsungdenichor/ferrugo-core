#include <catch2/catch_test_macros.hpp>
#include <ferrugo/core/container_utils.hpp>

#include "matchers.hpp"

using namespace std::string_view_literals;

using namespace ferrugo;

TEST_CASE("vec - single item", "[containers]")
{
    REQUIRE_THAT(core::vec(10), matchers::elements_are(10));
}

TEST_CASE("vec - multiple items, same type", "[containers]")
{
    REQUIRE_THAT(core::vec(10, 20, 30), matchers::elements_are(10, 20, 30));
}

TEST_CASE("concat vectors", "[containers]")
{
    REQUIRE_THAT(core::concat(core::vec(10, 20, 30), core::vec(100), core::vec(20, 30)), matchers::elements_are(10, 20, 30, 100, 20, 30));
}
