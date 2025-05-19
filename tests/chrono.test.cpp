#include <catch2/catch_test_macros.hpp>
#include <ferrugo/core/chrono.hpp>

#include "matchers.hpp"

using namespace ferrugo;

TEST_CASE("utc conversion", "[chrono]")
{
    REQUIRE_THAT(core::utc_time_t::get_date(0.0), matchers::equal_to(core::utc_time_t::date_type{ -4713, 11, 24 }));
    REQUIRE_THAT(core::utc_time_t::get_date(10.0), matchers::equal_to(core::utc_time_t::date_type{ -4713, 12, 4 }));
    REQUIRE_THAT(core::utc_time_t::get_date(100.0), matchers::equal_to(core::utc_time_t::date_type{ -4712, 3, 3 }));
    REQUIRE_THAT(core::utc_time_t::get_date(1'000.0), matchers::equal_to(core::utc_time_t::date_type{ -4710, 8, 20 }));
    REQUIRE_THAT(core::utc_time_t::get_date(10'000.0), matchers::equal_to(core::utc_time_t::date_type{ -4685, 4, 12 }));
    REQUIRE_THAT(core::utc_time_t::get_date(100'000.0), matchers::equal_to(core::utc_time_t::date_type{ -4439, 9, 9 }));
    REQUIRE_THAT(core::utc_time_t::get_date(1'000'000.0), matchers::equal_to(core::utc_time_t::date_type{ -1975, 10, 21 }));
    REQUIRE_THAT(core::utc_time_t::get_date(2'000'000.0), matchers::equal_to(core::utc_time_t::date_type{ 763, 9, 18 }));
    REQUIRE_THAT(core::utc_time_t::get_date(2'100'000.0), matchers::equal_to(core::utc_time_t::date_type{ 1037, 7, 3 }));
    REQUIRE_THAT(core::utc_time_t::get_date(2'200'000.0), matchers::equal_to(core::utc_time_t::date_type{ 1311, 4, 18 }));
    REQUIRE_THAT(core::utc_time_t::get_date(2'300'000.0), matchers::equal_to(core::utc_time_t::date_type{ 1585, 1, 31 }));
    REQUIRE_THAT(core::utc_time_t::get_date(2'400'000.0), matchers::equal_to(core::utc_time_t::date_type{ 1858, 11, 16 }));
    REQUIRE_THAT(core::utc_time_t::get_date(2'450'000.0), matchers::equal_to(core::utc_time_t::date_type{ 1995, 10, 9 }));
    REQUIRE_THAT(core::utc_time_t::get_date(2'460'000.0), matchers::equal_to(core::utc_time_t::date_type{ 2023, 2, 24 }));
    REQUIRE_THAT(core::utc_time_t::get_date(2'460'500.0), matchers::equal_to(core::utc_time_t::date_type{ 2024, 7, 8 }));
    REQUIRE_THAT(core::utc_time_t::get_date(2'460'550.0), matchers::equal_to(core::utc_time_t::date_type{ 2024, 8, 27 }));
    REQUIRE_THAT(core::utc_time_t::get_date(2'460'814.5), matchers::equal_to(core::utc_time_t::date_type{ 2025, 05, 19 }));
    REQUIRE_THAT(core::utc_time_t::get_date(2'460'999.0), matchers::equal_to(core::utc_time_t::date_type{ 2025, 11, 19 }));
    REQUIRE_THAT(core::utc_time_t::get_date(2'461'000.0), matchers::equal_to(core::utc_time_t::date_type{ 2025, 11, 20 }));
    REQUIRE_THAT(core::utc_time_t::get_date(2'461'500.0), matchers::equal_to(core::utc_time_t::date_type{ 2027, 4, 4 }));
    REQUIRE_THAT(core::utc_time_t::get_date(2'462'000.0), matchers::equal_to(core::utc_time_t::date_type{ 2028, 8, 16 }));
    REQUIRE_THAT(core::utc_time_t::get_date(2'500'000.0), matchers::equal_to(core::utc_time_t::date_type{ 2132, 8, 31 }));
    REQUIRE_THAT(
        core::utc_time_t::get_date(10'000'000.0), matchers::equal_to(core::utc_time_t::date_type{ 22'666, 12, 20 }));
    REQUIRE_THAT(
        core::utc_time_t::get_date(100'000'000.0), matchers::equal_to(core::utc_time_t::date_type{ 269'078, 8, 7 }));
    REQUIRE_THAT(
        core::utc_time_t::get_date(1'000'000'000.0), matchers::equal_to(core::utc_time_t::date_type{ 2'733'194, 11, 27 }));
}
