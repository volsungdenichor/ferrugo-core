
#include <array>
#include <cmath>
#include <cstdint>
#include <cuchar>
#include <deque>
#include <ferrugo/core/channel.hpp>
#include <ferrugo/core/chrono.hpp>
#include <ferrugo/core/core.hpp>
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

void print(ferrugo::core::string_view txt)
{
    std::cout << "'" << txt << "'"
              << "\n";
    std::cout << ferrugo::core::string{ txt } << "\n";
    std::cout << txt.get(-1) << "\n";
}

void run()
{
    print(
        ferrugo::core::glyph{ "👍" } + ferrugo::core::string{ "Οὐχὶ ταὐτὰ παρίσταταί μοι γιγνώσκειν, ὦ ἄνδρες ᾿Αθηναῖοι," }
        + ferrugo::core::glyph{ "💚" });
}

int main()
{
    if (const auto result = core::try_invoke(&run); result.has_error())
    {
        core::format_to(std::cerr, result.error(), "\n");
    }
}
