# C++11 Trie Implementation

This is a C++ implementation of 
compressed prefix [trie](https://en.wikipedia.org/wiki/Trie "Trie").

To use it, you don't need to clone the repository.
You only need to download one single file, *trie.h*.
All the implementation is in one header.

*This implementation depends on C++11*

### When to use

Using trie is beneficial when have a lot of very long strings having the common prefix 
(e.g. urls, file paths) and you want to use them as keys for set or map. 
Trie will provide you with performance, close to the one of hash table 
(still worse on average, however), but with some benefits, like partial search 
(currently by prefix substring).

How long is very long? Is would say that the real difference will only become 
noticeable when the average length is greater then CPU cache line (usually 64 bytes).
Below that I would not expect benefit in performance over the 
normal map operations.

In future, I plan to implement fast pattern matching and edit distance search,
which cannot be implemented efficiently with hash table.

Trie is also theoretically is more space efficient then just regular `map`/`unordered_map`.
In practice that heavily depends on the nature of strings you have.

## Motivation

First, a minute of being honest, the real motivation behind this project 
is to get myself familiar with new C++11 features. That's why, you will 
probably find excessive use of lambda expressions, unnecessary templates, 
usage of `and` keyword instead of `&&` etc. Having said that, I'm going to 
use this implementation in some small projects of mine.

I am going to work on this to make it a really usable library.
My goal is to do at least one improvement/bug fix per week.

## Related work

For simple prefix-tree implementation it makes sense to use 
[boost::spirit::qi::tst_map](http://www.boost.org/doc/libs/1_59_0/libs/spirit/doc/html/spirit/qi/reference/string/symbols.html "Boost Spirit")

Really good library for practical purposes is [patl](https://code.google.com/p/patl/ "Practical PATRICIA Implementation").
In fact this library has much more then just trie implementation.
If you need to use something in your project, please consider this library.
This is a very solid implementation, but I am not sure whether it's supported anymore.

## Simple Usage Examples

The trie implementation has some caveats which should be mentioned 
from the beginning.

* `value_type` for this map is actually something, which is called 
  `mapped_type` in the regular `std::map`. We don't store the key string 
  in a way normal map stores it.

* Very heavy-weight iterator. In fact iterator always holds the 
  big vector of the path the current key is stored at.

### As Regular Map

```C++
{
    typedef trie::trie_map<char, int> TestMap;
    TestMap tmap;

    tmap.insert("105", 105);
    tmap.insert("104", 104);
    tmap.insert("2093", 2093);
    tmap.insert("2097", 2097);

    std::cout << tmap["105"] << " ";
    std::cout << tmap["104"] << " ";
    std::cout << tmap["2093"] << " ";
    std::cout << tmap["2097"] << " ";
    std::cout << std::endl;
}
```

```
1 2 3 4
```

### As Regular Set

Since there is no additional cost involved, the trie can be used as multiset.
In order to do that the value type should be specified as `trie::SetCounter`.
Note, that specifying `SetCounter` as value type is different then 
just specifying `int`. Internally, `SetCounter` provides different node 
implementation, which is more efficient. It is very similar to `int`, but 
uses 0 as a special value to distinguish between leaf and non-leaf nodes.

```C++
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
```

```
1 1 0 0
```

It's not recommended to use `find()` or `find_prefix()` to look for key in set,
because the iterator they return is very heavy. Function `contains()`
does a much more efficient lookup.

### Iterating Over Trie

Trie is a different from the map when it comes to iterating over objects.
The reason for that difference is that it does not really store keys,
so dereferencing the iterator does not return `pair<key, value>`, instead it 
only returns the reference to the value. To obtain key, we call `key()`
method of the iterator, which reconstructs the key (thus, somewhat slow).

Also note, that iterator itself is very heavy, since we store the whole 
path to the current node in the tree. One can think of that as a drawback, 
but on the other hand, we don't have to store parent node reference this way.

```C++
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

    for (auto it = tmap.begin(); it != tmap.end(); ++it) {
        std::cout << it.key() << " ";
    }
}
```

```
1 2 4 3 10.0.0.1 10.0.17.8 192.168.0.2 192.168.0.1 
```

From the perspective of ordering, trie is somewhat in the middle between `map` 
and `unordered_map`. The only order, which is guaranteed during iteration is that
any string appears later then any of its prefix substring. However, strict ordering 
can be achieved, it just requires a more sophisticated iterator (TBD).

### Subtrie Iterator

There is a special kind of iterator, which is _subtrie iterator_. It is returned by
`find_prefix()` method. After the subtrie is traversed (regular `++` operator), 
the iterator becomes equal to the _end iterator_. So, _end iterator_ is universal 
for trie and any of its subtries.

```C++
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
}
```

```
/home/user1/video 11;
/home/user1/audio 10;
```

## Implementation Details

Wiki to read on subject:
* [Trie](https://en.wikipedia.org/wiki/Trie "Trie")
* [Radix Trie](https://en.wikipedia.org/wiki/Radix_tree "Radix Trie")

## Testing

TBD

## Performance

TBD

### Theoretical

TBD

### Real Evaluations

I am only comparing this library to standard map. If anyone wants to do 
comparison against existing trie implementations (like `boost::tst` or `PATL`), 
I'd be glad to help with that. My expectation is they are going to be 
a little faster.

TBD

## Future Plans

### Small Improvements.

* Delete and remove

* Custom allocators

* Test it under different compilers

* Create an ordering iterator

* Create an iterator function (like key()), which would return rope 
instead of string

### Big Changes

* Implement squeeze to increase locality

* Implement editing distance lookup

* Make a framework around the structure, which would allow to use it as 
  almost lock-free for reads, using shadow versioned pointers with 
  delayed destruction

* Implement a binary trie (real Radix)
