#pragma once

#include "accessors-common.hpp"

#include <glog/logging.h>

#include <deque>


namespace smesh {



// forward declarations
template<
	class T,
	template<Const_Flag,class,class> class ACCESSOR_TEMPLATE
>
class Dense_Map;
















//
// a vector with some elements marked as deleted - lazy deletion
//
// ACCESSOR_TEMPLATE: should derive from second tmeplate argument; it will be Accessor_Base
//
// TODO: don't call constructors too early
//
// TODO: implement faster vector version (not deque)
//
// TODO: implement a linked version to jump empty spaces faster
//
template<
	class T,
	template<Const_Flag,class,class> class ACCESSOR_TEMPLATE = internal::Index_Accessor_Template
>
class Dense_Map {

	// forward declarations
	public: template<Const_Flag C> class Accessor_Base;


public:
	using Mapped_Type = T;
	template<Const_Flag C> using Accessor = typename Accessor_Base<C>::Derived;


public:
	Dense_Map() {}
	Dense_Map(int new_offset) : offset(new_offset) {}

	decltype(auto) operator[](int key) {
		return Accessor<MUTAB>(*this, key - offset);
	}

	decltype(auto) operator[](int key) const {
		return Accessor<CONST>(*this, key - offset);
	}

	auto domain_begin() const {
		return offset;
	}

	auto domain_end() const {
		return raw.size() + offset;
	}

	template<class VAL>
	inline void push_back(VAL&& val) {
		raw.push_back({true, std::forward<VAL>(val)});
	}

	template<class... Args>
	void emplace_back(Args&&... args) {
		raw.emplace_back(true, {std::forward<Args>(args)... });
	}


	auto begin()       {  return Iterator<MUTAB>(*this, offset);  }
	auto end()         {  return Iterator<MUTAB>(*this, offset + raw.size());  }

	auto begin() const {  return Iterator<CONST>(*this, offset);  }
	auto end()   const {  return Iterator<CONST>(*this, offset + raw.size());  }













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
			DCHECK(key - owner.offset < (int)owner.raw.size());
			DCHECK(raw().exists);
			raw().exists = false;
		}

		operator Const<Mapped_Type,C>&() const {
			return value;
		}

		template<class TT>
		auto operator=(TT&& new_value) const {
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

			if(idx >= (int)owner.raw.size()) {

				//owner.raw.resize((key - owner.offset + 1) * 3/2);
				owner.raw.resize(idx + 1);
			}
			raw().exists = true;
			raw().value = std::forward<TT>(new_value);
			return Accessor<C>(owner, idx);
		}

	// store environment:
	protected:
		Accessor_Base( Const<Dense_Map,C>& o, int i) :
				key(i + o.offset),
				value(o.raw[i].value),
				exists(i < (int)o.raw.size() && i >= 0 && o.raw[i].exists),
				owner(o),
				idx(i) {}

		auto operator()() const { return update(); }
		auto update()     const { return Accessor<C>(owner, idx); }

		auto& raw() const {
			return owner.raw[idx];
		}
		
		Const<Dense_Map,C>& owner;
		mutable int idx;
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
			do ++idx; while(idx < (int)owner.raw.size() && !raw().exists);
		}

		inline void decrement() {
			do --idx; while(idx > 0 && !raw().exists);
		}


	private:
		Iterator(Const<Dense_Map,C>& o, int i) :
				owner(o), idx(i-o.offset) {
			DCHECK_GE(idx, 0) << "iterator constructor: index out of range";

			// can be equal to container.size() for end() iterators
			DCHECK_LE(idx, owner.raw.size()) << "iterator constructor: index out of range";

			// move forward if element is deleted
			while(idx < (int)owner.raw.size() && !raw().exists) {
				++idx;
			}
		}

		inline auto raw() const {
			return owner.raw[idx];
		}

	private:
		Const<Dense_Map,C>& owner;
		int idx;

		friend Dense_Map;
	};







	// data
private:
	struct Node {
		bool exists = false;
		Mapped_Type value;
	};
	std::deque<Node> raw;
	int offset = 0; // to support negative indices
	//int start_idx = 0; // TODO: to avoid performance issues when begin() is called inside a loop
};








} // namespace smesh



