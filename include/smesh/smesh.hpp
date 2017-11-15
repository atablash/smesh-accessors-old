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
	VERTS_LAZY_DEL =  0x0001,
	POLYS_LAZY_DEL =  0x0002,
	EDGE_LINKS =      0x0004,
	VERT_POLY_LINKS = 0x0008
};

ENABLE_BITMASK_OPERATORS(Smesh_Flags)


struct Void {};





// optional `del` member
template<bool> struct Add_Member_del { bool del = false; };
template<>     struct Add_Member_del<false> {};



// link to the other half-edge
template<class MESH,bool> struct Add_Member_edge_link { typename MESH::H_Poly_Vert edge_link; };
template<class MESH>      struct Add_Member_edge_link <MESH, false> {};



// link to the other half-edge
// optim TODO: replace with small object optimization vector
template<class MESH,bool> struct Add_Member_poly_links { std::vector<typename MESH::H_Poly_Vert> poly_links; };
template<class MESH>      struct Add_Member_poly_links <MESH, false> {};




enum class Const_Flag {
	FALSE = 0,
	TRUE = 1
};

template<class T, Const_Flag c> using Const = std::conditional_t<c == Const_Flag::TRUE, const T, T>;




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
	class OPTIONS = Smesh_Options,
	Smesh_Flags FLAGS =
		Smesh_Flags::VERTS_LAZY_DEL |
		Smesh_Flags::POLYS_LAZY_DEL |
		Smesh_Flags::EDGE_LINKS |
		Smesh_Flags::VERT_POLY_LINKS
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

	static constexpr bool Has_Edge_Links = bool(Flags & Smesh_Flags::EDGE_LINKS);
	static constexpr bool Has_Vert_Poly_Links = bool(Flags & Smesh_Flags::VERT_POLY_LINKS);


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


	template<Const_Flag> class A_Vert;
	template<Const_Flag C> using I_Vert = Index_Iterator< A_Vert<C>, std::vector<Vert>>;


	template<Const_Flag> class A_Poly;
	template<Const_Flag C> using I_Poly = Index_Iterator< A_Poly<C>, std::vector<Poly>>;



	struct H_Poly_Vert;
	struct H_Poly_Edge;


	template<Const_Flag> class A_Poly_Verts;
	template<Const_Flag> class A_Poly_Vert;


	template<class A, Const_Flag C>
	struct Get_A_Double_Idx {

		Get_A_Double_Idx(const std::pair<Const<Smesh,C>&,int>& p, int i1) :
			smesh(p.first), idx0(p.second), idx1(i1) {}

		operator A() {
			return A(smesh, idx0, idx1);
		}

	private:
		Const<Smesh,C>& smesh;
		int idx0;
		int idx1;
	};

	// extra is Smesh& and int (poly index)
	template<Const_Flag C>
	using I_Poly_Vert = Index_Iterator< A_Poly_Vert<C>, std::array<Poly_Vert, POLY_SIZE>,
		std::pair<Const<Smesh,C>&,int>,
		Get_A_Double_Idx<A_Poly_Vert<C>, C>>;



	template<Const_Flag> class A_Poly_Edges;
	template<Const_Flag> class A_Poly_Edge;


	template<Const_Flag C>
	using I_Poly_Edge = Index_Iterator< A_Poly_Edge<C>, std::array<Poly_Vert, POLY_SIZE>,
		std::pair<Const<Smesh,C>&, int>,
		Get_A_Double_Idx<A_Poly_Edge<C>, C>>;



	// vert -> poly links
	template<Const_Flag> class A_Poly_Links;

	template<Const_Flag C>
	struct A_Poly_Vert_From_Poly_Link_Iter {

		A_Poly_Vert_From_Poly_Link_Iter(const std::pair<Const<Smesh,C>&,int>& extra, const int i) :
			smesh(extra.first),
			idx0(smesh.raw_verts[extra.second].poly_links[i].poly),
			idx1(smesh.raw_verts[extra.second].poly_links[i].vert) {}

		operator A_Poly_Vert<C>() {
			return A_Poly_Vert<C>(smesh, idx0, idx1);
		}

	private:
		Const<Smesh,C>& smesh;
		const int idx0;
		const int idx1;
	};

	template<Const_Flag C>
	using I_Poly_Link = Index_Iterator< A_Poly_Vert<C>, std::vector< H_Poly_Vert >,
		std::pair<Smesh&,int>,
		A_Poly_Vert_From_Poly_Link_Iter<C> >;





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

		bool operator==(const H_Poly_Vert& o) const {
			return poly == o.poly && vert == o.vert;
		}

		bool operator!=(const H_Poly_Vert& o) const {
			return !(*this == o);
		}
	};

	struct H_Poly_Edge {
		int32_t poly = -1;
		int8_t edge = -1; // 0, 1 or 2


		A_Poly_Edge<Const_Flag::FALSE> get(Smesh& smesh) const {
			return A_Poly_Edge<Const_Flag::FALSE> (smesh, poly, edge);
		}

		A_Poly_Edge<Const_Flag::TRUE> get(const Smesh& smesh) const {
			return A_Poly_Edge<Const_Flag::TRUE> (smesh, poly, edge);
		}

		bool operator==(const H_Poly_Edge& o) const {
			return poly == o.poly && edge == o.edge;
		}

		bool operator!=(const H_Poly_Edge& o) const {
			return !(*this == o);
		}
	};






//
// internal structure types
//
private:
	struct Vert : public Vert_Props,
			public Add_Member_del<bool(Flags & Smesh_Flags::VERTS_LAZY_DEL)>,
			public Add_Member_poly_links<Smesh, bool(Flags & Smesh_Flags::VERT_POLY_LINKS)> {
		Vert() {}
		
		template<class A_POS>
		Vert(A_POS && a_pos) : pos( std::forward<A_POS>(a_pos) ) {}
		
		Pos pos = {0,0,0};
	};
	
	struct Poly_Vert : public Poly_Vert_Props,
			public Add_Member_edge_link<Smesh, bool(Flags & Smesh_Flags::EDGE_LINKS)> { // vertex and corresponidng edge
		Idx idx; // vertex index
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


	// does not have const flag - constness is decided by *this constness in this case
	class A_Verts {
	public:
		A_Vert<Const_Flag::FALSE> add() {
			smesh.raw_verts.emplace_back();
			return A_Vert<Const_Flag::FALSE> {smesh, smesh.raw_verts.back()};
		}
		
		template<class POS_>
		A_Vert<Const_Flag::FALSE> add(POS_&& pos) {
			smesh.raw_verts.emplace_back(pos);
			return A_Vert<Const_Flag::FALSE> {smesh, smesh.raw_verts.back()};
		}
		
		A_Vert<Const_Flag::FALSE> add(const Pos_Float& x, const Pos_Float& y, const Pos_Float& z) {
			smesh.raw_verts.emplace_back(Pos{x,y,z});
			return A_Vert<Const_Flag::FALSE> (smesh, smesh.raw_verts.size()-1);
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



		A_Vert<Const_Flag::FALSE> operator[](int idx) {
			check_idx(idx);
			return A_Vert<Const_Flag::FALSE> {smesh, idx};
		}

		A_Vert<Const_Flag::TRUE> operator[](int idx) const {
			check_idx(idx);
			return A_Vert<Const_Flag::TRUE> {smesh, idx};
		}

		

		// iterator
		I_Vert<Const_Flag::FALSE> begin() {
			return I_Vert<Const_Flag::FALSE> (smesh, smesh.raw_verts, 0);
		}

		I_Vert<Const_Flag::FALSE> end() {
			return I_Vert<Const_Flag::FALSE> (smesh, smesh.raw_verts, smesh.raw_verts.size());
		}


		// const iterator
		I_Vert<Const_Flag::TRUE> begin() const {
			return I_Vert<Const_Flag::TRUE> (smesh, smesh.raw_verts, 0);
		}

		I_Vert<Const_Flag::TRUE> end() const {
			return I_Vert<Const_Flag::TRUE> (smesh, smesh.raw_verts, smesh.raw_verts.size());
		}


	// store environment:
	private:
		A_Verts(Smesh& a_smesh) : smesh(a_smesh) {}
		Smesh& smesh;
		
		friend Smesh;
	};
	
	A_Verts verts = A_Verts(*this);
	
	
	
	
	
	// does not have const flag - constness is decided by *this constness in this case
	class A_Polys {
	public:

		A_Poly<Const_Flag::FALSE> add(const Idx i0, const Idx i1, const Idx i2) {
			Poly p;
			p.verts[0].idx = i0;
			p.verts[1].idx = i1;
			p.verts[2].idx = i2;
			smesh.raw_polys.emplace_back(std::move(p));
			return A_Poly<Const_Flag::FALSE>(smesh, smesh.raw_polys.size()-1);
		}
		
		void reserve( const int n ) {
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
		
		A_Poly<Const_Flag::FALSE> operator[]( const int idx ) {
			DCHECK(0 <= idx && idx < (int)smesh.raw_polys.size()) << "polygon index out of range";
			return A_Poly<Const_Flag::FALSE> (smesh, smesh.raw_polys[idx]);
		}

		A_Poly<Const_Flag::TRUE> operator[]( const int idx ) const {
			DCHECK(0 <= idx && idx < (int)smesh.raw_polys.size()) << "polygon index out of range";
			return A_Poly<Const_Flag::TRUE> (smesh, smesh.raw_polys[idx]);
		}


		// iterator
		I_Poly<Const_Flag::FALSE> begin() {
			return I_Poly<Const_Flag::FALSE> (smesh, smesh.raw_polys, 0);
		}

		I_Poly<Const_Flag::FALSE> end() {
			return I_Poly<Const_Flag::FALSE> (smesh, smesh.raw_polys, smesh.raw_polys.size());
		}


		// const iterator
		I_Poly<Const_Flag::TRUE> begin() const {
			return I_Poly<Const_Flag::TRUE> (smesh, smesh.raw_polys, 0);
		}

		I_Poly<Const_Flag::TRUE> end() const {
			return I_Poly<Const_Flag::TRUE> (smesh, smesh.raw_polys, smesh.raw_polys.size());
		}

		
	// store environment:
	private:
		A_Polys( Smesh& m ) : smesh(m) {}
		Smesh& smesh;
		
		friend Smesh;
	};
	
	A_Polys polys = A_Polys(*this);
	






	template<Const_Flag C>
	class A_Poly_Links {
	public:
		void add(const A_Poly_Vert<C>& pv) const {
			smesh.raw_verts[vert].poly_links.push_back(pv.handle);
		}

		int size() const {
			return (int)smesh.raw_verts[vert].poly_links.size();
		}

		bool empty() const {
			return size() == 0;
		}

		A_Poly_Vert<C> operator[](int i) const {
			const auto& poly_link = smesh.raw_verts[vert].poly_links[i];
			return A_Poly_Vert<C>(smesh, poly_link.poly, poly_link.vert);
		}


		// iterator
		I_Poly_Link<C> begin() const {
			return I_Poly_Link<C>({smesh, vert}, smesh.raw_verts[vert].poly_links, 0);
		}

		I_Poly_Link<C> end() const {
			return I_Poly_Link<C>({smesh, vert}, smesh.raw_verts[vert].poly_links, smesh.raw_verts[vert].poly_links.size());
		}

	// store environment:
	private:
		A_Poly_Links( Const<Smesh,C>& m, const int v ) : smesh(m), vert(v) {}
		Const<Smesh,C>& smesh;
		const int vert;
		
		friend Smesh;
	};
	
	



	
	// accessor
	template<Const_Flag C>
	class A_Vert {
	public:

		Const<Pos,C>& pos;
		Const<Vert_Props,C>& props;

		const int idx;

		const A_Poly_Links<C> poly_links;

		void remove() const {
			DCHECK( !smesh.raw_verts[idx].del );
			smesh.raw_verts[idx].del = true;
			++smesh.raw_verts_deleted;
		}

	// store environment:
	private:
		A_Vert( Const<Smesh,C>& m, const int v)  :
				pos( m.raw_verts[v].pos ),
				props( m.raw_verts[v] ),
				idx(v),
				poly_links(m, v),
				smesh(m) {

			if constexpr(bool(Flags & Smesh_Flags::VERTS_LAZY_DEL))
				DCHECK(!m.raw_verts[v].del) << "vertex is already deleted";
		}
		
		Const<Smesh,C>& smesh;
		
		friend Smesh;
	};

	
















public:
	template<Const_Flag C>
	class A_Poly {
	public:

		Const<Poly_Props,C>& props;

		const Idx idx;

		void remove() const {
			DCHECK( !smesh.raw_polys[idx].del );
			smesh.raw_polys[idx].del = true;
			++smesh.raw_polys_deleted;
		}

		const A_Poly_Verts<C> verts;
		const A_Poly_Edges<C> edges;


	private:
		A_Poly( Const<Smesh,C>& m, const int p )  :
				props(m.raw_polys[p]),
				idx(p),
				verts(m, p),
				edges(m, p),
				smesh(m) {

			if constexpr(bool(Flags & Smesh_Flags::POLYS_LAZY_DEL))
				DCHECK( !m.raw_polys[p].del ) << "polygon is deleted";
		}

		Const<Smesh,C>& smesh;

		friend Smesh;
	};


















public:
	template<Const_Flag C>
	class A_Poly_Verts {
	public:
		constexpr int size() const {
			return POLY_SIZE;
		}

		A_Poly_Vert<C> operator[]( decltype(H_Poly_Vert::vert) i ) const {
			check_idx(i);
			return A_Poly_Vert<C>( smesh, poly, i );
		}

		I_Poly_Vert<C> begin() const {
			return I_Poly_Vert<C>( {smesh, poly}, smesh.raw_polys[poly].verts, 0 );
		}

		I_Poly_Vert<C> end() const {
			return I_Poly_Vert<C>( {smesh, poly}, smesh.raw_polys[poly].verts, POLY_SIZE );
		}

	private:
		inline void check_idx(int i) const {
			DCHECK_GE(i, 0) << "A_Poly_Verts::operator[]: index out of range";
			DCHECK_LT(i, POLY_SIZE) << "A_Poly_Verts::operator[]: index out of range";
		}

	private:
		A_Poly_Verts( Const<Smesh,C>& s, const int p ) : smesh(s), poly(p) {}
		Const<Smesh,C>& smesh;
		const int poly;

		friend Smesh;
	};











public:
	template<Const_Flag C>
	class A_Poly_Edges {
	public:
		constexpr int size() const {
			return POLY_SIZE;
		}

		A_Poly_Edge<C> operator[]( int i ) const {
			check_idx(i);
			return A_Poly_Edge<C>( smesh, poly, i );
		}

		I_Poly_Edge<C> begin() const {
			return I_Poly_Edge<C>( {smesh, poly}, smesh.raw_polys[poly].verts, 0 );
		}

		I_Poly_Edge<C> end() const {
			return I_Poly_Edge<C>( {smesh, poly}, smesh.raw_polys[poly].verts, POLY_SIZE );
		}

	private:
		inline void check_idx(int i) const {
			DCHECK_GE(i, 0) << "A_Poly_Verts::operator[]: index out of range";
			DCHECK_LT(i, POLY_SIZE) << "A_Poly_Verts::operator[]: index out of range";
		}

	private:
		A_Poly_Edges( Const<Smesh,C>& s, int p ) : smesh(s), poly(p) {}
		Const<Smesh,C>& smesh;
		const int poly;

		friend Smesh;
	};












public:
	template<Const_Flag C>
	class A_Poly_Vert {
	public:
		Const<Pos,C>& pos;
		const Idx idx;
		const decltype(H_Poly_Vert::vert) idx_in_poly;

		const A_Vert<C> vert;

		const A_Poly<C> poly;

		Const<Poly_Vert_Props,C>& props;

		A_Poly_Vert<C> prev() const {
			const auto n = POLY_SIZE; //verts.size();
			auto new_poly_vert = (handle.vert + n - 1) % n;
			return A_Poly_Vert<C> (smesh, handle.poly, decltype(H_Poly_Vert::vert)(new_poly_vert));
		}

		A_Poly_Vert<C> next() const {
			const auto n = POLY_SIZE; //verts.size();
			auto new_poly_vert = (handle.vert + 1) % n;
			return A_Poly_Vert<C> (smesh, handle.poly, decltype(H_Poly_Vert::vert)(new_poly_vert));
		}

		const H_Poly_Vert handle;


	private:
		inline void check_idx(int i) const {
			DCHECK_GE(i, 0) << "A_Poly_Verts::operator[]: index out of range";
			DCHECK_LT(i, POLY_SIZE) << "A_Poly_Verts::operator[]: index out of range";
		}

		A_Poly_Vert( Const<Smesh,C>& m, int p, decltype(H_Poly_Vert::vert) pv ) :
				pos( m.raw_verts[ m.raw_polys[p].verts[pv].idx ].pos ),
				idx(              m.raw_polys[p].verts[pv].idx ),
				idx_in_poly(pv),
				vert( m,          m.raw_polys[p].verts[pv].idx ),
				poly(m,p),
				props(            m.raw_polys[p].verts[pv] ),
				handle{p,pv},
				smesh(m) {

			check_idx(handle.vert);
		}

		Const<Smesh,C>& smesh;

		friend Smesh;
	};












	
public:
	template<Const_Flag C>
	class A_Poly_Edge { // or half-edge
	public:
		A_Poly_Edge link() const {
			const auto& l = raw().edge_link;
			DCHECK_GE(l.poly, 0); DCHECK_LT(l.poly, smesh.raw_polys.size());
			DCHECK_GE(l.vert, 0); DCHECK_LT(l.vert, 3);
			return A_Poly_Edge( smesh, l.poly, l.vert );
		}

		void link(A_Poly_Edge& other_poly_edge) const {
			DCHECK(!has_link()) << "edge is already linked";
			DCHECK(!other_poly_edge.has_link()) << "other edge is already linked";

			// check if common vertices are shared
			DCHECK_EQ(verts[0].idx, other_poly_edge.verts[1].idx);
			DCHECK_EQ(verts[1].idx, other_poly_edge.verts[0].idx);

			// 2-way
			raw().edge_link = edge_to_vert(other_poly_edge.handle);
			other_poly_edge.raw().edge_link = edge_to_vert(handle);
		}

		inline bool has_link() const {
			return raw().edge_link.poly != -1;
		}

		const A_Poly<C> poly;

		const H_Poly_Edge handle;

		const std::array<const A_Vert<C>, 2> verts;

		// warning: comparing only handles!
		// can't be used to compare objects from different meshes
		bool operator==(const A_Poly_Edge& o) const {
			return handle == o.handle;
		}

		bool operator!=(const A_Poly_Edge& o) const {
			return !(*this == o);
		}

	private:
		auto& raw() const {
			return smesh.raw_polys[handle.poly].verts[handle.edge];
		}

		static inline H_Poly_Vert edge_to_vert(const H_Poly_Edge& x) {
			return {x.poly, x.edge};
		}
 
	private:
		A_Poly_Edge( Const<Smesh,C>& m, const int p, const int8_t pv ) :
			poly( m, p ),
			handle{p,pv},
			verts({A_Vert<C>{m, m.raw_polys[p].verts[pv].idx},
			       A_Vert<C>{m, m.raw_polys[p].verts[(pv+1)%POLY_SIZE].idx}}),
			smesh( m ) {}

		Const<Smesh,C>& smesh;

		friend Smesh;
	};







	
	
}; // class Smesh
	






} // namespace smesh


