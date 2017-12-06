#pragma once






//
// MEMBER DETECTOR
//
// https://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Member_Detector
//

#include <type_traits> // To use 'std::integral_constant'.
#include <iostream>    // To use 'std::cout'.
#include <iomanip>     // To use 'std::boolalpha'.


#define GENERATE_HAS_MEMBER(member)                                                \
                                                                                   \
template < class T >                                                               \
class HasMember_##member                                                           \
{                                                                                  \
    struct Fallback { int member; };                                               \
    struct Derived : T, Fallback { };                                              \
                                                                                   \
    template<typename U, U> struct Check;                                          \
                                                                                   \
    typedef char ArrayOfOne[1];                                                    \
    typedef char ArrayOfTwo[2];                                                    \
                                                                                   \
    template<typename U> static ArrayOfOne & func(Check<int Fallback::*, &U::member> *); \
    template<typename U> static ArrayOfTwo & func(...);                            \
  public:                                                                          \
    typedef HasMember_##member type;                                               \
    enum { value = sizeof(func<Derived>(0)) == 2 };                                \
};                                                                                 \
                                                                                   \
template < class T >                                                               \
struct has_member_##member                                                         \
: public std::integral_constant<bool, HasMember_##member<T>::value>                \
{                                                                                  \
};








//
// ENUM CLASS BITMASKS
//
// http://blog.bitwigglers.org/using-enum-classes-as-type-safe-bitmasks/
//



#define ENABLE_BITWISE_OPERATORS(X)                                               \
                                                                                  \
constexpr X operator | (X lhs, X rhs)                                             \
{                                                                                 \
    using underlying = typename std::underlying_type_t<X>;                        \
    return static_cast<X> (                                                       \
        static_cast<underlying>(lhs) |                                            \
        static_cast<underlying>(rhs)                                              \
    );                                                                            \
}                                                                                 \
                                                                                  \
constexpr X operator & (X lhs, X rhs)                                             \
{                                                                                 \
    using underlying = typename std::underlying_type_t<X>;                        \
    return static_cast<X> (                                                       \
        static_cast<underlying>(lhs) &                                            \
        static_cast<underlying>(rhs)                                              \
    );                                                                            \
}                                                                                 \
                                                                                  \
constexpr X operator ! (X x)                                                      \
{                                                                                 \
    using underlying = typename std::underlying_type_t<X>;                        \
    return static_cast<X> (                                                       \
        ! static_cast<underlying>(x)                                              \
    );                                                                            \
}                                                                                 \
                                                                                  \
constexpr X operator ~ (X x)                                                      \
{                                                                                 \
    using underlying = typename std::underlying_type_t<X>;                        \
    return static_cast<X> (                                                       \
        ~ static_cast<underlying>(x)                                              \
    );                                                                            \
}

