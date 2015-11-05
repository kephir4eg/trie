This is a customizable compressed prefix trie (not exactly Radix tree)

# Motivation

First, a minute of being honest, the real motivation behind this project 
is to get myself familiar with new C++11 features. That's why, you will 
probably find excessive use of lambda expressions, unnecessary templates, 
usage of 'and' keyword instead of '&&' etc. Having said that, I'm going to 
use this implementation in some small projects of mine.

So I am going to work on this to make it a really usable library.

# Related work

For simple prefix-tree implementation it makes sense to use 
[boost::spirit::qi::tst_map](http://www.boost.org/doc/libs/1_59_0/libs/spirit/doc/html/spirit/qi/reference/string/symbols.html "Boost Spirit")

Really good library for practical purposes is [patl](https://code.google.com/p/patl/ "Practical PATRICIA Implementation").
In fact this library has much more then just trie implementation.
If you need to use something in your project, please consider this library.
This is a very solid implementation, but I am not sure whether it's supported anymore.

# Simple Usage Examples

The trie implementation has some caveats which should be mentioned:

* value_type for this map is actually something, which is called 
  mapped_type in the regular std::map. We don't store the key string 
  in a way normal map stores it.

* Very heavy-weight iterator. In fact iterator always holds the 
  big vector of the path the current key is stored at. The reason is 
  nodes do not store parent pointers.

* There is an insert method, which accepts replace policy.

## As Regular Map



## As Regular Set

Since there is no additional cost involved, the trie can be used as multiset.
In order to do that the value type should be specified as trie::Counter.

## String Lookup

## Prefix Lookup

## Char Wrapper

## Iterating Over Trie

# Implementation Details

Wiki to read on subject:
* x
* y
* z

As have been mentioned this is not a real Radix tree, Radix trie is 
usually binary. Here, the 

# Testing

# Performance

## Theoretical

## Real Evaluations

I am only comparing this library to standard map. If anyone wants to do 
comparison against existing trie implementations (like boost::tst or PATL), 
I'd be glad to help with that. My expectation is they are going to be 
a little faster.

# Future Plans

## Small Improvements.

* Test it under different compilers

* Make an iterator function, which would return rope instead of string

## Big Changes

* Delete and squeeze

* Implement editing distance lookup

* Make a framework around the structure, which would allow to use it as 
  almost lock-free for reads, using shadow versioned pointers with 
  delayed destruction

* Implement a binary trie (real Radix)
