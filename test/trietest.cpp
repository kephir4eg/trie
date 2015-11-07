#include "src/trie.h"

#include <map>
#include <chrono>
#include <algorithm>
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
        m.insert(key, x);
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
        return m.get(key);
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

std::ostream & operator << (std::ostream & stream, const std::vector<int> & x)
{
    stream << "[";

    if (x.size() != 0)
    {
        std::for_each(x.begin(), x.end() - 1, [&stream] (int y) { stream << y << ", "; });
        stream << *x.rbegin();
    }

    stream << "]";

    return stream;
}

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

        inserter("abcabcabc", 1);
        inserter("abcabc",    1);
        inserter("abcvabc",   1);
        inserter("abcxabc",   1);
        inserter("abcyasbc",  1);
        inserter("xabcvabc",  1);
        inserter("xabcxabc",  1);
        inserter("xabcyasbc", 1);

        lookup("abcabc");
    }

    void words(Generator & generator)
    {
        std::vector<std::string> wset;
        StringInserter<Container> inserter(cont);
        StringLookup<Container>   lookup(cont);
        int total = 20;

        for (int i = 0; i < total; ++i) {
            wset.push_back(generator());
        }

        for (int i = 0; i < total; ++i) {
            inserter(wset[i], 1);
        }

        int lost = total;

        for (int i = 0; i < total; ++i) {
            if (0 != lookup(wset[i])) {
                --lost;
            }
        }

        std::cout << "Lost : " << lost << std::endl;
    }

    void word_set(Generator & generator)
    {
        std::vector<std::string> wset;
        StringInserter<Container> inserter(cont);
        StringLookup<Container>   lookup(cont);
        int total = 100;

        for (int i = 0; i < total; ++i) {
            wset.push_back(generator());
        }

        for (int i = 0; i < total; ++i) {
            cont.insert(wset[i]);
        }

        int lost = total;

        for (int i = 0; i < total; ++i) {
            if (0 != lookup(wset[i])) {
                --lost;
            }
        }

        std::cout << "Lost : " << lost << std::endl;
    }

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
        std::string   line;

        while (getline(fin, line)) {
            words.wordset.push_back(line);
        }
    }

    std::cout << words.wordset.size() << std::endl;
    std::random_shuffle(words.wordset.begin(), words.wordset.end(), words.rnd);

    {
        typedef trie::trie_map<char, int> TestMap;
        TestMap tmap;

        tmap.insert("105", 1);
        tmap.insert("104", 2);
        tmap.insert("2093", 3);
        tmap.insert("2097", 4);

        std::cout << tmap["105"] << " ";
        std::cout << tmap["104"] << " ";
        std::cout << tmap["2093"] << " ";
        std::cout << tmap["2097"] << " ";
        std::cout << std::endl;
    }

    {
        typedef trie::trie_map<char, int> TestMap;
        TestMap tmap;

        tmap.insert("10.0.0.1",    1);
        tmap.insert("10.0.17.8",   2);
        tmap.insert("192.168.0.1", 3);
        tmap.insert("192.168.0.2", 4);

        for (auto it = tmap.begin(); it != tmap.end(); ++it) {
            std::cout << *it << " ";
        }

        std::cout << std::endl;

        for (auto it = tmap.begin(); it != tmap.end(); ++it) {
            std::cout << it.key() << " ";
        }

        std::cout << std::endl;
    }

    {
        typedef trie::trie_map<char, trie::SetCounter> TestSet;
        TestSet tset;

        tset.insert("10.0.0.1");
        tset.insert("10.0.17.8");
        tset.insert("192.168.0.1");
        tset.insert("192.168.0.2");

        std::cout << tset.contains("10.0.0.1") << " ";
        std::cout << tset.contains("10.0.17.8") << " ";
        std::cout << tset.contains("10.0.17.2") << " ";
        std::cout << tset.contains("10.0.1.1") << " ";
        std::cout << std::endl;
    }

    {
        typedef trie::trie_map<char, int> TestMap;
        TestMap tmap;

        tmap.insert("/home/user1/audio", 10);
        tmap.insert("/home/user1/video", 11);
        tmap.insert("/home/user2/audio", 20);
        tmap.insert("/home/user2/video", 21);

        for (auto it = tmap.find_prefix("/home/user1"); it != tmap.end(); ++it) {
            std::cout << it.key() << " ";
            std::cout << *it << ";\n";
        }

        std::cout << std::endl;
    }

    {
        typedef trie::trie_map<char, int, 16> TestMap;
        ContainerTest<TestMap> simple("trie");
        simple.simple();
        std::cout << TestMap::_debug_print(simple.cont) << std::endl;
    }

    {
        typedef trie::trie_map<char, int, 1024> TestMap;
        ContainerTest<TestMap> simple("trie");
        simple.words(words);
        std::cout << TestMap::_debug_print(simple.cont) << std::endl;
    }

    {
        typedef trie::trie_map<char, trie::SetCounter> TestSet;
        ContainerTest<TestSet> simple("trie_set");
        simple.simple();

        for (auto it = simple.cont.begin(); it != simple.cont.end(); ++it)
        {
            std::cout << it.key() << std::endl;
        }

        bool found;
        auto it = simple.cont.find_prefix("abc", found);

        std::cout << " *** prefix exact match : " << found << std::endl;

        for (; it != simple.cont.end(); ++it)
        {
            std::cout << it.key() << std::endl;
        }

        it = simple.cont.find_prefix("abcabc", found);

        std::cout << " *** prefix exact match : " << found << std::endl;

        for (; it != simple.cont.end(); ++it)
        {
            std::cout << it.key() << std::endl;
        }

        it = simple.cont.find_prefix("xabc",
            [] () { std::cout << "Error!" << std::endl; });

        it = simple.cont.find_prefix("xabcxabc",
            [] () { std::cout << "OK exact prefix found!" << std::endl; });

        std::cout << " *** countains 'abcvabc' : " << simple.cont.contains("abcvabc") << std::endl;

        it = simple.cont.find("xabcxabc");

        for (; it != simple.cont.end(); ++it)
        {
            std::cout << it.key() << std::endl;
        }
    }

    {
        typedef trie::trie_map<char, trie::SetCounter> TestSet;
        ContainerTest<TestSet> simple("trie_set");
        simple.word_set(words);
        std::cout << TestSet::_debug_print(simple.cont) << std::endl;

        auto it = simple.cont.find("yaray");

        for (; it != simple.cont.end(); ++it)
        {
            std::cout << it.key() << std::endl;
        }
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

            std::cout << "mapX = "      << test1.numberOfItems << std::endl;
            std::cout << "mapInsert = " << test1.insertTime << std::endl;
            std::cout << "mapLookup = " << test1.lookupTime << std::endl;
        }

        std::cout << "*** Trie 0 : " << std::endl;

        {
            ContainerTest<trie::trie_map<char, int, 0> > test1("trie");
            words.rnd.seed = 9;
            test1.test(words);

            std::cout << "trie0X = "      << test1.numberOfItems << std::endl;
            std::cout << "trie0Insert = " << test1.insertTime << std::endl;
            std::cout << "trie0Lookup = " << test1.lookupTime << std::endl;
        }

        std::cout << "*** Trie 1K : " << std::endl;

        {
            ContainerTest<trie::trie_map<char, int, 1024> > test1("trie");
            words.rnd.seed = 9;
            test1.test(words);

            std::cout << "trie1X = "      << test1.numberOfItems << std::endl;
            std::cout << "trie1Insert = " << test1.insertTime << std::endl;
            std::cout << "trie1Lookup = " << test1.lookupTime << std::endl;
        }

        std::cout << "*** Trie 4K : " << std::endl;

        {
            ContainerTest<trie::trie_map<char, int, 4*1024> > test1("trie");
            words.rnd.seed = 9;
            test1.test(words);

            std::cout << "trie4X = "      << test1.numberOfItems << std::endl;
            std::cout << "trie4Insert = " << test1.insertTime << std::endl;
            std::cout << "trie4Lookup = " << test1.lookupTime << std::endl;
        }

        words.seqsz++;
        words.wordset.resize(words.wordset.size() / 10);
    }

    return 0;
}
