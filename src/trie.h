#ifndef TRIE_H
#define TRIE_H

#include <memory>
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <iterator>
#include <iostream>
#include <utility>
#include <type_traits>

namespace trie
{

struct SetCounter { };

namespace detail
{

template <typename AtomT, typename PrefixHolderT>
struct TrieNode : public PrefixHolderT
{
private:
    typedef TrieNode<AtomT, PrefixHolderT> self_type;
    typedef self_type * self_pointer;

    self_pointer * data = nullptr;
    uint32_t size = 0; /* Will still be 64 bit due to alignment */

public:
    typedef self_type * const * map_iterator;

    TrieNode(int hint) { if (hint > 0) { resize(hint); } }

    inline static int atom_hash(AtomT x, uint32_t mask) {
        return (x & mask);
    }

    inline static int least_uncolliding_size(AtomT a, AtomT b)
    {
        unsigned int v = ((int) a ^ (int) b);
        return (v & -v) << 1;
    }

    static self_type * value(map_iterator x) { return *x; };

    void resize(uint32_t new_size)
    {
        self_pointer * ndata = (new_size == 0) ?
            nullptr : (new self_pointer[new_size]);

        std::fill(ndata, ndata + new_size, nullptr);

        if (new_size > size) {
            for (uint32_t i = 0; i < size; ++i) {
                if (data[i] != nullptr) {
                    ndata[atom_hash(*data[i]->kbegin(), new_size - 1)] = data[i];
                }
            }
        }

        delete[] data;
        data = ndata;
        size = new_size;
    }

    map_iterator find(AtomT x) const 
    {
        if (size != 0)
        {
            map_iterator result = data + atom_hash(x, size-1);

            if (nullptr != *result and (*result)->starts_with(x)) {
                return result;
            }
        }

        return nullptr;
    }

    ~TrieNode() { resize(0); };

    void put(self_type * edge)
    {
        AtomT x = *edge->kbegin();

        if (size == 0) { resize(2); }

        int hash = atom_hash(x, size-1);

        if (data[hash] == nullptr) {
            data[hash] = edge;
            return;
        }

        resize(least_uncolliding_size(x, *data[hash]->kbegin()));

        data[atom_hash(x, size-1)] = edge;
    }

    map_iterator begin() const { return data; }
    map_iterator end()   const { return data + size; }
    std::nullptr_t nf()  const { return nullptr; }

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
    typedef std::vector< AtomT > key_type;
    typedef typename NodeT::map_iterator traverse_ptr;

    key_type base_prefix;
    const NodeT * m_root;
    std::vector<traverse_ptr> m_ptrs;

    TrieIteratorInternal(const NodeT * a_root) : m_root(a_root) { };

    const NodeT * get(int i = 0) const
    {
        int j = (int) m_ptrs.size() + i;
        return j > 0 ? NodeT::value(m_ptrs[j - 1]) : (j == 0 ? m_root : nullptr);
    }

    typename NodeT::value_type & get_value() const
    {
        NodeT * top = const_cast<NodeT *>(m_ptrs.empty() ? m_root : NodeT::value(m_ptrs.back()));
        return top->get_value();
    }

    std::basic_string<AtomT> get_key_str()
    {
        std::basic_string<AtomT> result(base_prefix.begin(), base_prefix.end());

        const NodeT * i = m_root;

        std::copy(i->kbegin(), i->kend(), std::back_inserter(result));

        for (auto && traverse_ptr : m_ptrs)
        {
            i = NodeT::value(traverse_ptr);
            std::copy(i->kbegin(), i->kend(), std::back_inserter(result));
        }

        return result;
    }

    key_type get_key()
    {
        key_type result(base_prefix);

        const NodeT * i = m_root;
        result.insert(result.back(), i->kbegin(), i->kend());

        for (auto && traverse_ptr : m_ptrs)
        {
            i = traverse_ptr->second;
            result.insert(result.back(), i->kbegin(), i->kend());
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

    bool next_value()
    {
        do {
            next();
        } while (m_root != nullptr and !get()->has_value());

        return m_root != nullptr;
    }

    void push(traverse_ptr it)
    {
        m_ptrs.push_back(it);
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

    bool starts_with(AtomT x) const { return (*chunk)[begin] == x; };
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
        this->end   = next->begin;
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

    bool starts_with(AtomT x) const { return *prefix == x; };
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
    /* No default implementation.
     * Must be specialized by code, which tries to use it. */
};

template<typename AtomT, typename ValueT, size_t CMinChunkSize>
struct TrieNodeSelector<AtomT, ValueT, CMinChunkSize, 
    typename std::enable_if<std::is_integral<AtomT>::value>::type>
{
    typedef PrefixHolder<char, ValueT, CMinChunkSize>  PrefixHolderType;
    typedef TrieNode<AtomT, PrefixHolderType>    type;
};

};

template <typename AtomT, typename ValueT, size_t CMinChunkSize = 0, 
    typename NodeImpl = typename detail::TrieNodeSelector<AtomT, ValueT, CMinChunkSize>::type >
struct trie_map
{
private:
    typedef NodeImpl NodeT;
    typedef detail::TrieIteratorInternal<AtomT, NodeT> IteratorInternalT;

    /* We use deque in order to make edge storage stable */
    typedef std::deque< NodeT >                    EdgeStorageT;
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
    void insert_infix(KeyIterator it, KeyIterator end, NodeT * parent, NodeT * n)
    {
        size_t ksize = std::distance(it, end);
        ChunkT * target = parent == nullptr ? nullptr : parent->insertion_hint();

        if ((target == nullptr) or (target->size() + ksize) > CMinChunkSize)
        {
            if (CMinChunkSize == 0 or keys.empty() or 
                    (keys.back().size() + ksize) > CMinChunkSize)
            {
                keys.emplace_back();
                keys.back().reserve(CMinChunkSize);
            }

            target = std::addressof(keys.back());
        }

        /* WARNING : Here, we rely on the fact, that vector pointers always
         * remain stable if values inserted fit into reserved space, which
         * should work in practice, but std::vector specification
         * does not guarantee that.
         */
        detail::trie_offset_t kidx    = target->size();
        target->insert(target->end(), it, end);
        detail::trie_offset_t kendidx = target->size();

        n->setkey(target, kidx, kendidx);
    }

    EdgeStorageT edges;

    NodeT * root() { return std::addressof(*edges.begin()); }

    NodeT * new_edge(int hint)
    {
        edges.emplace_back(hint);
        return std::addressof(edges.back());
    }

    template<typename KeyIterator>
    NodeT * insert_edge(NodeT * parent, KeyIterator it, KeyIterator end, const value_type & value)
    {
        NodeT * n = new_edge(0);
        insert_infix(it, end, parent, n);
        if (parent != nullptr) { parent->put(n); }
        n->set_value(value);
        return n;
    }

    template<typename ReplacePolicy>
    void insert_value(NodeT & at, const value_type & value, const ReplacePolicy & replace)
    {
        if (at.has_value()) {
            replace(at.get_value(), value);
        } else {
            at.set_value(value);
        }
    }

    typedef std::shared_ptr<IteratorInternalT> IteratorPtr;

public:

    /** @brief The iterator with very unfair behaviour
     *
     *  There is no const iterator counterpart, because there is no real
     *  benefit in making this iterator const.
     *
     *  @warning: This operator is not copiable in regular sense.
     *    In order to get a fixed copy of the iterator,
     *    use explicit clone() method!
     */
    struct iterator : public std::forward_iterator_tag
    {
        friend class trie_map;

    private:
        IteratorPtr _impl;

        iterator() : _impl() { };

        void normalize() {
            if (!_impl->get()->has_value()) { ++(*this); }
        }

        explicit iterator(IteratorPtr a_impl)
            : _impl(a_impl) { normalize(); }

        explicit iterator(IteratorInternalT * a_impl)
            : _impl(a_impl) { normalize(); }

    public:
        value_type & value() {
            return _impl->get_value();
        }

        std::basic_string<AtomT> key() {
            return _impl->get_key_str();
        }

        value_type & operator *() { return value(); }

        /*
         * There is only one increment operator, since the postfix one
         * is very heavy if implemented correctly.
         */
        iterator & operator ++()
        {
            if (_impl.get() != nullptr)  {
                if (!_impl->next_value()) { _impl.reset(); }
            }

            return *this;
        }

        /**
         * Returns a "real" copy of the iterator, may be a heavy operation.
         */
        iterator clone() {
            return (_impl.get() == nullptr) ?
                iterator() : iterator(new IteratorInternalT(*_impl));
        }

        bool operator == (const iterator & other) const
        {
            return _impl.get() == other._impl.get()
                || (_impl.get() && other._impl.get()
                        && _impl->get() == other._impl->get());
        }

        bool operator != (const iterator & other) const
        {
            return not (*this == other);
        }
    };

private:
    typedef typename NodeT::map_iterator NodeItr;

    /**
     * Generalized lookup algorithm.
     */
    template<typename KeyIterator, typename A, typename B, typename C, typename D, typename E>
    inline void general_search
    (
        NodeT * n,
        KeyIterator it,
        KeyIterator end,
        A exactMatchAction,
        B noNextEdgeAction,
        C endInTheMiddleAction,
        D splitInTheMiddleAction,
        E edgeAction
    )
    {
        key_iterator kbegin = n->kbegin();

        while (n != nullptr)
        {
            key_iterator kend   = n->kend();
            key_iterator k      = kbegin;

            while ((it != end) and (k != kend) and (*k == *it))
                { ++k; ++it; }

            if (it == end)
            {
                if (k == kend) {
                    exactMatchAction(n);
                } else {
                    endInTheMiddleAction(n, k);
                }

                return;
            }
            else if (k != kend)
            {
                splitInTheMiddleAction(n, k, it);
                return;
            }

            NodeItr next_edge = n->find(*it);

            if (next_edge == n->nf())
            {
                noNextEdgeAction(n, it);
                return;
            }

            edgeAction(next_edge, it);

            n = NodeT::value(next_edge);
            kbegin = n->kbegin() + 1;
            ++it; /* Already found the first character */
        }
    }

public:
    template<typename KeyIterator, typename ReplacePolicy>
    void insert(KeyIterator it, KeyIterator end, const value_type & value,
                    const ReplacePolicy & replace)
    {
        if (edges.empty())
        {
            insert_edge(nullptr, it, end, value);
            ++msize;
            return;
        }

        general_search(root(), it, end,
            [this, &value, &replace] (NodeT * n) {
                insert_value(*n, value, replace);
            },

            [this, &value, end] (NodeT * n, KeyIterator kit) {
                insert_edge(n, kit, end, value);
                ++msize;
            },

            [this, &value] (NodeT * n, key_iterator eit) {
                n->split(new_edge(1), eit - n->kbegin());
                n->set_value(value);
                ++msize;
            },

            [this, &value, end] (NodeT * n, key_iterator eit, KeyIterator kit) {
                n->split(new_edge(2), eit - n->kbegin());
                insert_edge(n, kit, end, value);
                ++msize;
            },

            [] (NodeItr x, KeyIterator) { (void)x; }
        );
    }

    size_t size() const noexcept { return msize; }

    template<typename KeyIterator>
    void add(KeyIterator it, KeyIterator end, const value_type & value) {
        return insert(it, end, value,
            [] (value_type & old, const value_type & n) { old += n; } );
    }

    template<typename KeyIterator>
    void insert(KeyIterator it, KeyIterator end, const value_type & value) {
        return insert(it, end, value,
            [] (value_type & old, const value_type & n) { old = n; });
    }

    template<typename ReplacePolicy>
    void insert(const std::basic_string<AtomT> & str, const value_type & value,
                    const ReplacePolicy & replace)
    {
        return insert(str.begin(), str.end(), value, replace);
    }

    void add(const std::basic_string<AtomT> & str, const value_type & value) {
        return add(str.begin(), str.end(), value);
    }

    void insert(const std::basic_string<AtomT> & str, const value_type & value) {
        return insert(str.begin(), str.end(), value);
    }

private:
    template<typename _ValueT, 
        typename = typename std::enable_if<std::is_same<_ValueT, SetCounter>::value>::type>
    struct SetSpecific { };

public:

    template<typename KeyIterator, typename _ValueT = ValueT, typename = SetSpecific<_ValueT> >
    void insert(KeyIterator it, KeyIterator end) {
        return insert(it, end, 1);
    }

    template<typename KeyIterator, typename _ValueT = ValueT, typename = SetSpecific<_ValueT>  >
    void add(KeyIterator it, KeyIterator end) {
        return add(it, end, 1);
    }

    template<typename _ValueT = ValueT, typename = SetSpecific<_ValueT> >
    void insert(const std::basic_string<AtomT> & str) {
        return insert(str.begin(), str.end(), 1);
    }

    template<typename _ValueT = ValueT, typename = SetSpecific<_ValueT>  >
    void add(const std::basic_string<AtomT> & str) {
        return add(str.begin(), str.end(), 1);
    }

    template<typename KeyIterator>
    bool contains(KeyIterator it, KeyIterator end)
    {
        if (edges.empty()) { return false; }

        bool result = false;

        general_search(root(), it, end,
            [&result] (NodeT * n) {
                if (n->has_value()) { result = true; } },

            [] (NodeT * , KeyIterator) { },
            [] (NodeT * , key_iterator ) { },
            [] (NodeT * , key_iterator , KeyIterator ) { },
            [] (NodeItr, KeyIterator) { }
        );

        return result;
    }

    bool contains(const std::basic_string<AtomT> & str)
    {
        return contains(str.begin(), str.end());
    }

private:
    template <typename KeyIterator, typename CallbackType>
    iterator find_prefix_int(NodeT * root_node, KeyIterator it, KeyIterator kend, CallbackType exactMatch)
    {
        iterator output;
        KeyIterator inputEnd;

        general_search(root_node, it, kend,
            /* Exact Match */
            [&exactMatch, &output] (NodeT * n)  {
                if (n->has_value()) { exactMatch(); }
                output._impl.reset(new IteratorInternalT(n));
            },

            [] (NodeT *, KeyIterator) { },

            [&output] (NodeT * n, key_iterator) { 
                output._impl.reset(new IteratorInternalT(n));
            },

            [] (NodeT *, key_iterator, KeyIterator) {  },

            [&inputEnd] (NodeItr, KeyIterator iend) {
                inputEnd = iend; }
        );

        if (output._impl.get() != nullptr)
        {
            output.normalize();
            std::copy(it, inputEnd, std::back_inserter(output._impl->base_prefix));
        }

        return output;
    }

    template <typename KeyIterator>
    iterator find_prefix_int(NodeT * root_node, KeyIterator it, KeyIterator kend, bool & exactMatch)
    {
        exactMatch = false;
        return find_prefix_int(root_node, it, kend, [&exactMatch] () { exactMatch = true; });
    }

    template <typename KeyIterator>
    iterator find_prefix_int(NodeT * root_node, KeyIterator it, KeyIterator kend, std::nullptr_t)
    {
        return find_prefix_int(root_node, it, kend, [] () {});
    }

public:
    template <typename KeyIterator, typename CallbackType>
    iterator find_prefix(KeyIterator it, KeyIterator kend, CallbackType exactMatch)
    {
        if (edges.empty()) { return end(); }
        return find_prefix_int(root(), it, kend, exactMatch);
    }

    /* NOTE : this specialization is needed to catch bool as reference, not as value */
    template <typename KeyIterator>
    iterator find_prefix(KeyIterator it, KeyIterator kend, bool & exactMatch)
    {
        if (edges.empty()) { return end(); }
        return find_prefix_int(root(), it, kend, exactMatch);
    }

    template <typename KeyIterator, typename CallbackType>
    iterator find_prefix(const iterator & base, KeyIterator it, KeyIterator kend, CallbackType exactMatch)
    {
        if (base._impl.get() == nullptr || base._impl->get() == nullptr) {
            return end();
        }

        return find_prefix_int(base._impl->get(), it, kend, exactMatch);
    }

    template <typename CallbackType>
    iterator find_prefix(const std::basic_string<AtomT> & str, CallbackType exactMatch) {
        return find_prefix(str.begin(), str.end(), exactMatch);
    }

    /* NOTE : this "specialization" (overload actually) is needed to catch 
     * bool as reference, not as value */
    iterator find_prefix(const std::basic_string<AtomT> & str, bool & exactMatch) {
        return find_prefix(str.begin(), str.end(), exactMatch);
    }

    iterator find_prefix(const std::basic_string<AtomT> & str) {
        return find_prefix(str.begin(), str.end(), [] () {});
    }

    template <typename KeyIterator>
    iterator find(KeyIterator it, KeyIterator kend)
    {
        if (edges.empty()) { return end(); }

        IteratorInternalT * root_it = new IteratorInternalT(root());
        IteratorPtr output(root_it);

        general_search(root(), it, kend,
            [&output] (NodeT * n) {
                if (!n->has_value()) {
                    output.reset(); } },

            [&output] (NodeT * n, KeyIterator  ) { output.reset(); },
            [&output] (NodeT * n, key_iterator ) { output.reset(); },
            [&output] (NodeT * n, key_iterator, KeyIterator ) 
                { output.reset(); },

            [&output] (NodeItr x, KeyIterator)   { output->push(x); }
        );

        return iterator(output);
    }

    iterator find(const std::basic_string<AtomT> & str)
    {
        return find(str.begin(), str.end());
    }

    iterator begin() { 
        return edges.empty() ? end() :
            iterator(IteratorPtr(new IteratorInternalT(root()))); }

    iterator end()   { return iterator(); }

    template <typename KeyIterator>
    value_type * get(KeyIterator it, KeyIterator end)
    {
        if (edges.empty()) { return nullptr; }

        value_type * result = nullptr;

        general_search(root(), it, end,
            [&result] (NodeT * n) {
                if (n->has_value()) {
                    result = std::addressof(n->get_value()); }
            },

            [] (NodeT * , KeyIterator) { },
            [] (NodeT * , key_iterator ) { },
            [] (NodeT * , key_iterator , KeyIterator ) { },
            [] (NodeItr, KeyIterator) { }
        );

        return result;
    }

    value_type * get(const std::basic_string<AtomT> & str)
    {
        return get(str.begin(), str.end());
    }

    template <typename KeyIterator>
    value_type & at(KeyIterator it, KeyIterator end)
    {
        value_type * result = get(it, end);

        if (result == nullptr) {
            throw std::out_of_range("trie::at"); 
        }

        return *result;
    }

    value_type & at(const std::basic_string<AtomT> & str)
    {
        return at(str.begin(), str.end());
    }

    value_type & operator [](const std::basic_string<AtomT> & str)
    {
        return at(str.begin(), str.end());
    }

    size_t _edges() { return edges.size(); }
    size_t _keys()  { return keys.size(); }

    struct _debug_print
    {
        const trie_map & map;

        _debug_print(const trie_map & amap) : map(amap) {};

        std::ostream & operator ()(std::ostream & stream) const
        {
            if (map.edges.empty())
            {
                return stream << "[ empty ]";
            }

            trie_map::IteratorInternalT it(std::addressof(map.edges[0]));

            while (it.m_root != nullptr) 
            {
                const trie_map::NodeT * n = it.get(0);

                std::copy(n->kbegin(), n->kend(), 
                    std::ostream_iterator<char, char>(stream));

                if (n->has_value())
                {
                    stream << "(=" << n->get_value() << ")";
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

        friend std::ostream & operator << (std::ostream & stream,
            const _debug_print & x)
        {
            return x(stream);
        }
    };
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
