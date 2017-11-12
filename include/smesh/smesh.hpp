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
enum class Smesh_Flags {
	NONE = 0,
	VERTS_LAZY_DEL = 0x0001,
	POLYS_LAZY_DEL = 0x0002
};

ENABLE_BITMASK_OPERATORS(Smesh_Flags)


struct Void {};




// optional `del` member
template<bool> struct Add_Member_del { bool del = false; };
template<>     struct Add_Member_del<false> {};


struct Smesh_Options {
	Void Vert_Props();
	Void Poly_Props();
	Void Poly_Vert_Props();
};

//
// a triangle mesh structure with edge links (half-edges)
//
// - lazy removal of vertices or polygons (see bool del flag). TODO: remap support when no lazy removal
//
template <
	class POS_FLOAT,
	Smesh_Flags FLAGS =
		Smesh_Flags::VERTS_LAZY_DEL |
		Smesh_Flags::POLYS_LAZY_DEL,
	class OPTIONS = Smesh_Options
>
class Smesh {

public:
	using Pos_Float = POS_FLOAT;
	using Pos = Eigen::Matrix<Pos_Float,3,1>;
	using Idx = int32_t; // vertex index type

	using Vert_Props = decltype(OPTIONS().Vert_Props());
	using Poly_Props = decltype(OPTIONS().Poly_Props());
	using Poly_Vert_Props = decltype(OPTIONS().Poly_Vert_Props());

	static constexpr Smesh_Flags Flags = FLAGS;


	static const int POLY_SIZE = 3;



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
	struct Vert : public Vert_Props, public Add_Member_del<bool(Flags & Smesh_Flags::VERTS_LAZY_DEL)> {
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
	
	struct Poly : public Poly_Props, public Add_Member_del<bool(Flags & Smesh_Flags::POLYS_LAZY_DEL)> {
		Poly_Vert verts[POLY_SIZE];
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
	template<class A, class T, class EXTRA, class GET_A> class   Iterator;
	template<class A, class T, class EXTRA, class GET_A> class C_Iterator;


private:
	//
	// iterator
	// wraps T*, returns accessor A
	//
	// it's not random access because of `del` flags
	//
	template<class A, class T, class EXTRA = Smesh&, class GET_A = A>
	class Iterator {
	public:
		Iterator& operator++() {
			increment();
			return *this; }

		Iterator operator++(int) {
			auto old = *this;
			increment();
			return old; }

		Iterator& operator--() {
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


		template<class AA, class EE, class GG>
		bool operator==(const C_Iterator<AA,T,EE,GG>& o) const {
			return p == o.p;
		}

		template<class AA, class EE, class GG>
		bool operator!=(const C_Iterator<AA,T,EE,GG>& o) const {
			return ! (*this == o);
		}


		A operator*() const {
			return GET_A(extra, *p);
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
		Iterator(const EXTRA& a_extra, T* a_p, T* a_begin, T* a_end) :
			extra(a_extra), p(a_p), begin(a_begin), end(a_end) {}

		EXTRA extra;
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
	template<class A, class T, class EXTRA = Smesh&, class GET_A = A>
	class C_Iterator {
	public:
		C_Iterator& operator++() {
			++p;
			return *this; }

		C_Iterator operator++(int) {
			auto old = *this;
			++p;
			return old; }

		C_Iterator& operator--() {
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


		template<class AA, class EE, class GG>
		bool operator==(const Iterator<AA,T,EE,GG>& o) const {
			return p == o.p;
		}

		template<class AA, class EE, class GG>
		bool operator!=(const Iterator<AA,T,EE,GG>& o) const {
			return ! (*this == o);
		}


		const A operator*() const {
			return GET_A(extra, *p); }


	private:
		inline void increment() {
			do ++p; while(p < end && _smesh_detail::is_deleted(*p));
		}

		inline void decrement() {
			do --p; while(p > begin && _smesh_detail::is_deleted(*p)); // I believe decrementing `begin` is undefined anyway
		}

	private:
		// need to store `begin` and `end` because of `del` flags
		C_Iterator(EXTRA& a_extra,
			const T* a_p, const T* a_begin, const T* a_end) :
				extra(a_extra), p(a_p), begin(a_begin), end(a_end) {}

		EXTRA extra;
		const T* p;
		const T* const begin;
		const T* const end;

		friend Smesh;
	};






private:
	template<class A, class CONTAINER_REF, class EXTRA = Smesh&, class GET_A = A>
	class Index_Iterator {
	public:
		Index_Iterator& operator++() {
			++idx;
			return *this; }

		Index_Iterator operator++(int) {
			auto old = *this;
			++idx;
			return old; }

		Index_Iterator& operator--() {
			--idx;
			return *this; }

		Index_Iterator operator--(int) {
			auto old = *this;
			--idx;
			return old; }


		// warning! safe to compare only iterators to same container (optimization)
		bool operator==(const Index_Iterator& o) const {
			return idx == o.idx; }

		bool operator!=(const Index_Iterator& o) const {
			return ! (*this == o); }


		template<class AA, class EE, class GG>
		bool operator==(const Index_Iterator<AA,CONTAINER_REF,EE,GG>& o) const {
			return idx == o.idx;
		}

		template<class AA, class EE, class GG>
		bool operator!=(const Index_Iterator<AA,CONTAINER_REF,EE,GG>& o) const {
			return ! (*this == o);
		}


		const A operator*() const {
			std::cout << "asd " << idx << " " << container[idx].idx << " " << container << std::endl;
			return GET_A(extra, container, idx); }


	private:
		Index_Iterator(const EXTRA& ex, const CONTAINER_REF co, int i) :
				extra(ex), container(co), idx(i) {
			std::cout << "dupa0 " << 0 << " " << co[0].idx << " " << co << std::endl;
			std::cout << "dupa3 " << 3 << " " << co[3].idx << " " << co << std::endl;
			DCHECK_GE(i, 0) << "iterator constructor: index out of range";
		}

	private:
		EXTRA extra;
		const CONTAINER_REF container;
		int idx;

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
	using  I_Vert =   Iterator< A_Vert, Vert>;
	using CI_Vert = C_Iterator<CA_Vert, Vert>;


	class  A_Poly;
	class CA_Poly;
	using  I_Poly =   Iterator< A_Poly, Poly>;
	using CI_Poly = C_Iterator<CA_Poly, Poly>;



	class A_Poly_Verts;

	class  A_Poly_Vert;
	class CA_Poly_Vert;
	using  I_Poly_Vert = Index_Iterator< A_Poly_Vert, Poly_Vert*>;
	using CI_Poly_Vert = Index_Iterator<CA_Poly_Vert, Poly_Vert*>;



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
		
		A_Vert add(const Pos_Float& x, const Pos_Float& y, const Pos_Float& z) {
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


		inline void check_idx(int idx) {
			DCHECK_GE(idx, 0) << "vertex index out of range";
			DCHECK_LT(idx, (int)smesh.raw_verts.size()) << "vertex index out of range";
		}

		
		A_Vert operator[](int idx) {
			check_idx(idx);
			return A_Vert{smesh, smesh.raw_verts[idx]};
		}

		CA_Vert operator[](int idx) const {
			check_idx(idx);
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

			if constexpr(bool(Flags & Smesh_Flags::VERTS_LAZY_DEL))
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

			if constexpr(bool(Flags & Smesh_Flags::POLYS_LAZY_DEL))
				DCHECK( !poly.del ) << "polygon is deleted";
		}
		Poly& poly;

		friend Smesh;
	};


	class CA_Poly {
	public:

		Poly_Props& props;

		A_Poly_Verts verts;


	private:
		CA_Poly( const Smesh& a_smesh, const Poly& a_poly )  :
				props(a_poly),
				verts(a_smesh, a_poly),
				poly(a_poly) {

			if constexpr(bool(Flags & Smesh_Flags::POLYS_LAZY_DEL))
				DCHECK( !poly.del ) << "polygon is deleted";
		}
		const Poly& poly;

		friend Smesh;
	};











public:
	class A_Poly_Verts {
	public:
		constexpr int size() const {
			return POLY_SIZE;
		}

		A_Poly_Vert operator[]( int i ) {
			check_idx(i);
			return A_Poly_Vert( smesh, poly.verts, i );
		}

		CA_Poly_Vert operator[]( int i ) const {
			check_idx(i);
			return CA_Poly_Vert( smesh, poly.verts, i );
		}

		I_Poly_Vert begin() {
			return I_Poly_Vert( smesh, poly.verts, 0 );
		}

		I_Poly_Vert end() {
			return I_Poly_Vert( smesh, poly.verts, POLY_SIZE );
		}

	private:
		inline void check_idx(int i) {
			DCHECK_GE(i, 0) << "A_Poly_Verts::operator[]: index out of range";
			DCHECK_LT(i, POLY_SIZE) << "A_Poly_Verts::operator[]: index out of range";
		}

	private:
		A_Poly_Verts( Smesh& s, Poly& p ) : smesh(s), poly(p) {}
		Smesh& smesh;
		Poly& poly;

		friend Smesh;
	};











public:
	class A_Poly_Vert {
	public:
		Pos& pos;

		A_Vert vert;

		Poly_Vert_Props& props;

		A_Poly_Vert prev() const {
			int n = POLY_SIZE;//verts.size();
			int new_pv_idx = (pv_idx + n - 1) % n;
			return A_Poly_Vert(smesh, verts, new_pv_idx);
		}

		A_Poly_Vert next() const {
			int n = POLY_SIZE;//verts.size();
			int new_pv_idx = (pv_idx + 1) % n;
			return A_Poly_Vert(smesh, verts, new_pv_idx);
		}


	private:
		inline void check_idx(int i) {
			DCHECK_GE(i, 0) << "A_Poly_Verts::operator[]: index out of range";
			DCHECK_LT(i, POLY_SIZE) << "A_Poly_Verts::operator[]: index out of range";
		}

		A_Poly_Vert( Smesh& m, Poly_Vert vs[], int i ) :
				pos( m.raw_verts[ vs[i].idx ].pos ),
				vert( m, m.raw_verts[ vs[i].idx ] ),
				props( vs[i] ),
				smesh(m),
				verts(vs),
				pv_idx( i ) {

			check_idx(i);
		}

		Smesh& smesh;

		Poly_Vert* verts;
		const int pv_idx;

		friend Smesh;
	};




	class CA_Poly_Vert {
	public:
		const Pos& pos;

		CA_Vert vert;

		const Poly_Vert_Props& props;

		CA_Poly_Vert prev() const {
			int n = verts.size();
			int new_pv_idx = (pv_idx + n - 1) % n;
			return CA_Poly_Vert(smesh, verts, new_pv_idx);
		}

		CA_Poly_Vert next() const {
			int n = verts.size();
			int new_pv_idx = (pv_idx + 1) % n;
			return CA_Poly_Vert(smesh, verts, new_pv_idx);
		}

	private:
		CA_Poly_Vert( const Smesh& m, const Poly_Vert vs[], int i ) :
			pos( m.raw_verts[ vs[i].idx ].pos ),
			vert( m, m.raw_verts[ vs[i].idx ] ),
			props( vs[i] ),
			smesh(m),
			verts(vs),
			pv_idx( i ) {}

		const Smesh& smesh;

		const Poly_Vert verts[];
		const int pv_idx;

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


