#define BOOST_TEST_MODULE trie functional test set
#define BOOST_TEST_DYN_LINK

#include <boost/algorithm/string/predicate.hpp>
#include <boost/test/unit_test.hpp>

#include <string>
#include <set>
#include <src/trie.h>

namespace utf  = boost::unit_test;

#define ITEMS_TO_TEST (128*1024)
#define MAX_LENGTH 1024

typedef trie::trie_map<char, trie::SetCounter> TestSet;
typedef trie::trie_map<char, std::string> TestMapI;

/*
static const char * test_components_1[] =
{
    "1121",   "1231",     "1313",   "41412",
    "31314",  "1223092",  "01121",  "01231",
    "01313",  "041412",   "031314", "012292",
    "11217",  "12319",    "13139",  "414127",
    "313147", "12230927", "11219",  "12317",
    "13137",  "414129",   "313149", "12230929",
};
*/

typedef std::minstd_rand DefaultGenerator; /* Determinism and uniformness are not really important */

template<typename Generator>
std::string generate(Generator & g)
{
    std::string result;

    result.resize(g() % MAX_LENGTH);

    for (unsigned i = 0; i < result.size(); ++i) {
        result[i] = (char) (g() & 0xff);
    }

    return result;
}

BOOST_AUTO_TEST_CASE(fill_map)
{
    DefaultGenerator g(1);
    TestMapI t;
    std::set<std::string> t_model;

    for (int i = ITEMS_TO_TEST; i > 0; --i)
    {
        std::string x = generate(g);
        t_model.insert(x);
        t.insert(x, x);
    }

    for (const std::string & x : t_model)
    {
        auto it = t.find(x);
        BOOST_CHECK(it != t.end());
        BOOST_CHECK(it.value() == x);
        BOOST_CHECK(it.value() == it.key());
        BOOST_CHECK(t.contains(x) == true);
        BOOST_CHECK(t.get(x) != nullptr);
        BOOST_CHECK(t.at(x) == x);

        bool captureMatch = false;
        BOOST_CHECK(t.find_prefix(x, captureMatch).value() == it.key());
        BOOST_CHECK(captureMatch == true);
    }

    for (const std::string & x : t)
    {
        BOOST_CHECK(t_model.find(x) != t_model.end());
    }
}

BOOST_AUTO_TEST_CASE(fill_set)
{
    DefaultGenerator g(1);
    TestSet t;
    std::set<std::string> t_model;

    for (int i = ITEMS_TO_TEST; i > 0; --i)
    {
        std::string x = generate(g);
        t_model.insert(x);
        t.insert(x);
    }

    for (const std::string & x : t_model)
    {
        BOOST_CHECK(t.find(x) != t.end());
        BOOST_CHECK(t.contains(x) == true);
        BOOST_CHECK(t.get(x) != nullptr);
    }
}

BOOST_AUTO_TEST_CASE(prefix_lookup)
{
    TestMapI tmap;
    std::map<std::string, std::string> t_model;

    tmap.insert("/home/user1/audio", "a1");
    tmap.insert("/home/user1/video/x", "v1x");
    tmap.insert("/home/user1/video", "v1");
    tmap.insert("/home/user2/audio", "a2");
    tmap.insert("/home/user2/video", "v2");

    for (auto it = tmap.find_prefix("/home/user1"); it != tmap.end(); ++it) {
        t_model[it.key()] = it.value();
    }

    BOOST_CHECK(t_model.size() == 3);
    BOOST_CHECK(t_model["/home/user1/audio"] == std::string("a1"));
    BOOST_CHECK(t_model["/home/user1/video/x"] == std::string("v1x"));
    BOOST_CHECK(t_model["/home/user1/video"] == std::string("v1"));
}

template<typename M>
void simple(M & t)
{
    t.insert("abcabcabc", 1);
    t.insert("abcabc",    1);
    t.insert("abcvabc",   1);
    t.insert("abcxabc",   1);
    t.insert("abcyasbc",  1);
    t.insert("xabcvabc",  1);
    t.insert("xabcxabc",  1);
    t.insert("xabcyasbc", 1);
}

BOOST_AUTO_TEST_CASE(simple_test_1)
{
    TestSet t;
    simple(t);

    int count;
    bool found;

    auto it = simple.cont.find_prefix("abc", found);
    BOOST_CHECK(found == false);

    count = 0;

    for (; it != simple.cont.end(); ++it)
    {
        BOOST_CHECK(boost::starts_with(it.key(), "abc"));
        ++count;
    }

    BOOST_CHECK(count == 5);

    it = simple.cont.find_prefix("abcabc", found);

    count = 0;

    for (; it != simple.cont.end(); ++it)
    {
        BOOST_CHECK(boost::starts_with(it.key(), "abcabc"));
        ++count;
    }

    BOOST_CHECK(count == 2);

    count = 0;
    it = simple.cont.find_prefix("xabc", [&count] () { ++count; });
    BOOST_CHECK(count == 0);

    count = 0;
    it = simple.cont.find_prefix("xabcxabc", [&count] () { ++count; });
    BOOST_CHECK(count == 1);
}

BOOST_AUTO_TEST_CASE(empty_map)
{
    TestMapI t;

    BOOST_CHECK(t.get("something") == nullptr);
    BOOST_CHECK(t.get("") == nullptr);
    BOOST_CHECK(t.contains("") == false);
    BOOST_CHECK(t.find("") == t.end());
}

BOOST_AUTO_TEST_CASE(empty_map_iterators)
{
    TestMapI t;

    BOOST_CHECK(t.find("") == t.end());
    BOOST_CHECK(t.find_prefix("") == t.end());
    BOOST_CHECK(t.find("something") == t.end());
    BOOST_CHECK(t.find_prefix("something") == t.end());
}

BOOST_AUTO_TEST_CASE(empty_set)
{
    TestSet t;

    BOOST_CHECK(t.get("something") == nullptr);
    BOOST_CHECK(t.get("") == nullptr);
    BOOST_CHECK(t.contains("something") == false);
    BOOST_CHECK(t.contains("") == false);
}

BOOST_AUTO_TEST_CASE(empty_set_iterators)
{
    TestSet t;

    BOOST_CHECK(t.find("") == t.end());
    BOOST_CHECK(t.find_prefix("") == t.end());
    BOOST_CHECK(t.find("something") == t.end());
    BOOST_CHECK(t.find_prefix("something") == t.end());
}
