#pragma once



#include "dense-map.hpp"


namespace smesh {






enum class Storage_Type {
	INDEX_16,
	INDEX_32,
	//POINTER // TODO: not implemented
};

namespace {
	constexpr auto INDEX_16 = Storage_Type::INDEX_16;
	constexpr auto INDEX_32 = Storage_Type::INDEX_32;
	//constexpr auto POINTER  = Storage_Type::POINTER;
}




enum class Storage_Flags {
	NONE = 0,
	ERASABLE = 0x0001
};

ENABLE_BITWISE_OPERATORS(Storage_Flags);

namespace {
	constexpr auto STORAGE_ERASABLE = Storage_Flags::ERASABLE;
}












namespace internal {
	constexpr auto default__Storage_Flags = STORAGE_ERASABLE;
	constexpr auto default__Storage_Type = INDEX_32;

	template<Const_Flag C, class OWNER, class BASE>
	using Default__Storage_Accessor_Template = Index_Accessor_Template<C, OWNER, BASE>;


	template<class T,
		Storage_Flags FLAGS,
		template<Const_Flag,class,class> class ACCESSOR_TEMPLATE
	>
	using Dense_Map_Base = typename std::conditional_t<
		bool(FLAGS & STORAGE_ERASABLE),
		typename Dense_Map_Builder<T>::template Add_Flags<ERASABLE>,
		typename Dense_Map_Builder<T>::template Rem_Flags<ERASABLE>>
		::template Type<VECTOR>
		::template Accessor_Template<ACCESSOR_TEMPLATE> :: Dense_Map;
}











//
// currently it's only a proxy between Smesh and Dense_Map
// it will change after (if) pointer handles are implemented
//
template<
	class T,
	Storage_Flags FLAGS = internal::default__Storage_Flags,
	Storage_Type TYPE = internal::default__Storage_Type,
	template<Const_Flag,class,class> class ACCESSOR_TEMPLATE = internal::Default__Storage_Accessor_Template
>
class Storage : public internal::Dense_Map_Base<T, FLAGS, ACCESSOR_TEMPLATE> {

public:
	using internal::Dense_Map_Base<T, FLAGS, ACCESSOR_TEMPLATE> :: domain_end;

private:
	using internal::Dense_Map_Base<T, FLAGS, ACCESSOR_TEMPLATE> :: emplace_back;

public:
	static constexpr auto Flags = FLAGS;
	static constexpr auto Type = TYPE;


public:
	using Key = std::conditional_t<TYPE == INDEX_16, uint16_t, /*u*/int32_t>;


	template<class... Args>
	Storage(Args&&... args)
		: internal::Dense_Map_Base<T, FLAGS, ACCESSOR_TEMPLATE>(std::forward<Args>(args)... ) {}



	template<class... Args>
	auto add(Args&&... args) {
		DCHECK_LE(domain_end(), std::numeric_limits<Key>::max());
		//Key h = raw.domain_max();
		//int idx = raw.domain_end();
		return emplace_back(std::forward<Args>(args)... );
		//return h;
	}
};

















// BUILDER

template<
	class T,
	Storage_Flags FLAGS = internal::default__Storage_Flags,
	Storage_Type TYPE = internal::default__Storage_Type,
	template<Const_Flag,class,class> class ACCESSOR_TEMPLATE = internal::Default__Storage_Accessor_Template
>
class Storage_Builder {
private:
	template<class TT, Storage_Flags F, Storage_Type TY, template<Const_Flag,class,class> class TMPL>
	using _Storage = Storage<TT,F,TY,TMPL>;

public:
	using Storage = _Storage<T, FLAGS, TYPE, ACCESSOR_TEMPLATE>;

	template<Storage_Type NEW_TYPE>
	using Type = Storage_Builder<T, FLAGS, NEW_TYPE, ACCESSOR_TEMPLATE>;
	
	template<template<Const_Flag,class,class> class NEW_TMPL>
	using Accessor_Template = Storage_Builder<T, FLAGS, TYPE, NEW_TMPL>;


	template<Storage_Flags NEW_FLAGS>
	using Flags           = Storage_Builder<T, NEW_FLAGS, TYPE, ACCESSOR_TEMPLATE>;

	template<Storage_Flags NEW_FLAGS>
	using Add_Flags       = Storage_Builder<T, FLAGS |  NEW_FLAGS, TYPE, ACCESSOR_TEMPLATE>;

	template<Storage_Flags NEW_FLAGS>
	using Rem_Flags       = Storage_Builder<T, FLAGS & ~NEW_FLAGS, TYPE, ACCESSOR_TEMPLATE>;
};













}

