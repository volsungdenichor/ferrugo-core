#include <catch2/catch_test_macros.hpp>
#include <ferrugo/core/ostream_utils.hpp>

#include "matchers.hpp"

using namespace ferrugo;

TEST_CASE("ostream_iterator", "[ostream]")
{
    std::stringstream ss;
    std::vector<int> v{ 2, 3, 5, 7, 11 };
    std::copy(std::begin(v), std::end(v), core::ostream_iterator{ ss, ", " });
    REQUIRE(ss.str() == "2, 3, 5, 7, 11, ");
}

TEST_CASE("delimit", "[ostream]")
{
    std::vector<int> v{ 2, 3, 5, 7, 11 };
    REQUIRE(core::str(core::delimit(v, ", ")) == "2, 3, 5, 7, 11");
}
