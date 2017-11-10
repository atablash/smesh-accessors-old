#pragma once






//
// MEMBER DETECTOR
//
// https://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Member_Detector
//

#include <type_traits> // To use 'std::integral_constant'.
#include <iostream>    // To use 'std::cout'.
#include <iomanip>     // To use 'std::boolalpha'.


#define GENERATE_HAS_MEMBER(member)                                               \
                                                                                  \
template < class T >                                                              \
class HasMember_##member                                                          \
{                                                                                 \
private:                                                                          \
    using Yes = char[2];                                                          \
    using  No = char[1];                                                          \
                                                                                  \
    struct Fallback { int member; };                                              \
    struct Derived : T, Fallback { };                                             \
                                                                                  \
    template < class U >                                                          \
    static No& test ( decltype(U::member)* );                                     \
    template < typename U >                                                       \
    static Yes& test ( U* );                                                      \
                                                                                  \
public:                                                                           \
    static constexpr bool RESULT = sizeof(test<Derived>(nullptr)) == sizeof(Yes); \
};                                                                                \
                                                                                  \
template < class T >                                                              \
struct has_member_##member                                                        \
: public std::integral_constant<bool, HasMember_##member<T>::RESULT>              \
{                                                                                 \
};











//
// ENUM CLASS BITMASKS
//
// http://blog.bitwigglers.org/using-enum-classes-as-type-safe-bitmasks/
//



#define ENABLE_BITMASK_OPERATORS(X)                                               \
                                                                                  \
constexpr X operator | (X lhs, X rhs)                                             \
{                                                                                 \
    using underlying = typename std::underlying_type<X>::type;                    \
    return static_cast<X> (                                                       \
        static_cast<underlying>(lhs) |                                            \
        static_cast<underlying>(rhs)                                              \
    );                                                                            \
}                                                                                 \
                                                                                  \
                                                                                  \
constexpr X operator & (X lhs, X rhs)                                             \
{                                                                                 \
    using underlying = typename std::underlying_type<X>::type;                    \
    return static_cast<X> (                                                       \
        static_cast<underlying>(lhs) &                                            \
        static_cast<underlying>(rhs)                                              \
    );                                                                            \
}

