
#include <array>
#include <cmath>
#include <cstdint>
#include <cuchar>
#include <deque>
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
#include <ferrugo/core/string.hpp>
#include <ferrugo/core/type_name.hpp>
#include <ferrugo/core/zx.hpp>
#include <forward_list>
#include <fstream>
#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <set>
#include <typeindex>
#include <unordered_set>
#include <variant>
#include <vector>

void run()
{
    const auto x = std::string{ "👍" } + std::string{ "Οὐχὶ ταὐτὰ παρίσταταί μοι γιγνώσκειν, ὦ ἄνδρες ᾿Αθηναῖοι," }
                   + std::string{ "💚" };
    for (auto g : ferrugo::core::glyph::to_glyphs(x).take(4))
    {
        std::cout << g << "\n";
    }
}

int main(int argc, char* argv[])
{
    const std::vector<std::string> args = zx::span<char*>{ argv, argc };
    for (const auto a : args)
    {
        std::cout << a << "\n";
    }
    if (const auto result = zx::try_invoke(&run); result.has_error())
    {
        zx::format_to(std::cerr, result.error(), "\n");
    }
}
