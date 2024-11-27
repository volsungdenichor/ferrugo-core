#include <algorithm>
#include <array>
#include <boost/variant.hpp>
#include <cmath>
#include <ferrugo/core/either.hpp>
#include <ferrugo/core/error_handling.hpp>
#include <ferrugo/core/maybe.hpp>
#include <ferrugo/core/ostream_utils.hpp>
#include <ferrugo/core/source_location.hpp>
#include <ferrugo/core/std_ostream.hpp>
#include <ferrugo/core/str_t.hpp>
#include <functional>
#include <iostream>
#include <optional>
#include <random>
#include <sstream>
#include <variant>
#include <vector>

using seed_t = std::random_device::result_type;

class permutation_t
{
public:
    permutation_t(std::vector<int> values) : m_values{ std::move(values) }
    {
        if (m_values.size() != 256)
        {
            throw std::runtime_error{ "permutation_t: expected 256 values" };
        }
    }

    permutation_t(const std::optional<seed_t>& seed = {}) : permutation_t{ generate(seed) }
    {
    }

    permutation_t(const permutation_t&) = default;
    permutation_t(permutation_t&&) = default;

    int operator[](size_t index) const
    {
        return m_values[index % 256];
    }

private:
    static std::vector<int> generate(const std::optional<seed_t>& seed)
    {
        static auto rd = std::random_device{};
        auto gen = std::default_random_engine{ seed ? *seed : rd() };
        std::vector<int> result(256);
        std::iota(std::begin(result), std::end(result), 0);
        std::shuffle(std::begin(result), std::end(result), gen);
        return result;
    }

    std::vector<int> m_values;
};

class perlin_noise_base
{
public:
    perlin_noise_base(permutation_t permutation) : m_permutation{ std::move(permutation) }
    {
    }

    float operator()(const std::array<float, 3>& location) const
    {
        const auto pos = std::array<int, 3>{ floor(location[0]), floor(location[1]), floor(location[2]) };

        const auto rel_pos = std::array<float, 3>{ fractional_part(location[0]),
                                                   fractional_part(location[1]),
                                                   fractional_part(location[2]) };

        const auto p = std::array<float, 3>{ fade(rel_pos[0]), fade(rel_pos[1]), fade(rel_pos[2]) };

        const auto v = std::array<int, 2>{ m_permutation[pos[0] + 0] + pos[1], m_permutation[pos[0] + 1] + pos[1] };
        const auto vert = std::array<int, 4>{ m_permutation[v[0] + 0] + pos[2],
                                              m_permutation[v[1] + 0] + pos[2],
                                              m_permutation[v[0] + 1] + pos[2],
                                              m_permutation[v[1] + 1] + pos[2] };

        const auto level_0 = [&](int a, int b) -> float
        {
            static const std::array<std::array<int, 3>, 8> translations = { {
                { 0, 0, 0 },  //
                { 1, 0, 0 },  //
                { 0, 1, 0 },  //
                { 1, 1, 0 },  //
                { 0, 0, 1 },  //
                { 1, 0, 1 },  //
                { 0, 1, 1 },  //
                { 1, 1, 1 },  //
            } };

            a *= 2;
            const int c = 4 * b + a;
            return lerp(
                p[0],
                grad(
                    m_permutation[vert[a + 0] + b],
                    rel_pos[0] - translations[c + 0][0],
                    rel_pos[1] - translations[c + 0][1],
                    rel_pos[2] - translations[c + 0][2]),
                grad(
                    m_permutation[vert[a + 1] + b],
                    rel_pos[0] - translations[c + 1][0],
                    rel_pos[1] - translations[c + 1][1],
                    rel_pos[2] - translations[c + 1][2]));
        };

        const auto level_1 = [&](int a) -> float { return lerp(p[1], level_0(0, a), level_0(1, a)); };

        return lerp(p[2], level_1(0), level_1(1));
    }

private:
    static float fade(float v)
    {
        return 6 * std::pow(v, 5) - 15 * std::pow(v, 4) + 10 * std::pow(v, 3);
    }

    static int floor(float v)
    {
        return (int)std::floor(v);
    }

    static float fractional_part(float v)
    {
        return v - std::floor(v);
    }

    static float lerp(float r, float a, float b)
    {
        return ((1.F - r) * a) + (r * b);
    }

    static float grad(int hash, float x, float y, float z)
    {
        const int h = hash & 0xF;

        const float u = h < 0x08 ? x : y;
        const float v = h < 0x04 ? y : h == 0x0c || h == 0x0e ? x : z;

        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }

    permutation_t m_permutation;
};

class perlin_noise : perlin_noise_base
{
public:
    struct settings_t
    {
        int octaves = 1;
        float persistence = 1.F;
        float frequency = 1.F;
    };

    using base_t = perlin_noise_base;

    perlin_noise(settings_t settings, permutation_t permutation = {})
        : base_t{ std::move(permutation) }
        , m_settings{ std::move(settings) }
    {
    }

    float operator()(const std::array<float, 3>& location) const
    {
        float sum = 0.F;
        float amplitude = 1.F;
        float f = m_settings.frequency;

        for (std::size_t i = 0; i < m_settings.octaves; ++i)
        {
            const auto loc = std::array<float, 3>{ f * location[0], f * location[1], f * location[2] };
            sum += base_t::operator()(loc) * amplitude;
            amplitude *= m_settings.persistence;
            f *= 2.F;
        }

        const float max_value = amplitude * m_settings.octaves;

        return sum / max_value;
    }

    settings_t m_settings;
};

struct monostate
{
};

template <int V>
const int& val()
{
    static const int val = V;
    return val;
}

void run()
{
    using namespace ferrugo::core;
    maybe<std::tuple<const int, const int>> a = std::make_tuple(22, 333);
    const auto b = a.transform([](const auto& v) { return std::get<0>(v) + std::get<1>(v); })
                       .filter([](int x) { return x < 0; })
                       .transform(str);
    std::cout << debug(b) << "\n";

    int x = 10;
    maybe<const int&> ref = x;
    ref = val<100>();
    std::cout << debug(ref) << "\n";
    std::cout << x << "\n";
}

int main()
{
    // const perlin_noise perlin{ { 4, 0.2F, 0.1F }, permutation_t{ seed_t{ 10 } } };
    // std::cout << perlin(std::array<float, 3>{ { 0.F, 0.20F, 0.F } }) << std::endl;
    // std::cout << perlin(std::array<float, 3>{ { 0.F, 0.21F, 0.F } }) << std::endl;
    try
    {
        run();
    }
    catch (...)
    {
        std::cerr << "\n"
                  << "Error:"
                  << "\n"
                  << ferrugo::core::exception_proxy{};
    }
}
