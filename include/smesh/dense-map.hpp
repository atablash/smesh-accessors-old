#pragma once

#include "common.hpp"
#include "accessors-common.hpp"

#include <glog/logging.h>

#include <deque>


namespace smesh {







enum class Dense_Map_Type {
	VECTOR,
	DEQUE
};

namespace {
	constexpr auto VECTOR = Dense_Map_Type::VECTOR;
	constexpr auto DEQUE  = Dense_Map_Type::DEQUE;
}






enum class Dense_Map_Flags {
	NONE = 0,
	ERASABLE = 0x0001
};

ENABLE_BITMASK_OPERATORS(Dense_Map_Flags);

namespace {
	constexpr auto ERASABLE = Dense_Map_Flags::ERASABLE;
}





namespace internal {
	// for Dense_Map
	template<bool> class Add_Member_offset {protected: int offset = 0; };
	template<>     class Add_Member_offset <false> {};

	// for Node
	template<bool> struct Add_Member_exists { bool exists = false; };
	template<>     struct Add_Member_exists <false> {};
}







namespace internal {
	constexpr auto default__Dense_Map_Flags = ERASABLE;
	constexpr auto default__Dense_Map_Type = VECTOR;

	template<Const_Flag C, class OWNER, class BASE>
	using Default__Dense_Map_Accessor_Template = Index_Accessor_Template<C, OWNER, BASE>;
}






//
// a vector with some elements marked as deleted - lazy deletion
//
// ACCESSOR_TEMPLATE: should derive from second tmeplate argument; it will be Accessor_Base
//
// TODO: don't call constructors too early
//
// TODO: implement a linked version to jump empty spaces faster
//
template<
	class T,
	Dense_Map_Flags FLAGS = internal::default__Dense_Map_Flags,
	Dense_Map_Type TYPE = internal::default__Dense_Map_Type,
	template<Const_Flag,class,class> class ACCESSOR_TEMPLATE = internal::Default__Dense_Map_Accessor_Template
>
class Dense_Map : public internal::Add_Member_offset<TYPE == DEQUE> {


	// forward declarations
	public: template<Const_Flag C> class Accessor_Base;
	private: struct Node;


public:
	using Mapped_Type = T;
	static constexpr auto Flags = FLAGS;
	using Container = std::conditional_t<TYPE == VECTOR, std::vector<Node>, std::deque<Node>>;
	template<Const_Flag C> using Accessor = typename Accessor_Base<C>::Derived;


public:
	Dense_Map() {}

	Dense_Map(int new_offset)
		: internal::Add_Member_offset<TYPE == DEQUE>(new_offset) {}

	decltype(auto) operator[](int key) {
		return Accessor<MUTAB>(*this, key - get_offset());
	}

	decltype(auto) operator[](int key) const {
		return Accessor<CONST>(*this, key - get_offset());
	}

	auto domain_begin() const {
		return get_offset();
	}

	auto domain_end() const {
		return get_offset() + raw.size();
	}

	template<class VAL>
	inline void push_back(VAL&& val) {
		raw.push_back(std::forward<VAL>(val));
	}

	template<class... Args>
	void emplace_back(Args&&... args) {
		raw.emplace_back(std::forward<Args>(args)... );
	}


	auto begin()       {  return Iterator<MUTAB>(*this, get_offset());  }
	auto end()         {  return Iterator<MUTAB>(*this, get_offset() + raw.size());  }

	auto begin() const {  return Iterator<CONST>(*this, get_offset());  }
	auto end()   const {  return Iterator<CONST>(*this, get_offset() + raw.size());  }













	//
	// ACCESSOR (BASE)
	//

public:
	template<Const_Flag C>
	class Accessor_Base {
	public:
		using Derived = ACCESSOR_TEMPLATE<C, Dense_Map, Accessor_Base>;

	public:
		const int key;
		Const<Mapped_Type,C>& value;

		bool exists;

		void erase() const {
			DCHECK(key - owner.get_offset() < (int)owner.raw.size());
			DCHECK(raw().exists);
			raw().exists = false;
		}

		operator Const<Mapped_Type,C>&() const {
			return value;
		}

		template<class TT>
		auto operator=(TT&& new_value) const {

			if constexpr(TYPE==DEQUE) {
				if(owner.raw.empty()) {
						owner.offset = key;
						idx = 0;
				}

				if(idx < 0) {
					owner.offset += idx;

					//owner.raw.insert(owner.raw.begin(), std::max(diff, (int)owner.raw.size()*3/2), Node());
					owner.raw.insert(owner.raw.begin(), -idx, Node());
					idx = 0;
				}
			}

			if(idx >= (int)owner.raw.size()) {
				owner.raw.resize(idx + 1);
			}

			raw().exists = true;
			raw().value = std::forward<TT>(new_value);
			return Accessor<C>(owner, idx);
		}

	// store environment:
	protected:
		Accessor_Base( Const<Dense_Map,C>& o, int i) :
				key(i + o.get_offset()),
				value(o.raw[i].value),
				exists(i < (int)o.raw.size() && i >= 0 && o.raw[i].get_exists()),
				owner(o),
				idx(i) {}

		auto operator()() const { return update(); }
		auto update()     const { return Accessor<C>(owner, idx); }

		auto& raw() const {
			return owner.raw[idx];
		}
		
		Const<Dense_Map,C>& owner;
		mutable int idx; // TODO: remove for VECTOR, because key == idx
	};













	//
	// ITERATOR
	//
private:
	template<Const_Flag C>
	class Iterator {
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


		// WARNING: compares indices only
		template<Const_Flag CC>
		bool operator==(const Iterator<CC>& o) const {
			DCHECK_EQ(&owner, &o.owner);
			return idx == o.idx;
		}

		template<Const_Flag CC>
		bool operator!=(const Iterator<CC>& o) const {  return ! (*this == o);  }


		      auto operator*()       {  return Accessor<C>(owner, idx);  }
		const auto operator*() const {  return Accessor<C>(owner, idx);  }


		// unable to implement if using accessors:
		//const auto operator->() const {  return &container[idx];  }
		//      auto operator->()       {  return &container[idx];  }

	private:
		inline void increment() {
			do ++idx; while(idx < (int)owner.raw.size() && !raw().get_exists());
		}

		inline void decrement() {
			do --idx; while(idx > 0 && !raw().exists);
		}


	private:
		Iterator(Const<Dense_Map,C>& o, int i) :
				owner(o), idx(i-o.get_offset()) {
			DCHECK_GE(idx, 0) << "iterator constructor: index out of range";

			// can be equal to container.size() for end() iterators
			DCHECK_LE(idx, owner.raw.size()) << "iterator constructor: index out of range";

			// move forward if element is deleted
			while(idx < (int)owner.raw.size() && !raw().get_exists()) {
				++idx;
			}
		}

		inline auto& raw() const {
			return owner.raw[idx];
		}

	private:
		Const<Dense_Map,C>& owner;
		int idx;

		friend Dense_Map;
	};







	// data
private:

	struct Node : internal::Add_Member_exists<bool(Flags & ERASABLE)> {

		Node() = default;

		template<class... Args>
		Node(Args&&... args) : value(std::forward<Args>(args)...) {
			if constexpr(bool(Flags & ERASABLE)) {
				internal::Add_Member_exists<bool(Flags & ERASABLE)>::exists = true;
			}
		}

		inline bool get_exists() const {
			if constexpr(bool(Flags & ERASABLE)) {
				return internal::Add_Member_exists<bool(Flags & ERASABLE)>::exists;
			}
			else return true;
		}

		Mapped_Type value;
	};

	Container raw;

	inline int get_offset() const {
		if constexpr(TYPE==DEQUE) return internal::Add_Member_offset<TYPE == DEQUE>::offset;
		else return 0;
	}

	//int start_idx = 0; // TODO: to avoid performance issues when begin() is called inside a loop
};


















// BUILDER
template<
	class T,
	Dense_Map_Flags FLAGS = internal::default__Dense_Map_Flags,
	Dense_Map_Type TYPE = internal::default__Dense_Map_Type,
	template<Const_Flag,class,class> class ACCESSOR_TEMPLATE = internal::Default__Dense_Map_Accessor_Template
>
class Dense_Map_Builder {
private:
	template<class TT, Dense_Map_Flags F, Dense_Map_Type TY, template<Const_Flag,class,class> class TMPL>
	using _Dense_Map = Dense_Map<TT,F,TY,TMPL>;

public:
	using Dense_Map = _Dense_Map<T, FLAGS, TYPE, ACCESSOR_TEMPLATE>;

	template<Dense_Map_Type NEW_TYPE>
	using Type = Dense_Map_Builder<T, FLAGS, NEW_TYPE, ACCESSOR_TEMPLATE>;
	
	template<template<Const_Flag,class,class> class NEW_TMPL>
	using Accessor_Template = Dense_Map_Builder<T, FLAGS, TYPE, NEW_TMPL>;


	template<Dense_Map_Flags NEW_FLAGS>
	using Flags           = Dense_Map_Builder<T, NEW_FLAGS, TYPE, ACCESSOR_TEMPLATE>;

	template<Dense_Map_Flags NEW_FLAGS>
	using Add_Flags       = Dense_Map_Builder<T, FLAGS |  NEW_FLAGS, TYPE, ACCESSOR_TEMPLATE>;

	template<Dense_Map_Flags NEW_FLAGS>
	using Remove_Flags    = Dense_Map_Builder<T, FLAGS & ~NEW_FLAGS, TYPE, ACCESSOR_TEMPLATE>;
};













} // namespace smesh



