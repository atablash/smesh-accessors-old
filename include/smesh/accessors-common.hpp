#pragma once

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

	public:
		using BASE::operator=;

		friend OWNER;
	};

}












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






template<class A, class CONTAINER, class EXTRA, class GET_A>
class Index_Iterator_To_Accessor {
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


	// warning! safe to compare only iterators to the same container (optimization)
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
		// const cast to avoid defining a separate const_iterator
		return GET_A(extra, idx);
	}


private:
	inline void increment() {
		do ++idx; while(idx < (int)container.size() && _smesh_detail::is_deleted(container[idx]));
	}

	inline void decrement() {
		// I believe decrementing `begin` is undefined anyway
		do --idx; while(idx > 0 && _smesh_detail::is_deleted(container[idx]));
	}


private:
	Index_Iterator(const EXTRA& ex, const CONTAINER& co, int i) :
			extra(ex), container(co), idx(i) {
		DCHECK_GE(i, 0) << "iterator constructor: index out of range";

		// can be equal to container.size() for end() iterators
		DCHECK_LE(i, container.size()) << "iterator constructor: index out of range";

		// move forward if element is deleted
		while(idx < (int)container.size() && _smesh_detail::is_deleted(container[idx])) {
			++idx;
		}
	}

private:
	EXTRA extra;
	const CONTAINER& container;
	int idx;

	friend Smesh;
};



*/



} // namespace smesh




