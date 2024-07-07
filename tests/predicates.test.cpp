#include <catch2/catch_test_macros.hpp>
#include <ferrugo/core/ostream_utils.hpp>
#include <ferrugo/core/predicates.hpp>

#include "matchers.hpp"

using namespace ferrugo;
using namespace std::string_view_literals;

auto divisible_by(int divisor)
{
    return [=](int v) { return v % divisor == 0; };
}

TEST_CASE("predicates - format", "")
{
    REQUIRE_THAT(  //
        (core::str(core::all(core::ge(0), core::lt(5)))),
        matchers::equal_to("(all (ge 0) (lt 5))"sv));
    REQUIRE_THAT(  //
        (core::str(core::any(1, 2, 3, core::ge(100)))),
        matchers::equal_to("(any 1 2 3 (ge 100))"sv));
    REQUIRE_THAT(  //
        (core::str(core::negate(core::any(1, 2, 3)))),
        matchers::equal_to("(not (any 1 2 3))"sv));
    REQUIRE_THAT(  //
        (core::str(core::each(core::any(1, 2, 3)))),
        matchers::equal_to("(each (any 1 2 3))"sv));
    REQUIRE_THAT(  //
        (core::str(core::contains(core::any(1, 2, 3)))),
        matchers::equal_to("(contains (any 1 2 3))"sv));
    REQUIRE_THAT(  //
        (core::str(core::size_is(core::lt(8)))),
        matchers::equal_to("(size_is (lt 8))"sv));
}

TEST_CASE("predicates - eq", "")
{
    const auto pred = core::eq(10);
    REQUIRE_THAT(  //
        (core::str(pred)),
        matchers::equal_to("(eq 10)"sv));
    REQUIRE_THAT(pred(10), matchers::equal_to(true));
    REQUIRE_THAT(pred(5), matchers::equal_to(false));
    REQUIRE_THAT(pred(15), matchers::equal_to(false));
}

TEST_CASE("predicates - ne", "")
{
    const auto pred = core::ne(10);
    REQUIRE_THAT(  //
        (core::str(pred)),
        matchers::equal_to("(ne 10)"sv));
    REQUIRE_THAT(pred(10), matchers::equal_to(false));
    REQUIRE_THAT(pred(5), matchers::equal_to(true));
    REQUIRE_THAT(pred(15), matchers::equal_to(true));
}

TEST_CASE("predicates - lt", "")
{
    const auto pred = core::lt(10);
    REQUIRE_THAT(  //
        (core::str(pred)),
        matchers::equal_to("(lt 10)"sv));
    REQUIRE_THAT(pred(10), matchers::equal_to(false));
    REQUIRE_THAT(pred(5), matchers::equal_to(true));
    REQUIRE_THAT(pred(15), matchers::equal_to(false));
}

TEST_CASE("predicates - gt", "")
{
    const auto pred = core::gt(10);
    REQUIRE_THAT(  //
        (core::str(pred)),
        matchers::equal_to("(gt 10)"sv));
    REQUIRE_THAT(pred(10), matchers::equal_to(false));
    REQUIRE_THAT(pred(5), matchers::equal_to(false));
    REQUIRE_THAT(pred(15), matchers::equal_to(true));
}

TEST_CASE("predicates - le", "")
{
    const auto pred = core::le(10);
    REQUIRE_THAT(  //
        (core::str(pred)),
        matchers::equal_to("(le 10)"sv));
    REQUIRE_THAT(pred(10), matchers::equal_to(true));
    REQUIRE_THAT(pred(5), matchers::equal_to(true));
    REQUIRE_THAT(pred(15), matchers::equal_to(false));
}

TEST_CASE("predicates - ge", "")
{
    const auto pred = core::ge(10);
    REQUIRE_THAT(  //
        (core::str(pred)),
        matchers::equal_to("(ge 10)"sv));
    REQUIRE_THAT(pred(10), matchers::equal_to(true));
    REQUIRE_THAT(pred(5), matchers::equal_to(false));
    REQUIRE_THAT(pred(15), matchers::equal_to(true));
}

TEST_CASE("predicates - all", "")
{
    const auto pred = core::all(core::ge(10), core::lt(20), divisible_by(3));
    REQUIRE_THAT(  //
        (core::str(pred)),
        matchers::equal_to("(all (ge 10) (lt 20) divisible_by(int)::{lambda(int)#1})"sv));
    REQUIRE_THAT(pred(9), matchers::equal_to(false));
    REQUIRE_THAT(pred(10), matchers::equal_to(false));
    REQUIRE_THAT(pred(11), matchers::equal_to(false));
    REQUIRE_THAT(pred(12), matchers::equal_to(true));
    REQUIRE_THAT(pred(13), matchers::equal_to(false));
    REQUIRE_THAT(pred(14), matchers::equal_to(false));
    REQUIRE_THAT(pred(15), matchers::equal_to(true));
    REQUIRE_THAT(pred(16), matchers::equal_to(false));
    REQUIRE_THAT(pred(17), matchers::equal_to(false));
    REQUIRE_THAT(pred(18), matchers::equal_to(true));
    REQUIRE_THAT(pred(19), matchers::equal_to(false));
    REQUIRE_THAT(pred(20), matchers::equal_to(false));
    REQUIRE_THAT(pred(21), matchers::equal_to(false));
}

TEST_CASE("predicates - any", "")
{
    const auto pred = core::any(divisible_by(5), divisible_by(3), 100);
    REQUIRE_THAT(  //
        (core::str(pred)),
        matchers::equal_to("(any divisible_by(int)::{lambda(int)#1} divisible_by(int)::{lambda(int)#1} 100)"sv));
    REQUIRE_THAT(pred(9), matchers::equal_to(true));
    REQUIRE_THAT(pred(10), matchers::equal_to(true));
    REQUIRE_THAT(pred(11), matchers::equal_to(false));
    REQUIRE_THAT(pred(12), matchers::equal_to(true));
    REQUIRE_THAT(pred(13), matchers::equal_to(false));
    REQUIRE_THAT(pred(14), matchers::equal_to(false));
    REQUIRE_THAT(pred(15), matchers::equal_to(true));
    REQUIRE_THAT(pred(16), matchers::equal_to(false));
    REQUIRE_THAT(pred(17), matchers::equal_to(false));
    REQUIRE_THAT(pred(18), matchers::equal_to(true));
    REQUIRE_THAT(pred(19), matchers::equal_to(false));
    REQUIRE_THAT(pred(20), matchers::equal_to(true));
    REQUIRE_THAT(pred(21), matchers::equal_to(true));
    REQUIRE_THAT(pred(100), matchers::equal_to(true));
}

TEST_CASE("predicates - negate", "")
{
    const auto pred = core::negate(core::all(core::ge(0), core::lt(5)));
    REQUIRE_THAT(  //
        (core::str(pred)),
        matchers::equal_to("(not (all (ge 0) (lt 5)))"sv));
    REQUIRE_THAT(pred(-1), matchers::equal_to(true));
    REQUIRE_THAT(pred(0), matchers::equal_to(false));
    REQUIRE_THAT(pred(1), matchers::equal_to(false));
    REQUIRE_THAT(pred(2), matchers::equal_to(false));
    REQUIRE_THAT(pred(3), matchers::equal_to(false));
    REQUIRE_THAT(pred(4), matchers::equal_to(false));
    REQUIRE_THAT(pred(5), matchers::equal_to(true));
}

TEST_CASE("predicates - is_empty", "")
{
    const auto pred = core::is_empty();
    REQUIRE_THAT(  //
        (core::str(pred)),
        matchers::equal_to("(is_empty)"sv));
    REQUIRE_THAT(pred(""sv), matchers::equal_to(true));
    REQUIRE_THAT(pred("###"sv), matchers::equal_to(false));
}

TEST_CASE("predicates - size_is", "")
{
    const auto pred = core::size_is(core::lt(3));
    REQUIRE_THAT(  //
        (core::str(pred)),
        matchers::equal_to("(size_is (lt 3))"sv));
    REQUIRE_THAT(pred(""sv), matchers::equal_to(true));
    REQUIRE_THAT(pred("#"sv), matchers::equal_to(true));
    REQUIRE_THAT(pred("##"sv), matchers::equal_to(true));
    REQUIRE_THAT(pred("###"sv), matchers::equal_to(false));
}

TEST_CASE("predicates - each", "")
{
    const auto pred = core::each('#');
    REQUIRE_THAT(  //
        (core::str(pred)),
        matchers::equal_to("(each #)"sv));
    REQUIRE_THAT(pred(""sv), matchers::equal_to(true));
    REQUIRE_THAT(pred("#"sv), matchers::equal_to(true));
    REQUIRE_THAT(pred("##"sv), matchers::equal_to(true));
    REQUIRE_THAT(pred("##__"sv), matchers::equal_to(false));
}

TEST_CASE("predicates - contains", "")
{
    const auto pred = core::contains('#');
    REQUIRE_THAT(  //
        (core::str(pred)),
        matchers::equal_to("(contains #)"sv));
    REQUIRE_THAT(pred(""sv), matchers::equal_to(false));
    REQUIRE_THAT(pred("#"sv), matchers::equal_to(true));
    REQUIRE_THAT(pred("##"sv), matchers::equal_to(true));
    REQUIRE_THAT(pred("__"sv), matchers::equal_to(false));
}