#pragma once

#include <glog/logging.h>

#include <functional>



namespace smesh {

//
// const flag
//
enum class Const_Flag {
	MUTAB = 0,
	CONST = 1
};

namespace {
	constexpr auto MUTAB = Const_Flag::MUTAB;
	constexpr auto CONST = Const_Flag::CONST;
};

template<class T, Const_Flag c> using Const = std::conditional_t<c == CONST, const T, T>;













namespace internal {
	template<Const_Flag C, class OWNER, class BASE>
	class Index_Accessor_Template : public BASE {
		Index_Accessor_Template(Const<OWNER,C>& o, int i)
			: BASE(o, i) {}

		using Context = void;

	public:
		using BASE::operator=;

		friend OWNER;
	};

}














//
// iterator
// wraps CONTAINER::iterator, returns accessor A using GET_A(context&, raw_element&)
//
template<class A, class CONTAINER, Const_Flag C, class CONTEXT, class GET_A = A>
class Iterator {
	using Container_Iterator = std::conditional_t<C == CONST,
		typename CONTAINER::const_iterator,
		typename CONTAINER::iterator>;

public:
	auto& operator++() {
		increment();
		return *this; }

	auto operator++(int) {
		auto old = *this;
		increment();
		return old; }

	auto& operator--() {
		decrement();
		return *this; }

	auto operator--(int) {
		auto old = *this;
		decrement();
		return old; }


	bool operator==(const Iterator& o) const {
		return p == o.p; }

	bool operator!=(const Iterator& o) const {
		return ! (*this == o); }


	template<class AA, Const_Flag CC, class EE, class GG>
	bool operator==(const Iterator<AA,CONTAINER,CC,EE,GG>& o) const {
		return p == o.p;
	}

	template<class AA, Const_Flag CC, class EE, class GG>
	bool operator!=(const Iterator<AA,CONTAINER,CC,EE,GG>& o) const {
		return ! (*this == o);
	}


	A operator*() const {
		return GET_A(context, *p);
	}


private:
	inline void increment() { ++p; }
	inline void decrement() { --p; }

public:
	Iterator(const CONTEXT& c, const Container_Iterator& a_p) :
		context(c), p(a_p) {}

private:
	CONTEXT context;
	Container_Iterator p;
};













//
// iterates a CONTAINER and returns accessors A using GET_A(context, intex)
//
template<class A, class CONTAINER, class CONTEXT, class GET_A = A>
class Index_Iterator {
public:
	auto& operator++() {
		increment();
		return *this; }

	auto operator++(int) {
		auto old = *this;
		increment();
		return old; }

	auto& operator--() {
		decrement();
		return *this; }

	auto operator--(int) {
		auto old = *this;
		decrement();
		return old; }


	// warning! safe to compare only iterators to same container (optimization)
	bool operator==(const Index_Iterator& o) const {
		return idx == o.idx; }

	bool operator!=(const Index_Iterator& o) const {
		return ! (*this == o); }


	template<class AA, class EE, class GG>
	bool operator==(const Index_Iterator<AA,CONTAINER,EE,GG>& o) const {
		return idx == o.idx;
	}

	template<class AA, class EE, class GG>
	bool operator!=(const Index_Iterator<AA,CONTAINER,EE,GG>& o) const {
		return ! (*this == o);
	}


	const A operator*() const {
		return GET_A(context, idx);
	}


private:
	inline void increment() { ++idx; }
	inline void decrement() { --idx; }

public:
	Index_Iterator(const CONTEXT& ex, const CONTAINER& co, int i) :
			context(ex), container(co), idx(i) {
		DCHECK_GE(i, 0) << "iterator constructor: index out of range";

		// can be equal to container.size() for end() iterators
		DCHECK_LE(i, container.size()) << "iterator constructor: index out of range";
	}

private:
	CONTEXT context;
	const CONTAINER& container;
	int idx;
};











/*

//
// an iterator that maps to an accessor instead of raw data
//
template<
	Const_Flag C,
	class CONTAINER,
	class GET_ACCESSOR = void,
	class CONTEXT = void
>
class Iterator_To_Accessor {
	using Container_Iterator = std::conditional_t<C == CONST,
		typename CONTAINER::const_iterator,
		typename CONTAINER::iterator>;

public:
	auto& operator++() {
		++iter;
		return *this; }

	auto operator++(int) {
		auto old = *this;
		++iter;
		return old; }

	auto& operator--() {
		--iter;
		return *this; }

	auto operator--(int) {
		auto old = *this;
		--iter;
		return old; }

	template<class AA, Const_Flag CC, class EE, class GG>
	bool operator==(const Iterator_To_Accessor<AA,CONTAINER,CC,EE,GG>& o) const {
		return iter == o.iter;
	}

	template<class AA, Const_Flag CC, class EE, class GG>
	bool operator!=(const Iterator_To_Accessor<AA,CONTAINER,CC,EE,GG>& o) const {
		return ! (*this == o);
	}


	decltype(auto) operator*() const {
		return GET_ACCESSOR(context, *iter);
	}

private:
	Iterator_To_Accessor(const CONTEXT& c, const Container_Iterator& it) :
		context(c), iter(it) {}

	CONTEXT context;
	Container_Iterator iter;

	friend Smesh;
};








*/



} // namespace smesh




