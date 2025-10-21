
#include <array>
#include <cmath>
#include <cstdint>
#include <cuchar>
#include <deque>
#include <ferrugo/core/backtrace.hpp>
#include <ferrugo/core/channel.hpp>
#include <ferrugo/core/chrono.hpp>
#include <ferrugo/core/dimensions.hpp>
#include <ferrugo/core/error_handling.hpp>
#include <ferrugo/core/functional.hpp>
#include <ferrugo/core/iterator_range.hpp>
#include <ferrugo/core/ostream_utils.hpp>
#include <ferrugo/core/overloaded.hpp>
#include <ferrugo/core/rational.hpp>
#include <ferrugo/core/sequence.hpp>
#include <ferrugo/core/std_ostream.hpp>
#include <ferrugo/core/type_name.hpp>
#include <forward_list>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <set>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "next.hpp"
#include "parsing.hpp"

int run(const std::vector<std::string_view>& args)
{
    std::string_view text = "    Ala||   \"ma| kota\"|i|psa  ";
    for (const auto& token : parsing::tokenize(text, parsing::csv('|')))
    {
        std::cout << std::quoted(token) << "\n";
    }

    std::cout << next::parse(R"([:div {:class A} [:div {:class B} "AAA"]])") << "\n";
    std::cout << ferrugo::core::backtrace() << std::endl;
    return 0;
}

int main(int argc, char* argv[])
{
    try
    {
        return run({ argv, argv + argc });
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << '\n';
        return -1;
    }
}
