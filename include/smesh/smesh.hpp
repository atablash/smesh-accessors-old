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



private:

	// internal raw structs for storage
	struct Vert;
	struct Poly;
	struct Poly_Vert;



private:

	template<class A, class CONTAINER, class EXTRA = Smesh&, class GET_A = A>
	class Index_Iterator;


public:
	class A_Verts;
	class A_Polys;


	class  A_Vert;
	class CA_Vert;
	using  I_Vert =   Index_Iterator< A_Vert, std::vector<Vert>>;
	using CI_Vert =   Index_Iterator<CA_Vert, std::vector<Vert>>;


	class  A_Poly;
	class CA_Poly;
	using  I_Poly =   Index_Iterator< A_Poly, std::vector<Poly>>;
	using CI_Poly =   Index_Iterator<CA_Poly, std::vector<Poly>>;



	class  A_Poly_Verts;
	class CA_Poly_Verts;

	class  A_Poly_Vert;
	class CA_Poly_Vert;

	template<class A, class MESH>
	struct Get_A_Double_Idx {
		Get_A_Double_Idx(const std::pair<MESH&,int>& p, int i1) :
			smesh(p.first), idx0(p.second), idx1(i1) {}

		operator A() {
			return A(smesh, idx0, idx1);
		}

	private:
		MESH& smesh;
		int idx0;
		int idx1;
	};

	// extra is Smesh& and int (poly index)
	using  I_Poly_Vert = Index_Iterator< A_Poly_Vert, std::array<Poly_Vert, POLY_SIZE>,
		std::pair<Smesh&,int>, Get_A_Double_Idx<A_Poly_Vert, Smesh>>;

	using CI_Poly_Vert = Index_Iterator<CA_Poly_Vert, std::array<Poly_Vert, POLY_SIZE>,
		std::pair<const Smesh&,int>, Get_A_Double_Idx<CA_Poly_Vert, const Smesh>>;



	class  A_Poly_Edges;
	class CA_Poly_Edges;

	class  A_Poly_Edge;
	class CA_Poly_Edge;

	using  I_Poly_Edge = Index_Iterator< A_Poly_Edge, std::array<Poly_Vert, POLY_SIZE>,
		std::pair<Smesh&,int>, Get_A_Double_Idx<A_Poly_Edge, Smesh>>;

	using CI_Poly_Edge = Index_Iterator<CA_Poly_Edge, std::array<Poly_Vert, POLY_SIZE>,
		std::pair<const Smesh&,int>, Get_A_Double_Idx<CA_Poly_Edge, const Smesh>>;







	//
	// handles
	// TODO: implement small handles
	//
public:
	//
	// a handle to either polygon's vertex or edge (half-edge)
	//
	struct H_Poly_Vert{
		int32_t poly = -1;
		int8_t vert = -1; // 0, 1 or 2
	};

	struct H_Poly_Edge {
		int32_t poly = -1;
		int8_t edge = -1; // 0, 1 or 2

		A_Poly_Edge get(Smesh& smesh) {
			return A_Poly_Edge(smesh, poly, edge);
		}
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
		//std::vector<H_Poly_Vert> polygons;
	};
	
	struct Poly_Vert : public Poly_Vert_Props { // vertex and corresponidng edge
		Idx idx; // vertex index
		//int32_t idx_prop; // property-vertex index
		H_Poly_Vert elink; // link to the other half-edge
	};
	
	struct Poly : public Poly_Props, public Add_Member_del<bool(Flags & Smesh_Flags::POLYS_LAZY_DEL)> {
		std::array<Poly_Vert, POLY_SIZE> verts;
	};
	
	

//
// internal data
//
private:
	std::vector<Vert> raw_verts;
	int raw_verts_deleted = 0;

	std::vector<Poly> raw_polys;
	int raw_polys_deleted = 0;


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
	
	
	




private:
	template<class A, class CONTAINER, class EXTRA, class GET_A>
	class Index_Iterator {
	public:
		Index_Iterator& operator++() {
			increment();
			return *this; }

		Index_Iterator operator++(int) {
			auto old = *this;
			increment();
			return old; }

		Index_Iterator& operator--() {
			decrement();
			return *this; }

		Index_Iterator operator--(int) {
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
		}

	private:
		EXTRA extra;
		const CONTAINER& container;
		int idx;

		friend Smesh;
	};





////////////////////////////////////////////////////////////////////////////////
//
// ACCESSORS
//
////////////////////////////////////////////////////////////////////////////////
public:

	
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
			return A_Vert(smesh, smesh.raw_verts.size()-1);
		}
		
		void reserve(int n) {
			smesh.raw_verts.reserve(n);
		}

		int size_including_deleted() const {
			return smesh.raw_verts.size();
		}

		int size() const {
			if constexpr(!bool(Flags & Smesh_Flags::VERTS_LAZY_DEL)) return smesh.raw_verts.size();

			return smesh.raw_verts.size() - smesh.raw_verts_deleted;
		}


		inline void check_idx(int idx) const {
			DCHECK_GE(idx, 0) << "vertex index out of range";
			DCHECK_LT(idx, (int)smesh.raw_verts.size()) << "vertex index out of range";
		}

		
		A_Vert operator[](int idx) {
			check_idx(idx);
			return A_Vert{smesh, idx};
		}

		
		const CA_Vert operator[](int idx) const {
			check_idx(idx);
			return CA_Vert{smesh, idx};
		}
		


		// iterator
		I_Vert begin() {
			return I_Vert(smesh, smesh.raw_verts, 0);
		}

		I_Vert end() {
			return I_Vert(smesh, smesh.raw_verts, smesh.raw_verts.size());
		}

		// const iterator
		CI_Vert begin() const {
			return CI_Vert(smesh, smesh.raw_verts, 0);
		}

		CI_Vert end() const {
			return CI_Vert(smesh, smesh.raw_verts, smesh.raw_verts.size());
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
			return A_Poly(smesh, smesh.raw_polys.size()-1);
		}
		
		void reserve( int n ) {
			smesh.raw_polys.reserve(n);
		}

		int size_including_deleted() const {
			return smesh.raw_polys.size();
		}

		int size() const {

			if constexpr(!bool(Flags & Smesh_Flags::POLYS_LAZY_DEL)) {
				return smesh.raw_polys.size();
			}

			return smesh.raw_polys.size() - smesh.raw_polys_deleted;
		}
		
		A_Poly operator[]( int idx ) {
			DCHECK(0 <= idx && idx < (int)smesh.raw_polys.size()) << "polygon index out of range";
			return A_poly(smesh, smesh.raw_polys[idx]);
		}

		const CA_Poly operator[]( int idx ) const {
			DCHECK( 0 <= idx  &&  idx < (int)smesh.raw_polys.size() ) << "polygon index out of range";
			return CA_poly(smesh, smesh.raw_polys[idx]);
		}


		// iterator
		I_Poly begin() {
			return I_Poly(smesh, smesh.raw_polys, 0);
		}

		I_Poly end() {
			return I_Poly(smesh, smesh.raw_polys, smesh.raw_polys.size());
		}

		// const iterator
		CI_Poly begin() const {
			return CI_Poly(smesh, smesh.raw_polys, 0);
		}

		CI_Poly end() const {
			return CI_Poly(smesh, smesh.raw_polys, smesh.raw_polys.size());
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

		const int idx;

		void remove() {
			DCHECK( !smesh.raw_verts[idx].del );
			smesh.raw_verts[idx].del = true;
			++smesh.raw_verts_deleted;
		}

	// store environment:
	private:
		A_Vert( Smesh& m, int v)  :
				pos( m.raw_verts[v].pos ),
				props( m.raw_verts[v] ),
				idx(v),
				smesh(m) {

			if constexpr(bool(Flags & Smesh_Flags::VERTS_LAZY_DEL))
				DCHECK(!m.raw_verts[v].del) << "vertex is already deleted";
		}
		
		Smesh& smesh;
		
		friend Smesh;
	};



	class CA_Vert {
	public:

		const Pos& pos;
		const Vert_Props& props;

		const int idx;

	// store environment:
	private:
		CA_Vert( const Smesh& m, int v)  :
				pos( m.raw_verts[v].pos ),
				props( m.raw_verts[v] ),
				idx(v),
				smesh(m) {

			if constexpr(bool(Flags & Smesh_Flags::VERTS_LAZY_DEL))
				DCHECK(!m.raw_verts[v].del) << "vertex is already deleted";
		}
		
		const Smesh& smesh;
		
		friend Smesh;
	};

	
















public:
	class A_Poly {
	public:

		Poly_Props& props;

		void remove() {
			DCHECK( !smesh.raw_polys[poly].del );
			smesh.raw_polys[poly].del = true;
			++smesh.raw_polys_deleted;
		}

		A_Poly_Verts verts;
		A_Poly_Edges edges;


	private:
		A_Poly( Smesh& m, int p )  :
				props(m.raw_polys[p]),
				verts(m, p),
				edges(m, p),
				smesh(m),
				poly(p) {

			if constexpr(bool(Flags & Smesh_Flags::POLYS_LAZY_DEL))
				DCHECK( !m.raw_polys[p].del ) << "polygon is deleted";
		}
		Smesh& smesh;
		int poly;

		friend Smesh;
	};




	class CA_Poly {
	public:

		const Poly_Props& props;

		CA_Poly_Verts verts;
		CA_Poly_Edges edges;


	private:
		CA_Poly( Smesh& m, int p )  :
				props(m.raw_polys[p]),
				verts(m, p),
				edges(m, p),
				smesh(m),
				poly(p) {

			if constexpr(bool(Flags & Smesh_Flags::POLYS_LAZY_DEL))
				DCHECK( !m.raw_polys[p].del ) << "polygon is deleted";
		}
		const Smesh& smesh;
		int poly;

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
			return A_Poly_Vert( smesh, poly, i );
		}

		I_Poly_Vert begin() {
			return I_Poly_Vert( {smesh, poly}, smesh.raw_polys[poly].verts, 0 );
		}

		I_Poly_Vert end() {
			return I_Poly_Vert( {smesh, poly}, smesh.raw_polys[poly].verts, POLY_SIZE );
		}

	private:
		inline void check_idx(int i) {
			DCHECK_GE(i, 0) << "A_Poly_Verts::operator[]: index out of range";
			DCHECK_LT(i, POLY_SIZE) << "A_Poly_Verts::operator[]: index out of range";
		}

	private:
		A_Poly_Verts( Smesh& s, int p ) : smesh(s), poly(p) {}
		Smesh& smesh;
		const int poly;

		friend Smesh;
	};



	class CA_Poly_Verts {
	public:
		constexpr int size() const {
			return POLY_SIZE;
		}

		CA_Poly_Vert operator[]( int i ) {
			check_idx(i);
			return CA_Poly_Vert( smesh, poly, i );
		}

		CI_Poly_Vert begin() {
			return CI_Poly_Vert( {smesh, poly}, smesh.raw_polys[poly].verts, 0 );
		}

		CI_Poly_Vert end() {
			return CI_Poly_Vert( {smesh, poly}, smesh.raw_polys[poly].verts, POLY_SIZE );
		}

	private:
		inline void check_idx(int i) {
			DCHECK_GE(i, 0) << "A_Poly_Verts::operator[]: index out of range";
			DCHECK_LT(i, POLY_SIZE) << "A_Poly_Verts::operator[]: index out of range";
		}

	private:
		CA_Poly_Verts( Smesh& s, int p ) : smesh(s), poly(p) {}
		const Smesh& smesh;
		const int poly;

		friend Smesh;
	};











public:
	class A_Poly_Edges {
	public:
		constexpr int size() const {
			return POLY_SIZE;
		}

		A_Poly_Edge operator[]( int i ) {
			check_idx(i);
			return A_Poly_Edge( smesh, poly, i );
		}

		I_Poly_Edge begin() {
			return I_Poly_Edge( {smesh, poly}, smesh.raw_polys[poly].verts, 0 );
		}

		I_Poly_Edge end() {
			return I_Poly_Edge( {smesh, poly}, smesh.raw_polys[poly].verts, POLY_SIZE );
		}

	private:
		inline void check_idx(int i) {
			DCHECK_GE(i, 0) << "A_Poly_Verts::operator[]: index out of range";
			DCHECK_LT(i, POLY_SIZE) << "A_Poly_Verts::operator[]: index out of range";
		}

	private:
		A_Poly_Edges( Smesh& s, int p ) : smesh(s), poly(p) {}
		Smesh& smesh;
		const int poly;

		friend Smesh;
	};


	class CA_Poly_Edges {
	public:
		constexpr int size() const {
			return POLY_SIZE;
		}

		CA_Poly_Edge operator[]( int i ) {
			check_idx(i);
			return CA_Poly_Edge( smesh, poly, i );
		}

		CI_Poly_Edge begin() {
			return CI_Poly_Edge( {smesh, poly}, smesh.raw_polys[poly].verts, 0 );
		}

		CI_Poly_Edge end() {
			return CI_Poly_Edge( {smesh, poly}, smesh.raw_polys[poly].verts, POLY_SIZE );
		}

	private:
		inline void check_idx(int i) {
			DCHECK_GE(i, 0) << "A_Poly_Verts::operator[]: index out of range";
			DCHECK_LT(i, POLY_SIZE) << "A_Poly_Verts::operator[]: index out of range";
		}

	private:
		CA_Poly_Edges( const Smesh& s, int p ) : smesh(s), poly(p) {}
		const Smesh& smesh;
		const int poly;

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
			int new_poly_vert = (poly_vert + n - 1) % n;
			return A_Poly_Vert(smesh, poly, new_poly_vert);
		}

		A_Poly_Vert next() const {
			int n = POLY_SIZE;//verts.size();
			int new_poly_vert = (poly_vert + 1) % n;
			return A_Poly_Vert(smesh, poly, new_poly_vert);
		}


	private:
		inline void check_idx(int i) {
			DCHECK_GE(i, 0) << "A_Poly_Verts::operator[]: index out of range";
			DCHECK_LT(i, POLY_SIZE) << "A_Poly_Verts::operator[]: index out of range";
		}

		A_Poly_Vert( Smesh& m, int p, int pv ) :
				pos( m.raw_verts[ m.raw_polys[p].verts[pv].idx ].pos ),
				vert( m, m.raw_polys[p].verts[pv].idx ),
				props( m.raw_polys[p].verts[pv] ),
				smesh(m),
				poly(p),
				poly_vert( pv ) {

			check_idx(pv);
		}

		Smesh& smesh;

		const int poly;
		const int poly_vert;

		friend Smesh;
	};





	class CA_Poly_Vert {
	public:
		const Pos& pos;

		CA_Vert vert;

		const Poly_Vert_Props& props;

		CA_Poly_Vert prev() const {
			int n = POLY_SIZE;//verts.size();
			int new_poly_vert = (poly_vert + n - 1) % n;
			return CA_Poly_Vert(smesh, poly, new_poly_vert);
		}

		CA_Poly_Vert next() const {
			int n = POLY_SIZE;//verts.size();
			int new_poly_vert = (poly_vert + 1) % n;
			return CA_Poly_Vert(smesh, poly, new_poly_vert);
		}


	private:
		inline void check_idx(int i) {
			DCHECK_GE(i, 0) << "A_Poly_Verts::operator[]: index out of range";
			DCHECK_LT(i, POLY_SIZE) << "A_Poly_Verts::operator[]: index out of range";
		}

		CA_Poly_Vert( const Smesh& m, int p, int pv ) :
				pos( m.raw_verts[ m.raw_polys[p].verts[pv].idx ].pos ),
				vert( m, m.raw_polys[p].verts[pv].idx ),
				props( m.raw_polys[p].verts[pv] ),
				smesh(m),
				poly(p),
				poly_vert( pv ) {

			check_idx(pv);
		}

		const Smesh& smesh;

		const int poly;
		const int poly_vert;

		friend Smesh;
	};




















	
public:
	class A_Poly_Edge { // or half-edge
	public:
		A_Poly_Edge link() {
			const auto& l = raw().elink;
			DCHECK_GE(l.poly, 0); DCHECK_LT(l.poly, smesh.raw_polys.size());
			DCHECK_GE(l.vert, 0); DCHECK_LT(l.vert, 3);
			return A_Poly_Edge( smesh, l.poly, l.vert );
		}

		void link(A_Poly_Edge& other_poly_edge) {
			DCHECK(!has_link()) << "edge is already linked";
			DCHECK(!other_poly_edge.has_link()) << "other edge is already linked";

			// check if common vertices are shared
			DCHECK_EQ(verts[0].idx, other_poly_edge.verts[1].idx);
			DCHECK_EQ(verts[1].idx, other_poly_edge.verts[0].idx);

			// 2-way
			raw().elink = edge_to_vert(other_poly_edge.handle);
			other_poly_edge.raw().elink = edge_to_vert(handle);
		}

		inline bool has_link() {
			return raw().elink.poly != -1;
		}

		A_Poly poly;

		const H_Poly_Edge handle;

		std::array<A_Vert, 2> verts;

	private:
		auto& raw() {
			return smesh.raw_polys[handle.poly].verts[handle.edge];
		}

		static inline H_Poly_Vert edge_to_vert(const H_Poly_Edge& x) {
			return {x.poly, x.edge};
		}
 
	private:
		A_Poly_Edge( Smesh& m, int p, int8_t pv ) :
			poly( m, p ),
			handle{p,pv},
			verts({A_Vert{m, m.raw_polys[p].verts[pv].idx},
			       A_Vert{m, m.raw_polys[p].verts[(pv+1)%POLY_SIZE].idx}}),
			smesh( m ) {}

		Smesh& smesh;

		friend Smesh;
	};



	class CA_Poly_Edge { // or half-edge
	public:
		CA_Poly_Edge link() {
			const auto& l = raw().elink;
			DCHECK_GE(l.poly, 0); DCHECK_LT(l.poly, smesh.raw_polys.size());
			DCHECK_GE(l.vert, 0); DCHECK_LT(l.vert, 3);
			return CA_Poly_Edge( smesh, l.poly, l.vert );
		}

		void link(CA_Poly_Edge& other_poly_edge) {
			DCHECK(!has_link()) << "edge is already linked";
			DCHECK(!other_poly_edge.has_link()) << "other edge is already linked";

			// check if common vertices are shared
			DCHECK_EQ(verts[0].idx, other_poly_edge.verts[1].idx);
			DCHECK_EQ(verts[1].idx, other_poly_edge.verts[0].idx);

			// 2-way
			raw().elink = edge_to_vert(other_poly_edge.handle);
			other_poly_edge.raw().elink = edge_to_vert(handle);
		}

		inline bool has_link() {
			return raw().elink.poly != -1;
		}

		CA_Poly poly;

		const H_Poly_Edge handle;

		std::array<CA_Vert, 2> verts;

	private:
		auto& raw() {
			return smesh.raw_polys[handle.poly].verts[handle.edge];
		}

		static inline H_Poly_Vert edge_to_vert(const H_Poly_Edge& x) {
			return {x.poly, x.edge};
		}
 
	private:
		CA_Poly_Edge( const Smesh& m, int p, int8_t pv ) :
			poly( m, p ),
			handle{p,pv},
			verts({CA_Vert{m, m.raw_polys[p].verts[pv].idx},
			       CA_Vert{m, m.raw_polys[p].verts[(pv+1)%POLY_SIZE].idx}}),
			smesh( m ) {}

		const Smesh& smesh;

		friend Smesh;
	};







	
	
}; // class Smesh
	






} // namespace smesh


