#ifndef TRIE_H
#define TRIE_H

#include <vector>
#include <deque>
#include <map>
#include <iterator>
#include <utility>

#include <boost/container/flat_map.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace trie
{

struct SetCounter { };

namespace detail
{
/**
 * @brief Compact and simple, but slow implementation of Trie node
 * with binary lookups.
 *
 * Boost flat_map is used to store pointers
 */
template <typename AtomT, typename PrefixHolderT>
struct TrieNodeMapped : public PrefixHolderT
{
private:
    typedef TrieNodeMapped<AtomT, PrefixHolderT> self_type;
    typedef boost::container::flat_map<AtomT, self_type *> edge_map_t;
    edge_map_t  map;
public:
    typedef typename edge_map_t::const_iterator map_iterator;

    static self_type * value(map_iterator x) { return x->second; };

    TrieNodeMapped(int hint) : map(hint) {}

    self_type * get(const AtomT & x) const
    {
        typename edge_map_t::const_iterator it = map.find(x);
        return it == map.end() ? nullptr : it->second;
    }

    void put(self_type * edge)
    {
        map[*edge->kbegin()] = edge;
    }

    map_iterator begin() const { return map.begin(); }
    map_iterator end()   const { return map.end();   }

    void split(self_type * next, int breakIdx)
    {
        this->PrefixHolderT::psplit(next, breakIdx);
        std::swap(this->map, next->map);
        this->swap_value(*next);
        put(next);
    }
};

/**
 * How to get a cell to put Atom to in case it's not char/other integer type
 */
template<typename AtomT>
int atom_hash(AtomT x, int bucket_size) {
    return (x % bucket_size);
}

template<typename AtomT>
int least_uncolliding_size(AtomT a, AtomT b)
{
    unsigned int v = (a ^ b);
    return (v & -v) << 1;
}

/*
static const uint16_t _byte_bit_position[11] 
    = {0, 1, 2, 0, 3, 5, 0, 8, 4, 7, 6 };
*/

/**
 * Given to alphabet characters, 
 */
template<>
int least_uncolliding_size(char a, char b) 
{
    unsigned int v = ((int) a ^ (int) b);
    return (v & -v) << 1;

/*    return _byte_bit_position[(v & ~v) % 11]; */
}

/**
 * @brief A little more sofisticated implementation of node,
 * but should be faster on lookups. Also it may not maintain order.
 */
template <typename AtomT, typename PrefixHolderT>
struct TrieNodeHashed : public PrefixHolderT
{
private:
    typedef TrieNodeHashed<AtomT, PrefixHolderT> self_type;
    typedef self_type * self_pointer;

    self_pointer * data = nullptr;
    uint32_t size = 0; /* Will still be 64 bit due to alignment */

public:
    typedef self_type ** map_iterator;

    TrieNodeHashed(int hint) { resize(hint); }

    static self_type * value(map_iterator x) { return *x; };

    void resize(uint32_t new_size)
    {
        self_pointer * ndata = (new_size == 0) ?
            nullptr : (new self_pointer[new_size]);

        std::fill(ndata, ndata + new_size, nullptr);

        if (new_size > size) {
            for (uint32_t i = 0; i < size; ++i) {
                if (data[i] != nullptr) {
                    ndata[atom_hash(*data[i]->kbegin(), new_size)] = data[i];
                }
            }
        }

        delete[] data;
        data = ndata;
        size = new_size;
    }

    self_type * get(AtomT x) const
    {
        if (data == nullptr) { return nullptr; }
        int hash = atom_hash(x, size);
        self_type * result = data[hash];

        if (nullptr != result and (*result->kbegin() == x)) {
            return result;
        }
        return nullptr;
    }

    ~TrieNodeHashed() { resize(0); };

    void put(self_type * edge)
    {
        AtomT x = *edge->kbegin();

        if (size == 0) { resize(2); }

        int hash = atom_hash(x, size);

        if (data[hash] == nullptr) {
            data[hash] = edge;
            return;
        }

        resize(least_uncolliding_size(x, *data[hash]->kbegin()));

        data[atom_hash(x, size)] = edge;
    }

    map_iterator begin() const { return data; }
    map_iterator end()   const { return data + size; }

    void split(self_type * next, int breakIdx)
    {
        this->PrefixHolderT::psplit(next, breakIdx);
        std::swap(this->data, next->data);
        std::swap(this->size, next->size);
        this->swap_value(*next);
        put(next);
    }
};

template <typename AtomT, typename NodeT>
struct TrieIteratorInternal
{
    typedef std::vector< AtomT >          key_type;
    typedef typename NodeT::map_iterator  traverse_ptr;

    key_type base_prefix;
    const NodeT * m_root;
    std::vector<traverse_ptr> m_ptrs;

    TrieIteratorInternal(const NodeT * a_root) : m_root(a_root) { };

    const NodeT * get(int i = 0)
    {
        int j = (int) m_ptrs.size() + i;
        return j > 0 ? NodeT::value(m_ptrs[j - 1]) : (j == 0 ? m_root : nullptr);
    }

    key_type get_key()
    {
        key_type result(base_prefix);

        const NodeT * i = m_root;
        result.insert(result.back(), i->kbegin(), i->p.kend());

        for (auto && traverse_ptr : m_ptrs)
        {
            i = traverse_ptr->second;
            result.insert(result.back(), i->p.kbegin(), i->p.kend());
        }

        return result;
    }

    bool step_down()
    {
        const NodeT * x = get();
        traverse_ptr it = x->begin();

        while (it != x->end()
                and NodeT::value(it) == nullptr) { 
            ++it; 
        }

        if (it != x->end()) 
        {
            m_ptrs.push_back(it);
            return true;
        }

        return false;
    }

    bool step_fore()
    {
        const NodeT * up = get(-1);

        if (up != nullptr)
        {
            do {
                ++m_ptrs.back();
            } while (m_ptrs.back() != up->end()
                and NodeT::value(m_ptrs.back()) == nullptr);

            return m_ptrs.back() != up->end();
        }

        return false;
    }

    bool step_up()
    {
        m_ptrs.pop_back();

        return step_fore();
    }

    void next()
    {
        if (step_down())   { return; }
        if (step_fore())   { return; }

        while (!m_ptrs.empty()) {
            if (step_up()) { return; }
        }

        m_root = nullptr;
    }

    void find_next_value()
    {
        while (m_root != nullptr and !get(0)->has_value()) {
            next();
        }
    }
};

typedef uint32_t trie_offset_t;

template <typename ValueT>
struct ValueHolder
{
private:
    std::auto_ptr<ValueT> value;
public:
    typedef ValueT value_type; /* Effective type */

    value_type & get_value()             { return *value; };
    const value_type & get_value() const { return *value; };

    bool     has_value() const noexcept  { return value.get() != nullptr; };
    void     set_value(const ValueT & x) { value.reset(new ValueT(x)); };
    void     clr_value()                 { set_value(nullptr); };

    void swap_value(ValueHolder & other) { std::swap(this->value, other.value); };
};

template <>
struct ValueHolder<SetCounter>
{
private:
    int count = 0;
public:
    typedef int value_type; /* Effective type */

    value_type & get_value() noexcept { return count; };
    const value_type & get_value() const noexcept { return count; };

    bool     has_value() const noexcept      { return count != 0; };
    void     set_value(const value_type & x) { count = x; };
    void     clr_value()                     { count = 0; };

    void swap_value(ValueHolder & other) { std::swap(count, other.count); };
};


template <typename AtomT, typename ValueT, size_t CMinChunkSize>
struct PrefixHolder : public ValueHolder<ValueT>
{
private:
    typedef PrefixHolder<AtomT, ValueT, CMinChunkSize> self_type;
    typedef std::vector<AtomT> ChunkT;

    ChunkT * chunk;
    trie_offset_t begin, end;
public:
    typedef const AtomT * key_iterator;

    key_iterator kbegin() const { return std::addressof((*chunk)[begin]); };
    key_iterator kend()   const { return std::addressof((*chunk)[end]); };

    ChunkT * insertion_hint() { return chunk; }

    void setkey(ChunkT * achunk, trie_offset_t k, trie_offset_t kend)
    {
        chunk = achunk;
        begin = k;
        end   = kend;
    }

    void psplit(self_type * next, int breakIdx)
    {
        next->chunk = chunk;
        next->begin = this->begin + breakIdx;
        next->end   = this->end;
        this->end   -= breakIdx;
    }
};


template <typename AtomT, typename ValueT>
struct PrefixHolder<AtomT, ValueT, 0> : public ValueHolder<ValueT>
{
private:
    typedef PrefixHolder<AtomT, ValueT, 0> self_type;
    typedef std::vector<AtomT>     ChunkT;

    AtomT  * prefix;
    size_t prefix_len;
    ValueT * value = nullptr;
public:
    typedef const AtomT * key_iterator;

    key_iterator kbegin() const { return prefix; };
    key_iterator kend()   const { return prefix + prefix_len; };

    std::nullptr_t insertion_hint() { return nullptr; }

    void setkey(ChunkT * achunk, trie_offset_t k, trie_offset_t kend)
    {
        prefix     = std::addressof(achunk->begin()[k]);
        prefix_len = kend - k;
    }

    void psplit(self_type * next, int breakIdx)
    {
        next->prefix     = this->prefix + breakIdx;
        next->prefix_len = this->prefix_len - breakIdx;
        this->prefix_len -= next->prefix_len;
    }
};

template<typename AtomT, typename ValueT, size_t CMinChunkSize, 
    typename Spec = void>
struct TrieNodeSelector
{
    typedef PrefixHolder<AtomT, ValueT, CMinChunkSize>  PrefixHolderType;
    typedef TrieNodeMapped<AtomT, PrefixHolderType>     type;
};

template<typename AtomT, typename ValueT, size_t CMinChunkSize>
struct TrieNodeSelector<AtomT, ValueT, CMinChunkSize, 
    typename std::enable_if<std::is_integral<AtomT>::value>::type>
{
    typedef PrefixHolder<char, ValueT, CMinChunkSize>  PrefixHolderType;
    typedef TrieNodeHashed<AtomT, PrefixHolderType>    type;
};

};

template<typename Output>
struct OnExactMatch
{
    Output & match;
    explicit OnExactMatch(Output & amatch) : match(amatch) { };
    void operator ()() { match(); };
};

template <typename AtomT, typename ValueT, size_t CMinChunkSize = 512, 
    typename NodeImpl = typename detail::TrieNodeSelector<AtomT, ValueT, CMinChunkSize>::type >
struct trie_map
{
private:
    typedef NodeImpl NodeT;
    typedef detail::TrieIteratorInternal<AtomT, NodeT> IteratorInternalT;

    /* We use deque in order to make edge storage stable */
    typedef std::deque< NodeT >                        EdgeStorageT;
public:
    typedef typename NodeImpl::value_type          value_type;
    typedef typename IteratorInternalT::key_type   key_type;
    typedef typename NodeImpl::key_iterator        key_iterator;

    typedef value_type mapped_type; /* Defined for the compatibility with map */
private:
    typedef std::vector<AtomT> ChunkT;
    typedef std::deque<ChunkT> InternalStorageT;

    /* The number of elements */
    size_t msize = 0;

    InternalStorageT keys;

    template<typename KeyIterator>
    void insert_infix(KeyIterator it, KeyIterator end, NodeT * parent, NodeT * edge)
    {
        auto size = std::distance(it, end);
        ChunkT * target = parent == nullptr ? nullptr : parent->insertion_hint();

        if ((target == nullptr) or target->size() >= CMinChunkSize)
        {
            keys.emplace_back();
            keys.back().reserve(CMinChunkSize);
            target = std::addressof(keys.back());
        }

        /* WARNING : Here, we rely on the fact, that vector pointers always 
         * remain stable if values inserted fit into reserved space, which 
         * should work in practice, but std::vector specification
         * does not guarantee that.
         */
        detail::trie_offset_t kidx = std::distance(target->begin(), target->end());
        target->insert(target->end(), it, end);
        detail::trie_offset_t kendidx = std::distance(target->begin(), target->end());

        edge->setkey(target, kidx, kendidx);
    }

    EdgeStorageT edges;

    NodeT * new_edge(int hint)
    {
        edges.emplace_back(hint);
        return std::addressof(edges.back());
    }

    template<typename KeyIterator>
    NodeT * insert_edge(NodeT * parent, KeyIterator it, KeyIterator end, const ValueT & value)
    {
        NodeT * edge = new_edge(0);
        insert_infix(it, end, parent, edge);
        if (parent != nullptr) { parent->put(edge); }
        edge->set_value(value);
        return edge;
    }

    template<typename ReplacePolicy>
    void insert_value(NodeT & at, const ValueT & value, const ReplacePolicy & replace)
    {
        if (at.has_value()) {
            replace(at.get_value(), value);
        } else {
            at.set_value(value);
        }
    }

public:

#define NO_TRIE_ITERATOR
#ifndef NO_TRIE_ITERATOR
    /** @brief The iterator with very unfair behaviour
     * 
     *  @warning: This operator is not copiable in regular sense.
     *    In order to get a fixed copy of the iterator, 
     *    use explicit clone() method!
     */
    struct iterator : public std::forward_iterator_tag
    {
    private:
        std::shared_ptr<TrieIteratorInternal> _impl;

        iterator() { };

        explicit iterator(std::shared_ptr<TrieIteratorInternal> a_impl)
            : _impl(a_impl) { };
    public:
/*
        value_type & value() {
            return _impl->value;
        }
*/
        iterator & operator ++() {
            return *this;
        }

        /**
         * Returns a "real" copy of the iterator, may be a heavy operation
         */
        iterator clone() {
            return _impl.get() == nullptr ?
                iterator() : iterator(new TrieIteratorInternal(*_impl));
        }

        /**
         * @warning: when compared to the iterator of the other 
         * collection, may return true.
         */
        bool operator == (iterator && other)
        {
            return _impl.get() == other._impl.get()
                || _impl.m_edge->back() == other._impl->m_edge.back();
        }
    };

#endif /* NO_TRIE_ITERATOR */

    /**
     * Generalized lookup algorithm.
     */
    template<typename KeyIterator,
             typename A, typename B, typename C, typename D>
    void general_search
    (
        KeyIterator it, KeyIterator end,
        A exactMatchAction,
        B noNextEdgeAction,
        C endInTheMiddleAction,
        D splitInTheMiddleAction
    )
    {
        NodeT * edge = edges.empty() ? nullptr : std::addressof(*edges.begin());
        key_iterator kbegin = edge->kbegin();

        while (edge != nullptr)
        {
            key_iterator kend   = edge->kend();
            key_iterator k      = kbegin;

            while ((it != end) and (k != kend) and (*k == *it))
                { ++k; ++it; }

            if (it == end)
            {
                if (k == kend) {
                    exactMatchAction(edge);
                } else {
                    endInTheMiddleAction(edge, k);
                }

                return;
            }
            else if (k != kend)
            {
                splitInTheMiddleAction(edge, k, it);
                return;
            }

            auto next_edge = edge->get(*it);

            if (next_edge == nullptr)
            {
                noNextEdgeAction(edge, it);
                return;
            }

            edge = next_edge;
            kbegin = edge->kbegin() + 1;
            ++it; /* Already found the first character */
        }
    }

    template<typename KeyIterator, typename ReplacePolicy>
    void insert(KeyIterator it, KeyIterator end, const ValueT & value,
                    const ReplacePolicy & replace)
    {
        if (edges.empty())
        {
            insert_edge(nullptr, it, end, value);
            ++msize;
            return;
        }

        general_search(it, end,
            [this, &value, &replace] (NodeT * edge) {
                insert_value(*edge, value, replace);
            },

            [this, &value, end] (NodeT * edge, KeyIterator kit) {
                insert_edge(edge, kit, end, value);
                ++msize;
            },

            [this, &value] (NodeT * edge, key_iterator eit) {
                edge->split(new_edge(1), eit - edge->kbegin());
                edge->set_value(value);
                ++msize;
            },

            [this, &value, end] (NodeT * edge, key_iterator eit, KeyIterator kit) {
                edge->split(new_edge(2), eit - edge->kbegin());
                insert_edge(edge, kit, end, value);
                ++msize;
            }
        );
    }

    size_t size() const noexcept { return msize; }

    template<typename KeyIterator>
    void add(KeyIterator it, KeyIterator end, const ValueT & value) {
        return insert(it, end, value,
            [] (ValueT & old, const ValueT & n) { old += n; } );
    }

    template<typename KeyIterator>
    void insert(KeyIterator it, KeyIterator end, const ValueT & value) {
        return insert(it, end, value,
            [] (ValueT & old, const ValueT & n) { old = n; });
    }

#ifndef NO_TRIE_ITERATOR

    template <typename KeyIterator>
    iterator find_prefix(KeyIterator it, KeyIterator end, OnExactMatch exactMatch)
    {
        /* NOTE: here, we use shared_ptr just as auto_ptr
         *       to release the object if not needed */

        std::shared_ptr<TrieIteratorInternal> output(new TrieIteratorInternal);

        general_search(it, end,
            /* Exact Match */
            [this, &exactMatch, &output] (NodeT * edge) {
                exactMatch();
                output->m_root = edge;
            },

            [this, &output] (NodeT * edge, KeyIterator kit) {
                output.reset(nullptr);
            },

            [this, &output] (NodeT * edge, key_iterator eit) {
                output->m_root = edge;
            },

            [this, &output] (NodeT * edge, key_iterator eit, KeyIterator kit) {
                output.reset(nullptr);
            }
        );

        return iterator(output);
    }
#endif /* NO_TRIE_ITERATOR */

    template <typename KeyIterator>
    ValueT * get(KeyIterator it, KeyIterator end)
    {
        ValueT * result = nullptr;

        general_search(it, end,
            [&result] (NodeT * edge) {
                if (edge->has_value()) {
                    result = std::addressof(edge->get_value()); }
            },

            [] (NodeT * , KeyIterator) { },
            [] (NodeT * , key_iterator ) { },
            [] (NodeT * , key_iterator , KeyIterator ) { }
        );

        return result;
    }

    template <typename KeyIterator>
    ValueT & at(KeyIterator it, KeyIterator end)
    {
        ValueT * result = get(it, end);

        if (result == nullptr) {
            throw std::out_of_range("trie::at"); 
        }

        return *result;
    }

    struct debug_print
    {
        const trie_map & map;

        debug_print(const trie_map & amap) : map(amap) {};

        std::ostream & operator ()(std::ostream & stream) const
        {
            if (map.edges.empty())
            {
                return stream << "[ empty ]";
            }

            trie_map::IteratorInternalT it(std::addressof(map.edges[0]));

            while (it.m_root != nullptr) 
            {
                const trie_map::NodeT * edge = it.get(0);

                std::copy(edge->kbegin(), edge->kend(), std::ostream_iterator<char, char>(stream));

                if (edge->has_value())
                {
                    stream << "(=" << edge->get_value() << ")";
                }

                if (it.step_down()) { stream << "{"; continue; }
                if (it.step_fore()) { stream << "}{"; continue; }

                while (!it.m_ptrs.empty()) 
                {
                    stream << "}";
                    if (it.step_up()) { break; }
                }

                if (it.m_ptrs.empty()) { it.m_root = nullptr; }
            }

            return stream;
        }

        friend std::ostream & operator << (std::ostream & stream, const debug_print & x)
        {
            return x(stream);
        }
    };
};

template<>
struct OnExactMatch<bool>
{
    bool & match;
    /* implicit */ OnExactMatch(bool & amatch) : match(amatch) { match = false; };
    void operator ()() { match = true; };
};

template<>
struct OnExactMatch<std::nullptr_t>
{
    /* implicit */ OnExactMatch(std::nullptr_t) { };
    void operator ()() { };
};

/**
 * @warning: operator== ALWAYS returns \true if
 *      the left operand dereferences to \0.
 */
template <typename AtomT>
struct CStrIterator : std::forward_iterator_tag
{
private:
    AtomT * m_str;
    typedef CStrIterator<AtomT> self_type;
public:
    CStrIterator(AtomT * a_str) : m_str(a_str) {}
    explicit CStrIterator(AtomT * a_str, size_t offset) : m_str(a_str + offset) {}

    self_type operator ++() { return m_str++; }
    AtomT & operator *()    { return *m_str;  }

    bool operator ==(const self_type & other) const
    {
        return m_str == other.m_str || *m_str == '\0';
    }
};

};

#endif /* TRIE_H */
