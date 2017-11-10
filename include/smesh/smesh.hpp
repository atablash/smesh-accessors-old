#pragma once

#include <Eigen/Dense>

#include <glog/logging.h>

#include "common.hpp"

//#include <atablash/member-detector.hpp>




namespace smesh {



//
// helper
//

namespace _smesh_detail {

GENERATE_HAS_MEMBER(del);

template<class T, class = std::enable_if_t<has_member_del<T>::value> >
inline std::enable_if_t< has_member_del<T>::value, bool>
is_deleted(const T& t)
//requires requires(T t) { {t.del} -> bool }	// compile with concepts lite
{
	return t.del;
}

template<class T>
//requires !requires(T t) { {t.del} -> bool }	// compile with concepts lite
inline std::enable_if_t< ! has_member_del<T>::value, bool>
is_deleted(const T& t)
{
	return false;
}

} // namespace _smesh_detail






//
// mesh static flags
//
enum class Smesh_Options {
	NONE = 0,
	VERTS_LAZY_DEL = 0x0001,
	POLYS_LAZY_DEL = 0x0002
};

ENABLE_BITMASK_OPERATORS(Smesh_Options)


struct Void {};




// optional `del` member
template<bool> struct Add_Member_del { bool del = false; };
template<>     struct Add_Member_del<false> {};




//
// a triangle mesh structure with edge links (half-edges)
//
// - lazy removal of vertices or polygons (see bool del flag). TODO: remap support when no lazy removal
//
template <class F,
	Smesh_Options OPTS =
		Smesh_Options::VERTS_LAZY_DEL |
		Smesh_Options::POLYS_LAZY_DEL,
	class Vert_Props = Void,
	class Poly_Props = Void,
	class Poly_Vert_Props = Void
>
class Smesh {
	using Pos = Eigen::Matrix<F,3,1>;
	using Idx = int32_t; // vertex index type



	//
	// handles
	// TODO: implement small handles
	//
public:
	//
	// a handle to either polygon's vertex or edge (half-edge)
	//
	struct H_poly_vert{
		int32_t poly = -1;
		int8_t vert = -1; // 0, 1 or 2
	};






//
// internal structure types
//
private:
	struct Vert : public Vert_Props, public Add_Member_del<bool(OPTS & Smesh_Options::VERTS_LAZY_DEL)> {
		Vert() {}
		
		template<class A_POS>
		Vert(A_POS && a_pos) : pos( std::forward<A_POS>(a_pos) ) {}
		
		Pos pos = {0,0,0};
		//std::vector<H_poly_vert> polygons;
	};
	
	struct Poly_Vert : public Poly_Vert_Props { // vertex and corresponidng edge
		Idx idx; // vertex index
		//int32_t idx_prop; // property-vertex index
		H_poly_vert elink; // link to the other half-edge
	};
	
	struct Poly : public Poly_Props, public Add_Member_del<bool(OPTS & Smesh_Options::POLYS_LAZY_DEL)> {
		Poly_Vert verts[3];
	};
	
	

//
// internal data
//
private:
	std::vector<Vert> raw_verts;
	std::vector<Poly> raw_polys;


//
// copy and move contruction
// (be careful not to copy/move AVertices and A_polys)
//
public:
	Smesh() {}
	
	Smesh(const Smesh& o) : raw_verts(o.raw_verts), raw_polys(o.raw_polys) {}
	Smesh(Smesh&& o) : raw_verts(std::move(o.raw_verts)), raw_polys(std::move(o.raw_polys)) {}
	
	Smesh& operator=(const Smesh& o) {
		raw_verts = o.raw_verts;
		raw_polys = o.raw_polys;
	}
	
	
	
	






	//
	// private typedefs - used for defining iterators to various things
	//
private:
	template<class A, class T> class   Iterator;
	template<class A, class T> class C_Iterator;


private:
	//
	// iterator
	// wraps T*, returns accessor A
	//
	// it's not random access because of `del` flags
	//
	template<class A, class T>
	class Iterator {
	public:
		Iterator& operator++() {
			increment();
			return *this; }

		Iterator operator++(int) {
			auto old = *this;
			increment();
			return old; }

		Iterator operator--() {
			decrement();
			return *this; }

		Iterator operator--(int) {
			auto old = *this;
			decrement();
			return old; }


		bool operator==(const Iterator& o) const {
			return p == o.p; }

		bool operator!=(const Iterator& o) const {
			return ! (*this == o); }


		bool operator==(const C_Iterator<A,T>& o) const {
			return p == o.p;
		}

		bool operator!=(const C_Iterator<A,T>& o) const {
			return ! (*this == o);
		}


		A operator*() const {
			return A(smesh, *p);
		}


	private:
		inline void increment() {
			do ++p; while(p < end && _smesh_detail::is_deleted(*p));
		}

		inline void decrement() {
			do --p; while(p > begin && _smesh_detail::is_deleted(*p)); // I believe decrementing `begin` is undefined anyway
		}

	private:
		// need to store `begin` and `end` because of `del` flags
		Iterator(Smesh& smesh_, T* p_, T* begin_, T* end_) :
			smesh(smesh_), p(p_), begin(begin_), end(end_) {}

		Smesh& smesh;
		T* p;
		const T* const begin;
		const T* const end;

		friend Smesh;
	};





private:
	//
	// const iterator
	// wraps const T*, returns accessor const A
	//
	template<class A, class T>
	class C_Iterator {
	public:
		C_Iterator& operator++() {
			++p;
			return *this; }

		C_Iterator operator++(int) {
			auto old = *this;
			++p;
			return old; }

		C_Iterator operator--() {
			--p;
			return *this; }

		C_Iterator operator--(int) {
			auto old = *this;
			--p;
			return old; }


		bool operator==(const C_Iterator& o) const {
			return p == o.p; }

		bool operator!=(const C_Iterator& o) const {
			return ! (*this == o); }


		bool operator==(const Iterator<A,T>& o) const {
			return p == o.p;
		}

		bool operator!=(const Iterator<A,T>& o) const {
			return ! (*this == o);
		}


		// we're returning const accessor, so it's safe to const_cast
		const A operator*() const {
			return A(const_cast<Smesh&>(smesh), const_cast<T&>(*p)); }


	private:
		inline void increment() {
			do ++p; while(p < end && _smesh_detail::is_deleted(*p));
		}

		inline void decrement() {
			do --p; while(p > begin && _smesh_detail::is_deleted(*p)); // I believe decrementing `begin` is undefined anyway
		}

	private:
		// need to store `begin` and `end` because of `del` flags
		C_Iterator(const Smesh& smesh_,
			const T* p_, const T* begin_, const T* end_) :
				smesh(smesh_), p(p_), begin(begin_), end(end_) {}

		const Smesh& smesh;
		const T* p;
		const T* const begin;
		const T* const end;

		friend Smesh;
	};







////////////////////////////////////////////////////////////////////////////////
//
// ACCESSORS
//
////////////////////////////////////////////////////////////////////////////////
public:
	
	class A_Verts;
	class A_Polys;


	class  A_Vert;
	class CA_Vert;
	using  I_Vert =   Iterator<A_Vert, Vert>;
	using CI_Vert = C_Iterator<A_Vert, Vert>;


	class  A_Poly;
	using  I_Poly =   Iterator<A_Poly, Poly>;
	using CI_Poly = C_Iterator<A_Poly, Poly>;



	class A_Poly_Verts;

	class  A_Poly_Vert;
	class CA_Poly_Vert;
	using  I_Poly_Vert =   Iterator<A_Poly_Vert, Poly_Vert>;
	using CI_Poly_Vert = C_Iterator<A_Poly_Vert, Poly_Vert>;



	class A_Poly_Edges;

	class A_Poly_Edge;
	



	
	class A_Verts {
	public:
		A_Vert add() {
			smesh.raw_verts.emplace_back();
			return A_Vert{smesh, smesh.raw_verts.back()};
		}
		
		template<class POS_>
		A_Vert add(POS_&& pos) {
			smesh.raw_verts.emplace_back(pos);
			return A_Vert{smesh, smesh.raw_verts.back()};
		}
		
		A_Vert add(F x, F y, F z) {
			smesh.raw_verts.emplace_back(Pos{x,y,z});
			return A_Vert(smesh, smesh.raw_verts.back());
		}
		
		void reserve(int n) {
			smesh.raw_verts.reserve(n);
		}

		int size_including_deleted() const {
			return smesh.raw_verts.size();
		}

		// todo: implement size() - take deleted verts into account!

		
		A_Vert operator[](int idx) {
			DCHECK(0 <= idx && idx < (int)smesh.raw_verts.size()) << "vertex index out of range";
			return A_Vert{smesh, smesh.raw_verts[idx]};
		}

		CA_Vert operator[](int idx) const {
			DCHECK(0 <= idx && idx < (int)smesh.raw_verts.size()) << "vertex index out of range";
			return CA_Vert{smesh, smesh.raw_verts[idx]};
		}


		// iterator
		I_Vert begin() {
			return I_Vert(smesh, &*smesh.raw_verts.begin(), &*smesh.raw_verts.begin(), &*smesh.raw_verts.end());
		}

		I_Vert end() {
			return I_Vert(smesh, &*smesh.raw_verts.end(), &*smesh.raw_verts.begin(), &*smesh.raw_verts.end());
		}

		// const iterator
		CI_Vert begin() const {
			return CI_Vert(smesh, &*smesh.raw_verts.begin(), &*smesh.raw_verts.begin(), &*smesh.raw_verts.end());
		}

		CI_Vert end() const {
			return CI_Vert(smesh, &*smesh.raw_verts.end(), &*smesh.raw_verts.begin(), &*smesh.raw_verts.end());
		}


	// store environment:
	private:
		A_Verts(Smesh& a_smesh) : smesh(a_smesh) {}
		Smesh& smesh;
		
		friend Smesh;
	};
	
	A_Verts verts = A_Verts(*this);
	
	
	
	
	
	
	class A_Polys {
	public:
		A_Poly add(Idx i0, Idx i1, Idx i2) {
			Poly p;
			p.verts[0].idx = i0;
			p.verts[1].idx = i1;
			p.verts[2].idx = i2;
			smesh.raw_polys.emplace_back(std::move(p));
			return A_Poly(smesh, smesh.raw_polys.back());
		}
		
		void reserve( int n ) {
			smesh.raw_polys.reserve(n);
		}

		int size_including_deleted() const {
			return smesh.raw_polys.size();
		}

		// TODO: account for deleted - implement size()
		
		A_Poly operator[]( int idx ) {
			DCHECK(0 <= idx && idx < (int)smesh.raw_polys.size()) << "polygon index out of range";
			return A_poly(smesh, smesh.raw_polys[idx]);
		}

		const A_Poly operator[]( int idx ) const {
			DCHECK( 0 <= idx  &&  idx < (int)smesh.raw_polys.size() ) << "polygon index out of range";
			return A_poly(smesh, smesh.raw_polys[idx]);
		}


		// iterator
		I_Poly begin() {
			return I_Poly(smesh, &*smesh.raw_polys.begin(), &*smesh.raw_polys.begin(), &*smesh.raw_polys.begin());
		}

		I_Poly end() {
			return I_Poly(smesh, &*smesh.raw_polys.end(), &*smesh.raw_polys.begin(), &*smesh.raw_polys.begin());
		}

		// const iterator
		CI_Poly begin() const {
			return CI_Poly(smesh, &*smesh.raw_polys.begin(), &*smesh.raw_polys.begin(), &*smesh.raw_polys.begin());
		}

		CI_Poly end() const {
			return CI_Poly(smesh, &*smesh.raw_polys.end(), &*smesh.raw_polys.begin(), &*smesh.raw_polys.begin());
		}
		
	// store environment:
	private:
		A_Polys( Smesh& a_smesh ) : smesh(a_smesh) {}
		Smesh& smesh;
		
		friend Smesh;
	};
	
	A_Polys polys = A_Polys(*this);
	
	
	



	
	// accessor
	class A_Vert {
	public:

		Pos& pos;
		Vert_Props& props;

		int idx;

		void remove() {
			DCHECK( !vert.del );
			vert.del = true;
		}

	// store environment:
	private:
		A_Vert( Smesh& a_smesh,  Vert& a_vert)  :
				pos(    a_vert.pos ),
				props(  a_vert ),
				idx(    &a_vert - &a_smesh.raw_verts[0] ),
				vert(   a_vert ) {

			if constexpr(bool(OPTS & Smesh_Options::VERTS_LAZY_DEL))
				DCHECK(!vert.del) << "vertex is already deleted";
		}
		
		//Smesh& smesh;
		Vert& vert;
		
		friend Smesh;
	};


	class CA_Vert {
	public:

		const Pos& pos;
		const Vert_Props& props;

		int idx;

	// store environment:
	private:
		CA_Vert( const Smesh& a_smesh, const Vert& a_vert)  :
				pos(    a_vert.pos ),
				props(  a_vert ),
				idx(    &a_vert - &a_smesh.raw_verts[0] ),
				vert(   a_vert ) {

			DCHECK(!vert.del) << "vertex is already deleted";
		}
		
		//Smesh& smesh;
		const Vert& vert;
		
		friend Smesh;
	};


	
















public:
	class A_Poly {
	public:

		Poly_Props& props;

		void remove() {
			DCHECK( !poly.del );
			poly.del = true;
		}

		A_Poly_Verts verts;


	private:
		A_Poly( Smesh& a_smesh,  Poly& a_poly )  :
				props(a_poly),
				verts(a_smesh, a_poly),
				poly(a_poly) {

			if constexpr(bool(OPTS & Smesh_Options::POLYS_LAZY_DEL))
				DCHECK( !poly.del ) << "polygon is deleted";
		}
		Poly& poly;

		friend Smesh;
	};










public:
	class A_Poly_Verts {
	public:
		int size() const {
			return 3;
		}

		A_Poly_Vert operator[]( int i ) {
			return A_Poly_Vert( smesh,  poly.verts[i] );
		}

		CA_Poly_Vert operator[]( int i ) const {
			return CA_Poly_Vert( smesh,  poly.verts[i] );
		}

		I_Poly_Vert begin() {
			return I_Poly_Vert( smesh, &poly.verts[0],  &poly.verts[0],  &poly.verts[3] );
		}

		I_Poly_Vert end() {
			return I_Poly_Vert( smesh, &poly.verts[3],  &poly.verts[0],  &poly.verts[3] );
		}

	private:
		A_Poly_Verts( Smesh& a_smesh,  Poly& a_poly )  :  smesh( a_smesh ),  poly( a_poly )  {}
		Smesh&  smesh;
		Poly&  poly;

		friend Smesh;
	};











public:
	class A_Poly_Vert {
	public:
		Pos& pos;

		A_Vert vert;

		Poly_Vert_Props& props;

	private:
		A_Poly_Vert( Smesh& a_smesh,  Poly_Vert&  a_poly_vert ) :
			pos( a_smesh.raw_verts[ a_poly_vert.idx ].pos ),
			vert( a_smesh, a_smesh.raw_verts[ a_poly_vert.idx ] ),
			props( a_poly_vert ),
			smesh( a_smesh ),
			poly_vert( a_poly_vert ) {}

		Smesh& smesh;
		Poly_Vert& poly_vert;

		friend Smesh;
	};




	class CA_Poly_Vert {
	public:
		const Pos& pos;

		CA_Vert vert;

		const Poly_Vert_Props& props;

	private:
		CA_Poly_Vert( const Smesh& a_smesh, const Poly_Vert& a_poly_vert ) :
			pos( a_smesh.raw_verts[ a_poly_vert.idx ].pos ),
			vert( a_smesh, a_smesh.raw_verts[ a_poly_vert.idx ] ),
			props( a_poly_vert ),
			smesh( a_smesh ),
			poly_vert( a_poly_vert ) {}

		const Smesh& smesh;
		const Poly_Vert& poly_vert;

		friend Smesh;
	};

















	
public:
	class A_Poly_Edge { // or half-edge
	public:
		A_Poly_Edge link() {
			const auto& l = poly_vert.elink;
			auto& poly = smesh.raw_polys[ l.poly ];
			return A_Poly_Edge( smesh,  poly,  poly.verts[ l.vert ] );
		}

		const A_Poly_Edge link() const {
			const auto& l = poly_vert.elink;
			auto& poly = smesh.raw_polys[ l.poly ];
			return A_Poly_Edge( smesh,  poly,  poly.verts[ l.vert ] );
		}


		A_Poly poly() {
			return A_Poly( smesh,  my_poly );
		}

		const A_Poly poly() const {
			return A_Poly( smesh,  my_poly );
		}
 
	private:
		A_Poly_Edge( Smesh& a_smesh,  Poly& a_poly,  Poly_Vert& a_poly_vert ) :
			smesh( a_smesh ),
			my_poly( a_poly ),
			poly_vert( a_poly_vert )  {}

		Smesh& smesh;
		Poly& my_poly;
		Poly_Vert&  poly_vert;

		friend Smesh;
	};
	
	
}; // class Smesh
	






} // namespace smesh


