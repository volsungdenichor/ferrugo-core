
#include <algorithm>
#include <array>
#include <bitset>
#include <cstdint>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
// #include <zx/surface.hpp>

struct Nesting
{
    using FareClass = char;
    using node_id_t = std::uint8_t;
    using index_t = std::uint8_t;
    using size_t = std::uint8_t;
    static constexpr node_id_t null_node = 255;
    static constexpr index_t null_index = 255;

    std::bitset<26> m_node_presence;
    std::array<index_t, 26> m_cabin_indices;
    std::array<node_id_t, 26> m_cabins;
    std::vector<std::pair<node_id_t, node_id_t>> m_edges;  // (parent, child) pairs in insertion order
    size_t m_cabins_count = 0;

    struct Node
    {
        FareClass fc;
        std::vector<Node> children;
    };

    Nesting(const std::vector<Node>& nodes = {}) : m_node_presence{}, m_cabin_indices{}, m_cabins{}
    {
        m_cabin_indices.fill(null_index);
        m_cabins.fill(null_node);
        m_cabins_count = 0;

        for (const Node& node : nodes)
        {
            add_cabin(node.fc);
        }
        for (const Node& node : nodes)
        {
            add(node);
        }
    }

    bool contains(FareClass c) const
    {
        return m_node_presence.test(symbol_to_index(c));
    }

    Nesting& add_cabin(FareClass fc)
    {
        const index_t index = symbol_to_index(fc);
        add_node(fc);
        m_cabins[index] = index;
        m_cabin_indices[index] = m_cabins_count;
        ++m_cabins_count;
        return *this;
    }

    Nesting& add_edge(FareClass parent, FareClass child)
    {
        const index_t parent_index = symbol_to_index(parent);
        const index_t child_index = symbol_to_index(child);

        add_node(parent);
        add_node(child);
        m_cabins[child_index] = m_cabins[parent_index];
        m_cabin_indices[child_index] = m_cabin_indices[parent_index];
        m_edges.emplace_back(parent_index, child_index);
        return *this;
    }

    FareClass get_root(FareClass fc) const
    {
        if (!contains(fc))
        {
            throw std::invalid_argument("Node not present in the forest");
        }
        return index_to_symbol(m_cabins[symbol_to_index(fc)]);
    }

    std::optional<FareClass> get_parent(FareClass fc) const
    {
        if (!contains(fc))
        {
            throw std::invalid_argument("Node not present in the forest");
        }
        const index_t child_index = symbol_to_index(fc);
        for (const auto& [parent, child] : m_edges)
        {
            if (child == child_index)
            {
                return index_to_symbol(parent);
            }
        }
        return std::nullopt;
    }

    std::size_t get_cabin_index(FareClass fc) const
    {
        if (!contains(fc))
        {
            throw std::invalid_argument("Node not present in the forest");
        }
        return m_cabin_indices[symbol_to_index(fc)];
    }

    template <class Func>
    Func for_each_cabin(Func func) const
    {
        for (size_t i = 0; i < this->m_cabins_count; ++i)
        {
            const auto root_index = std::find(m_cabin_indices.begin(), m_cabin_indices.end(), i) - m_cabin_indices.begin();
            std::invoke(func, index_to_symbol(root_index));
        }
        return func;
    }

    template <class Func>
    Func for_each_cabin_indexed(Func func) const
    {
        for (size_t i = 0; i < this->m_cabins_count; ++i)
        {
            const auto root_index = std::find(m_cabin_indices.begin(), m_cabin_indices.end(), i) - m_cabin_indices.begin();
            std::invoke(func, i, index_to_symbol(root_index));
        }
        return func;
    }

    template <class Func>
    Func for_each_ancestors(FareClass fc, Func func) const
    {
        if (!contains(fc))
        {
            throw std::invalid_argument("Node not present in the forest");
        }
        index_t current = symbol_to_index(fc);
        while (true)
        {
            // Find parent of current node
            index_t parent_index = null_node;
            for (const auto& [parent, child] : m_edges)
            {
                if (child == current)
                {
                    parent_index = parent;
                    break;
                }
            }
            if (parent_index == null_node)
            {
                break;
            }
            current = parent_index;
            std::invoke(func, index_to_symbol(current));
        }
        return func;
    }

    template <class Func>
    Func for_each_self_and_ancestors(FareClass fc, Func func) const
    {
        if (!contains(fc))
        {
            throw std::invalid_argument("Node not present in the forest");
        }
        index_t current = symbol_to_index(fc);
        while (true)
        {
            std::invoke(func, index_to_symbol(current));
            // Find parent of current node
            index_t parent_index = null_node;
            for (const auto& [parent, child] : m_edges)
            {
                if (child == current)
                {
                    parent_index = parent;
                    break;
                }
            }
            if (parent_index == null_node)
            {
                break;
            }
            current = parent_index;
        }
        return func;
    }

    template <class Func>
    Func for_each_child(FareClass fc, Func func) const
    {
        if (!contains(fc))
        {
            throw std::invalid_argument("Node not present in the forest");
        }
        const index_t parent_index = symbol_to_index(fc);
        for (const auto& [parent, child] : m_edges)
        {
            if (parent == parent_index)
            {
                std::invoke(func, index_to_symbol(child));
            }
        }
        return func;
    }

    template <class Func>
    Func for_each_self_and_descendants(FareClass fc, Func func) const
    {
        if (!contains(fc))
        {
            throw std::invalid_argument("Node not present in the forest");
        }
        std::vector<FareClass> current_level{ fc };
        while (!current_level.empty())
        {
            std::vector<FareClass> next_level;
            for (FareClass node : current_level)
            {
                std::invoke(func, node);
                for_each_child(node, [&](FareClass child) { next_level.push_back(child); });
            }
            current_level = std::move(next_level);
        }
        return func;
    }

    friend std::ostream& operator<<(std::ostream& os, const Nesting& item)
    {
        bool first = true;
        item.for_each_cabin(
            [&](FareClass fc)
            {
                if (!first)
                {
                    os << " ";
                }
                first = false;
                item.for_each_self_and_descendants(fc, [&](FareClass descendant) { os << descendant; });
            });
        return os;
    }

private:
    void add_node(FareClass c)
    {
        m_node_presence.set(symbol_to_index(c));
    }

    void add(const Node& node)
    {
        for (const Node& child : node.children)
        {
            add_edge(node.fc, child.fc);
            add(child);
        }
    }

    static index_t symbol_to_index(FareClass fc)
    {
        if (!('A' <= fc && fc <= 'Z'))
        {
            throw std::invalid_argument("Invalid character");
        }
        return static_cast<index_t>(fc - 'A');
    }

    static FareClass index_to_symbol(index_t index)
    {
        if (index >= 26)
        {
            throw std::invalid_argument("Invalid index");
        }
        return static_cast<FareClass>(index + 'A');
    }
};

struct PrettyPrinter
{
    const Nesting& m_nesting;

    std::vector<Nesting::FareClass> get_cabins() const
    {
        std::vector<Nesting::FareClass> result;
        m_nesting.for_each_cabin([&](Nesting::FareClass fc) { result.push_back(fc); });
        return result;
    }

    std::vector<Nesting::FareClass> get_children(Nesting::FareClass fc) const
    {
        std::vector<Nesting::FareClass> result;
        m_nesting.for_each_child(fc, [&](Nesting::FareClass child) { result.push_back(child); });
        return result;
    }

    void print(std::ostream& os, Nesting::FareClass node, const std::string& prefix, bool is_last, bool is_root) const
    {
        os << prefix;
        if (!is_root)
        {
            os << (is_last ? "+- " : "|- ");
        }
        os << node << '\n';

        const std::vector<Nesting::FareClass> children = get_children(node);

        const std::string child_prefix = prefix + (is_root ? "" : (is_last ? "   " : "|  "));
        for (std::size_t i = 0; i < children.size(); ++i)
        {
            print(os, children[i], child_prefix, i + 1 == children.size(), false);
        }
    }

    void print(std::ostream& os) const
    {
        std::vector<Nesting::FareClass> cabins = get_cabins();

        for (std::size_t i = 0; i < cabins.size(); ++i)
        {
            print(os, cabins[i], "", i + 1 == cabins.size(), true);
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const PrettyPrinter& item)
    {
        item.print(os);
        return os;
    }
};

void run(const std::vector<std::string_view>& args)
{
    std::cout << sizeof(Nesting) << std::endl;
    const Nesting nesting({ { 'E', { { 'F' }, { 'G', { { 'H' }, { 'I' }, { 'J' } } } } },
                            { 'A', { { 'B' }, { 'C' }, { 'D' } } },
                            { 'K', { { 'L' }, { 'M', { { 'O' }, { 'P', { { 'Q' } } } } }, { 'N' } } } });

    std::cout << PrettyPrinter{ nesting } << std::endl;

    nesting.for_each_cabin_indexed([](std::size_t index, Nesting::FareClass fc)
                                   { std::cout << "Cabin index: " << index << " root: " << fc << '\n'; });

    nesting.for_each_self_and_descendants(
        'K',
        [&](Nesting::FareClass fc)
        {
            const auto p = nesting.get_parent(fc);
            std::cout << fc << " " << (p ? std::string(1, *p) : "null") << " " << nesting.get_cabin_index(fc) << " [";
            nesting.for_each_ancestors(fc, [&](Nesting::FareClass ancestor) { std::cout << ancestor; });
            std::cout << "]\n";
        });
}

int main(int argc, char* argv[])
{
    try
    {
        run({ argv, argv + argc });
        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << ex.what() << '\n';
        return -1;
    }
}
