#include "src/trie.h"

#include <map>
#include <chrono>
#include <fstream>
#include <sstream>

typedef trie::trie_map<char, int>    TestCountingSet;
typedef trie::trie_map<char, int, 0> TestCountingSimpleSet;

typedef std::vector<std::string>     WordSet;

typedef std::map<std::string, int>   StringMap;

template <typename Container>
struct StringInserter
{
    typedef typename Container::mapped_type value_type;
    Container & m;
    explicit StringInserter(Container & am) : m(am) {}

    void operator()(const std::string & key, const value_type & x)
    {
        m.insert(key.begin(), key.end(), x);
    }
};

template <typename Container>
struct StringLookup
{
    typedef typename Container::mapped_type value_type;
    Container & m;
    explicit StringLookup(Container & am) : m(am) {}

    value_type * operator()(const std::string & key)
    {
        return m.get(key.begin(), key.end());
    }
};

template <>
struct StringInserter<StringMap>
{
    typedef typename StringMap::mapped_type value_type;
    StringMap & m;
    explicit StringInserter(StringMap & am) : m(am) {}

    void operator()(const std::string & key, const value_type & x)
    {
        /* Explicity copy the string to be fair */
        m.insert(StringMap::value_type(std::string(key.data(), key.length()), x));
    }
};

template <>
struct StringLookup<StringMap>
{
    typedef typename StringMap::mapped_type value_type;
    StringMap & m;
    explicit StringLookup(StringMap & am) : m(am) {}

    value_type * operator()(const std::string & key)
    {
        auto it = m.find(key);
        return it == m.end() ? nullptr : std::addressof(it->second);
    }
};

struct StatefulRandom
{
    unsigned int seed = 2345;

    int operator()() { return rand_r(&seed); }
    int operator()(int n) { return rand_r(&seed) % n; }
};

struct Generator
{
    int seqsz = 0;
    StatefulRandom rnd;
    WordSet        wordset;

    std::string operator()()
    {
        std::ostringstream str;

        for (int i = seqsz; i > 0; --i)
        {
            str << wordset[rnd() % wordset.size()] << ".";
        }

        str << wordset[rnd() % wordset.size()];

        return str.str();
    };
};

struct perf_clock
{
    typedef std::chrono::high_resolution_clock clock;

    clock::time_point t0;
    uint64_t dt;

    void start() {
        t0 = clock::now();
    }

    void mark() {
        clock::time_point t1 = clock::now();
        dt = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        t0 = t1;
    }

    void psec(const std::string & trial, int itemCount)
    {
        std::cout << trial << ".avg\t" << dt / itemCount << std::endl;
    }
};

template <typename Container>
struct ContainerTest
{
    Container cont;
    volatile int found = 0;
    std::string prefix;
    std::vector<int> numberOfItems;
    std::vector<int> insertTime;
    std::vector<int> lookupTime;

    ContainerTest(const std::string & aprefix) : prefix(aprefix) { };

    void simple()
    {
        StringInserter<Container> inserter(cont);
        StringLookup<Container>   lookup(cont);

        inserter("abcabc",    1);
        inserter("abcabcabc", 2);
        inserter("abcvabc",   3);
        inserter("abcxabc",   4);
        inserter("abcyasbc",  5);
    };

    void test(Generator & generator)
    {
        std::vector<std::string> words;

        StringInserter<Container> inserter(cont);
        StringLookup<Container>   lookup(cont);
        perf_clock pc;

        const int itemCount = 10000;

        for (int i = 0; i < itemCount * 20; ++i) {
            words.push_back(generator());
        }

        uint64_t len = 0;
        std::for_each(words.begin(), words.end(), [&len] (const std::string & x) { len += x.length(); });
        len /= words.size();
        std::cout << "Average length : ~" << len << std::endl;

        for (int _total = 20; _total > 0; --_total)
        {
            numberOfItems.push_back(cont.size());

            pc.start();

            for (int i = 0; i < itemCount; ++i) {
                inserter(words[(i + _total * itemCount) % words.size()], i);
            }

            pc.mark();

            insertTime.push_back(pc.dt / itemCount);

            pc.start();
            found = 0;
            for (int i = 0; i < itemCount * 10; ++i) 
            {
                int * x = lookup(words[(i + _total * itemCount) % words.size()]);
                if (x != nullptr) { ++found; }
            }

            pc.mark();

            lookupTime.push_back(pc.dt / itemCount / 10);
        }

        std::cout << "Positive found : " << found << std::endl;

        words.clear();
        for (int i = 0; i < itemCount * 3; ++i) {
            words.push_back(generator());
        }

        pc.start();
        found = 0;
        for (int i = 0; i < itemCount * 10; ++i) 
        {
            int * x = lookup(words[i % words.size()]);
            if (x != nullptr) { ++found; }
        }

        pc.mark();
        pc.psec(prefix + ".random-lookup", itemCount * 10);

        std::cout << "Random found : " << found << std::endl;
    }
};

int main()
{
    Generator words;

    {
        std::ifstream fin("/usr/share/dict/words");
        std::string line;

        while (getline(fin, line)) {
            words.wordset.push_back(line);
        }
    }

    std::cout << words.wordset.size() << std::endl;
    std::random_shuffle(words.wordset.begin(), words.wordset.end(), words.rnd);

    {
        ContainerTest<TestCountingSet> simple("trie");
        simple.simple();
        std::cout << TestCountingSet::debug_print(simple.cont) << std::endl;
    }

    words.wordset.resize(200000);
    words.seqsz = 0;
    while (words.seqsz < 5)
    {
        std::cout << "***\n" 
            << "seq-len=" << (words.seqsz + 1)
            << " words=" << (words.wordset.size())
            << std::endl;

        std::cout << "*** Map : " << std::endl;

        {
            ContainerTest<StringMap> test1("map");
            words.rnd.seed = 9;
            test1.test(words);
        }

        std::cout << "*** Trie : " << std::endl;

        {
            ContainerTest<TestCountingSet> test1("trie");
            words.rnd.seed = 9;
            test1.test(words);
        }

        words.seqsz++;
        words.wordset.resize(words.wordset.size() / 10);
    }

    return 0;
}
