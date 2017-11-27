#pragma once

#include <Eigen/Dense>

#include <glog/logging.h>

#include "common.hpp"

#include "segment.hpp"

//#include <atablash/member-detector.hpp>


#include <unordered_set>



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

namespace {
	constexpr auto VERTS_LAZY_DEL  = Smesh_Flags::VERTS_LAZY_DEL;
	constexpr auto POLYS_LAZY_DEL  = Smesh_Flags::POLYS_LAZY_DEL;
	constexpr auto EDGE_LINKS      = Smesh_Flags::EDGE_LINKS;
	constexpr auto VERT_POLY_LINKS = Smesh_Flags::VERT_POLY_LINKS;
};


ENABLE_BITMASK_OPERATORS( Smesh_Flags );






// optional `del` member
template<bool> struct Add_Member_del { bool del = false; };
template<>     struct Add_Member_del<false> {};



// link to the other half-edge
template<bool, class MESH> struct Add_Member_edge_link { typename MESH::H_Poly_Vert edge_link; };
template<      class MESH> struct Add_Member_edge_link <false, MESH> {};



// link to the other half-edge
// optim TODO: replace with small object optimization vector
template<bool, class MESH> struct Add_Member_poly_links { std::unordered_set<typename MESH::H_Poly_Vert> poly_links; };
template<      class MESH> struct Add_Member_poly_links <false, MESH> {};






enum class Const_Flag {
	FALSE = 0,
	TRUE = 1
};

namespace {
	constexpr auto MUTAB = Const_Flag::FALSE;
	constexpr auto CONST = Const_Flag::TRUE;
};


template<class T, Const_Flag c> using Const = std::conditional_t<c == CONST, const T, T>;




struct Smesh_Options {
	auto Vert_Props()      -> void;
	auto Poly_Props()      -> void;
	auto Poly_Vert_Props() -> void;
};








//
// handles
// TODO: implement small handles
//

//
// a handle to either polygon's vertex or edge (half-edge)
//
struct g_H_Poly_Vert{
	int32_t poly = -1;
	int8_t vert = -1; // 0, 1 or 2



	template<class MESH>
	auto operator()(MESH& mesh) const {
		return typename MESH::VA_Poly_Vert (mesh, poly, vert);
	}

	template<class MESH>
	auto operator()(const MESH& mesh) const {
		return typename MESH::CA_Poly_Vert (mesh, poly, vert);
	}


	template<class MESH>
	auto get(MESH& mesh) const {
		return (*this)(mesh);
	}

	template<class MESH>
	auto get(const MESH& mesh) const {
		return (*this)(mesh);
	}



	bool operator==(const g_H_Poly_Vert& o) const {
		return poly == o.poly && vert == o.vert;
	}

	bool operator!=(const g_H_Poly_Vert& o) const {
		return !(*this == o);
	}
};





struct g_H_Poly_Edge {
	int32_t poly = -1;
	int8_t edge = -1; // 0, 1 or 2


	template<class MESH>
	auto operator()(MESH& mesh) const {
		return typename MESH::VA_Poly_Edge (mesh, poly, edge);
	}

	template<class MESH>
	auto operator()(const MESH& mesh) const {
		return typename MESH::CA_Poly_Edge (mesh, poly, edge);
	}


	template<class MESH>
	auto get(MESH& mesh) const {
		return (*this)(mesh);
	}

	template<class MESH>
	auto get(const MESH& mesh) const {
		return (*this)(mesh);
	}


	bool operator==(const g_H_Poly_Edge& o) const {
		return poly == o.poly && edge == o.edge;
	}

	bool operator!=(const g_H_Poly_Edge& o) const {
		return !(*this == o);
	}
};








//
// a triangle mesh structure with edge links (half-edges)
//
// - lazy removal of vertices or polygons (see bool del flag). TODO: remap support when no lazy removal
//
template <
	class SCALAR,
	class OPTIONS = Smesh_Options,
	Smesh_Flags FLAGS =
		VERTS_LAZY_DEL | POLYS_LAZY_DEL |
		EDGE_LINKS | VERT_POLY_LINKS
>
class Smesh {

public:
	using Scalar = SCALAR;
	using Pos = Eigen::Matrix<Scalar,3,1>;
	using Idx = int32_t; // vertex index type

private:
	struct Void {};
	template<class T> using Type_Or_Void = std::conditional_t< std::is_same_v<T,void>, Void, T >;

public:
	using Vert_Props =      Type_Or_Void< decltype( OPTIONS().Vert_Props() ) >;
	using Poly_Props =      Type_Or_Void< decltype( OPTIONS().Poly_Props() ) >;
	using Poly_Vert_Props = Type_Or_Void< decltype( OPTIONS().Poly_Vert_Props() ) >;

	static constexpr Smesh_Flags Flags = FLAGS;

	static constexpr bool Has_Edge_Links = bool(Flags & EDGE_LINKS);
	static constexpr bool Has_Vert_Poly_Links = bool(Flags & VERT_POLY_LINKS);

	static constexpr bool Has_Vert_Props = !std::is_same_v<Vert_Props, Void>;
	static constexpr bool Has_Poly_Props = !std::is_same_v<Poly_Props, Void>;
	static constexpr bool Has_Poly_Vert_Props = !std::is_same_v<Poly_Vert_Props, Void>;


	static const int POLY_SIZE = 3;



private:

	// internal raw structs for storage
	struct Vert;
	struct Poly;
	struct Poly_Vert;



private:

	template<class A, class CONTAINER, Const_Flag, class EXTRA = Smesh&, class GET_A = A>
	class Iterator;

	template<class A, class CONTAINER, class EXTRA = Smesh&, class GET_A = A>
	class Index_Iterator;

public:
	using H_Poly_Vert = g_H_Poly_Vert;
	using H_Poly_Edge = g_H_Poly_Edge;

public:
	class A_Verts;
	class A_Polys;


	template<Const_Flag> class A_Vert;
	template<Const_Flag C> using I_Vert = Index_Iterator< A_Vert<C>, std::vector<Vert>>;


	template<Const_Flag> class A_Poly;
	template<Const_Flag C> using I_Poly = Index_Iterator< A_Poly<C>, std::vector<Poly>>;


	template<Const_Flag> class A_Poly_Verts;
	template<Const_Flag> class A_Poly_Vert;
	using VA_Poly_Vert = A_Poly_Vert<Const_Flag::FALSE>;
	using CA_Poly_Vert = A_Poly_Vert<Const_Flag::TRUE>;


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
	using VA_Poly_Edge = A_Poly_Edge<Const_Flag::FALSE>;
	using CA_Poly_Edge = A_Poly_Edge<Const_Flag::TRUE>;


	template<Const_Flag C>
	using I_Poly_Edge = Index_Iterator< A_Poly_Edge<C>, std::array<Poly_Vert, POLY_SIZE>,
		std::pair<Const<Smesh,C>&, int>,
		Get_A_Double_Idx<A_Poly_Edge<C>, C>>;



	// vert -> poly links
	template<Const_Flag> class A_Poly_Links;

	template<Const_Flag C>
	struct A_Poly_Vert_From_Poly_Link_Iter {

		A_Poly_Vert_From_Poly_Link_Iter(Const<Smesh,C>& extra, const H_Poly_Vert& handle) :
			smesh(extra),
			idx0(handle.poly),
			idx1(handle.vert) {}

		operator A_Poly_Vert<C>() {
			return A_Poly_Vert<C>(smesh, idx0, idx1);
		}

	private:
		Const<Smesh,C>& smesh;
		const int idx0;
		const int idx1;
	};

	template<Const_Flag C>
	using I_Poly_Link = Iterator< A_Poly_Vert<C>, std::unordered_set<H_Poly_Vert>, C,
		Const<Smesh,C>&, A_Poly_Vert_From_Poly_Link_Iter<C> >;







//
// internal structure types
//
private:
	struct Vert : public Vert_Props,
			public Add_Member_del        <bool(Flags & VERTS_LAZY_DEL)>,
			public Add_Member_poly_links <bool(Flags & VERT_POLY_LINKS), Smesh> {
		Vert() {}
		
		template<class A_POS>
		Vert(A_POS && a_pos) : pos( std::forward<A_POS>(a_pos) ) {}
		
		Pos pos = {0,0,0};
	};
	
	struct Poly_Vert : public Poly_Vert_Props,
			public Add_Member_edge_link<bool(Flags & EDGE_LINKS), Smesh> { // vertex and corresponidng edge
		Idx idx; // vertex index
	};
	
	struct Poly : public Poly_Props, public Add_Member_del<bool(Flags & POLYS_LAZY_DEL)> {
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


public:
	Smesh() {}

	Smesh(const Smesh& o) :
		raw_verts        ( o.raw_verts ),
		raw_verts_deleted( o.raw_verts_deleted ),
		raw_polys        ( o.raw_polys ),
		raw_polys_deleted( o.raw_polys_deleted ) {}


	Smesh(Smesh&& o) :
		raw_verts        ( std::move(o.raw_verts) ),
		raw_verts_deleted( std::move(o.raw_verts_deleted) ),
		raw_polys        ( std::move(o.raw_polys) ),
		raw_polys_deleted( std::move(o.raw_polys_deleted) ) {}


	Smesh& operator=(const Smesh& o) {
		raw_verts         = o.raw_verts;
		raw_verts_deleted = o.raw_verts_deleted;
		raw_polys         = o.raw_polys;
		raw_polys_deleted = o.raw_polys_deleted;
	}


	Smesh& operator=(Smesh&& o) {
		raw_verts         = std::move(o.raw_verts);
		raw_verts_deleted = std::move(o.raw_verts_deleted);
		raw_polys         = std::move(o.raw_polys);
		raw_polys_deleted = std::move(o.raw_polys_deleted);
	}


private:
	//
	// iterator
	// wraps T*, returns accessor A
	//
	// does not support 'del' flags. check older commits for that version
	//
	template<class A, class CONTAINER, Const_Flag C, class EXTRA, class GET_A>
	class Iterator {
		using Container_Iterator = std::conditional_t<C == Const_Flag::TRUE,
			typename CONTAINER::const_iterator,
			typename CONTAINER::iterator>;

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


		template<class AA, Const_Flag CC, class EE, class GG>
		bool operator==(const Iterator<AA,CONTAINER,CC,EE,GG>& o) const {
			return p == o.p;
		}

		template<class AA, Const_Flag CC, class EE, class GG>
		bool operator!=(const Iterator<AA,CONTAINER,CC,EE,GG>& o) const {
			return ! (*this == o);
		}


		A operator*() const {
			return GET_A(extra, *p);
		}


	private:
		inline void increment() {
			++p;
		}

		inline void decrement() {
			--p;
		}

	private:
		Iterator(const EXTRA& a_extra, const Container_Iterator& a_p) :
			extra(a_extra), p(a_p) {}

		EXTRA extra;
		Container_Iterator p;

		friend Smesh;
	};
	
	




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
			raw().emplace_back();
			return A_Vert<Const_Flag::FALSE> {*smesh, raw().back()};
		}
		
		template<class POS_>
		A_Vert<Const_Flag::FALSE> add(POS_&& pos) {
			raw().emplace_back(pos);
			return A_Vert<Const_Flag::FALSE> {*smesh, raw().back()};
		}
		
		A_Vert<Const_Flag::FALSE> add(const Scalar& x, const Scalar& y, const Scalar& z) {
			raw().emplace_back(Pos{x,y,z});
			return A_Vert<Const_Flag::FALSE> (*smesh, raw().size()-1);
		}
		
		void reserve(int n) {
			raw().reserve(n);
		}

		int size_including_deleted() const {
			return raw().size();
		}

		int size() const {
			if constexpr(!bool(Flags & VERTS_LAZY_DEL)) return raw().size();

			return raw().size() - smesh->raw_verts_deleted;
		}

		bool empty() const {
			return size() == 0;
		}


		inline void check_idx(int idx) const {
			DCHECK_GE(idx, 0) << "vertex index out of range";
			DCHECK_LT(idx, (int)raw().size()) << "vertex index out of range";
		}



		A_Vert<Const_Flag::FALSE> operator[](int idx) {
			check_idx(idx);
			return A_Vert<Const_Flag::FALSE> {*smesh, idx};
		}

		A_Vert<Const_Flag::TRUE> operator[](int idx) const {
			check_idx(idx);
			return A_Vert<Const_Flag::TRUE> {*smesh, idx};
		}

		

		// iterator
		I_Vert<Const_Flag::FALSE> begin() {
			return I_Vert<Const_Flag::FALSE> (*smesh, raw(), 0);
		}

		I_Vert<Const_Flag::FALSE> end() {
			return I_Vert<Const_Flag::FALSE> (*smesh, raw(), raw().size());
		}


		// const iterator
		I_Vert<Const_Flag::TRUE> begin() const {
			return I_Vert<Const_Flag::TRUE> (*smesh, raw(), 0);
		}

		I_Vert<Const_Flag::TRUE> end() const {
			return I_Vert<Const_Flag::TRUE> (*smesh, raw(), raw().size());
		}

	private:
		auto& raw() const { return smesh->raw_verts; }


	// store environment:
	private:
		// store mesh as pointer to make this object (and smesh) assignable
		A_Verts(Smesh& a_smesh) : smesh(&a_smesh) {}
		Smesh* smesh;
		
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
			raw().emplace_back(std::move(p));
			return A_Poly<Const_Flag::FALSE>(*smesh, raw().size()-1);
		}
		
		void reserve( const int n ) {
			raw().reserve(n);
		}

		int size_including_deleted() const {
			return raw().size();
		}

		int size() const {

			if constexpr(!bool(Flags & POLYS_LAZY_DEL)) {
				return raw().size();
			}

			return raw().size() - smesh->raw_polys_deleted;
		}
		
		A_Poly<Const_Flag::FALSE> operator[]( const int idx ) {
			DCHECK(0 <= idx && idx < (int)raw().size()) << "polygon index out of range";
			return A_Poly<Const_Flag::FALSE> (*smesh, raw()[idx]);
		}

		A_Poly<Const_Flag::TRUE> operator[]( const int idx ) const {
			DCHECK(0 <= idx && idx < (int)raw().size()) << "polygon index out of range";
			return A_Poly<Const_Flag::TRUE> (*smesh, raw()[idx]);
		}


		// iterator
		I_Poly<Const_Flag::FALSE> begin() {
			return I_Poly<Const_Flag::FALSE> (*smesh, raw(), 0);
		}

		I_Poly<Const_Flag::FALSE> end() {
			return I_Poly<Const_Flag::FALSE> (*smesh, raw(), raw().size());
		}


		// const iterator
		I_Poly<Const_Flag::TRUE> begin() const {
			return I_Poly<Const_Flag::TRUE> (*smesh, raw(), 0);
		}

		I_Poly<Const_Flag::TRUE> end() const {
			return I_Poly<Const_Flag::TRUE> (*smesh, raw(), raw().size());
		}

	private:
		auto& raw() const { return smesh->raw_polys; }

		
	// store environment:
	private:
		// store mesh as pointer to make this object (and smesh) assignable
		A_Polys( Smesh& m ) : smesh(&m) {}
		Smesh* smesh;
		
		friend Smesh;
	};
	
	A_Polys polys = A_Polys(*this);
	






	template<Const_Flag C>
	class A_Poly_Links {
	public:
		void add(const A_Poly_Vert<C>& pv) const {
			DCHECK(smesh.raw_verts[vert].poly_links.find(pv.handle) == smesh.raw_verts[vert].poly_links.end())
				<< "handle already in set";

			smesh.raw_verts[vert].poly_links.insert(pv.handle);
		}

		int size() const {
			return (int)smesh.raw_verts[vert].poly_links.size();
		}

		bool empty() const {
			return size() == 0;
		}

		void clear() const {
			smesh.raw_verts[vert].poly_links.clear();
		}

		A_Poly_Vert<C> operator[](int i) const {
			const auto& poly_link = smesh.raw_verts[vert].poly_links[i];
			return A_Poly_Vert<C>(smesh, poly_link.poly, poly_link.vert);
		}


		// iterator
		I_Poly_Link<C> begin() const {
			return I_Poly_Link<C>(smesh, smesh.raw_verts[vert].poly_links.begin());
		}

		I_Poly_Link<C> end() const {
			return I_Poly_Link<C>(smesh, smesh.raw_verts[vert].poly_links.end());
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
		using Mesh = Smesh;

		Const<Pos,C>& pos;
		Const<Vert_Props,C>& props;

		const int idx;

		const A_Poly_Links<C> poly_links;

		void remove() const {
			DCHECK( !raw().del );
			raw().del = true;
			++smesh.raw_verts_deleted;
		}

	private:
		inline auto& raw() const {
			return smesh.raw_verts[idx];
		}

	// store environment:
	private:
		A_Vert( Const<Smesh,C>& m, const int v)  :
				pos( m.raw_verts[v].pos ),
				props( m.raw_verts[v] ),
				idx(v),
				poly_links(m, v),
				smesh(m) {

			if constexpr(bool(Flags & VERTS_LAZY_DEL))
				DCHECK(!m.raw_verts[v].del) << "vertex is already deleted";
		}

		auto operator()() const { return update(); }
		auto update() const { return A_Vert(smesh, idx); }
		
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

			// unlink edges
			if constexpr(bool(Flags & EDGE_LINKS)) {
				for(auto pe : edges) {
					if(pe.has_link) pe.unlink();
				}
			}

			// unlink vertices
			if constexpr(bool(Flags & VERT_POLY_LINKS)) {
				for(auto pv : verts) {
					pv.vert.raw().poly_links.erase(pv.handle);
				}
			}

			smesh.raw_polys[idx].del = true;
			++smesh.raw_polys_deleted;
		}

		template<Const_Flag C2>
		bool operator==(const A_Poly<C2>& other) const {
			DCHECK_EQ(&smesh, &other.smesh) << "can only compare polys from the same mesh";
			return idx == other.idx;
		}

		template<Const_Flag C2>
		bool operator!=(const A_Poly<C2>& other) const {
			return !(*this == other);
		}

		const A_Poly_Verts<C> verts;
		const A_Poly_Edges<C> edges;

	private:
		auto& raw() const {
			return smesh.raw_polys[idx];
		}

	private:
		A_Poly( Const<Smesh,C>& m, const int p ) :
				props(m.raw_polys[p]),
				idx(p),
				verts(m, p),
				edges(m, p),
				smesh(m) {

			//DCHECK_NE(raw().verts[0].idx, raw().verts[1].idx) << "polygon is degenerate";
			//DCHECK_NE(raw().verts[1].idx, raw().verts[2].idx) << "polygon is degenerate";
			//DCHECK_NE(raw().verts[2].idx, raw().verts[0].idx) << "polygon is degenerate";

			if constexpr(bool(Flags & POLYS_LAZY_DEL))
				DCHECK( !m.raw_polys[p].del ) << "polygon is deleted";
		}

		auto operator()() const { return update(); }
		auto update() const { return A_Poly(smesh, idx); }

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
		Const<Idx,C>& idx;
		const decltype(H_Poly_Vert::vert) idx_in_poly;

		const A_Vert<C> vert;

		const A_Poly<C> poly;

		Const<Poly_Vert_Props,C>& props;


		A_Poly_Vert<C> prev() const {
			const auto n = POLY_SIZE; //verts.size();
			auto new_poly_vert = (handle.vert + n - 1) % n;
			return A_Poly_Vert<C> (mesh, handle.poly, decltype(H_Poly_Vert::vert)(new_poly_vert));
		}

		A_Poly_Vert<C> next() const {
			const auto n = POLY_SIZE; //verts.size();
			auto new_poly_vert = (handle.vert + 1) % n;
			return A_Poly_Vert<C> (mesh, handle.poly, decltype(H_Poly_Vert::vert)(new_poly_vert));
		}


		auto prev_vert() const {
			return prev();
		}

		auto next_vert() const {
			return next();
		}


		auto next_edge() const {
			return A_Poly_Edge<C> (mesh, handle.poly, handle.vert);
		}

		auto prev_edge() const {
			const auto n = POLY_SIZE;
			auto new_poly_vert = (handle.vert + n - 1) % n;
			return A_Poly_Edge<C> (mesh, handle.poly, decltype(H_Poly_Edge::edge)(new_poly_vert));
		}


		const H_Poly_Vert handle;


	private:
		auto& raw() const {
			return mesh.raw_polys[handle.poly].verts[handle.vert];
		}

		inline void check_idx(int i) const {
			DCHECK_GE(i, 0) << "A_Poly_Verts::operator[]: index out of range";
			DCHECK_LT(i, POLY_SIZE) << "A_Poly_Verts::operator[]: index out of range";
		}

	private:
		A_Poly_Vert( Const<Smesh,C>& m, int p, decltype(H_Poly_Vert::vert) pv ) :
				pos( m.raw_verts[ m.raw_polys[p].verts[pv].idx ].pos ),
				idx(              m.raw_polys[p].verts[pv].idx ),
				idx_in_poly(pv),
				vert( m,          m.raw_polys[p].verts[pv].idx ),
				poly(m,p),
				props(            m.raw_polys[p].verts[pv] ),
				handle{p,pv},
				mesh(m) {

			check_idx(handle.vert);
		}

		Const<Smesh,C>& mesh;

		friend Smesh;
		friend g_H_Poly_Vert;
	};












	
public:
	template<Const_Flag C>
	class A_Poly_Edge { // or half-edge
	public:

		using Mesh = Smesh;

		A_Poly_Edge link() const {
			DCHECK(update().has_link) << "link is null";
			const auto& l = raw().edge_link;
			DCHECK_GE(l.poly, 0); DCHECK_LT(l.poly, mesh.raw_polys.size());
			DCHECK_GE(l.vert, 0); DCHECK_LT(l.vert, 3);
			return A_Poly_Edge( mesh, l.poly, l.vert );
		}

		void link(const A_Poly_Edge& other_poly_edge) const {
			DCHECK(!update().has_link) << "edge is already linked";
			DCHECK(!other_poly_edge().has_link) << "other edge is already linked";

			DCHECK(poly != other_poly_edge.poly) << "can't link to the same poly";

			// check if common vertices are shared
			DCHECK_EQ(update().verts[0].idx, other_poly_edge().verts[1].idx);
			DCHECK_EQ(update().verts[1].idx, other_poly_edge().verts[0].idx);

			// 2-way
			raw().edge_link = edge_to_vert(other_poly_edge.handle);
			other_poly_edge.raw().edge_link = edge_to_vert(handle);
		}

		void unlink() const {
			DCHECK(update().has_link) << "edge not linked";
			DCHECK(raw().edge_link.get(mesh).next_edge().has_link) << "mesh corrupted";
			DCHECK(raw().edge_link.get(mesh).next_edge().link() == *this) << "mesh corrupted";

			raw().edge_link.get(mesh).raw().edge_link.poly = -1;
			raw().edge_link.poly = -1;
		}

		A_Poly_Edge<C> prev() const {
			const auto n = POLY_SIZE; //verts.size();
			auto new_poly_edge = (handle.edge + n - 1) % n;
			return A_Poly_Edge<C> (mesh, handle.poly, decltype(H_Poly_Edge::edge)(new_poly_edge));
		}

		A_Poly_Edge<C> next() const {
			const auto n = POLY_SIZE; //verts.size();
			auto new_poly_edge = (handle.edge + 1) % n;
			return A_Poly_Edge<C> (mesh, handle.poly, decltype(H_Poly_Edge::edge)(new_poly_edge));
		}

		auto prev_edge() const { return prev(); }
		auto next_edge() const { return next(); }


		auto prev_vert() const {
			return A_Poly_Vert<C>(mesh, handle.poly, handle.edge);
		}

		auto next_vert() const {
			const auto n = POLY_SIZE;
			auto new_poly_edge = (handle.edge + 1) % n;
			return A_Poly_Vert<C>(mesh, handle.poly, new_poly_edge);
		}


		// warning: comparing only handles!
		// can't be used to compare objects from different meshes
		bool operator==(const A_Poly_Edge& o) const {
			return handle == o.handle;
		}

		bool operator!=(const A_Poly_Edge& o) const {
			return !(*this == o);
		}


		Const<Smesh,C>& mesh;

		const A_Poly<C> poly;

		const H_Poly_Edge handle;

		const std::array<const A_Vert<C>, 2> verts;

		const Segment<typename Mesh::Scalar, 3> segment;

		const bool has_link;


	private:
		auto& raw() const {
			return mesh.raw_polys[handle.poly].verts[handle.edge];
		}


		static inline H_Poly_Vert edge_to_vert(const H_Poly_Edge& x) {
			return {x.poly, x.edge};
		}

	private:
		A_Poly_Edge( Const<Smesh,C>& m, const int p, const int8_t pv ) :
			mesh(m),
			poly( m, p ),
			handle{p,pv},
			verts({A_Vert<C>{m, m.raw_polys[p].verts[pv].idx},
			       A_Vert<C>{m, m.raw_polys[p].verts[(pv+1)%POLY_SIZE].idx}}),
			segment(
				m.raw_verts[ m.raw_polys[p].verts[pv].idx ].pos,
				m.raw_verts[ m.raw_polys[p].verts[(pv+1)%POLY_SIZE].idx ].pos),
			has_link(m.raw_polys[p].verts[pv].edge_link.poly != -1) {}

	public:
		auto update() const { return A_Poly_Edge(mesh, handle.poly, handle.edge); }
		auto operator()() const { return update(); }


		friend Smesh;
		friend g_H_Poly_Edge;
	};







	
	
}; // class Smesh
	






inline std::ostream& operator<<(std::ostream& s, const g_H_Poly_Edge& pe) {
	s << "(p:" << (int)pe.poly << "|e:" << (int)pe.edge << ")";
	return s;
}




// ostream
template<class SCALAR, class OPTIONS, Smesh_Flags FLAGS, Const_Flag C>
std::ostream& operator<<(std::ostream& stream, const typename Smesh<SCALAR, OPTIONS, FLAGS>::template A_Poly<C>& a_poly) {
	stream << "poly(idx:" << a_poly.idx << ",indices:" <<
		a_poly.verts[0].idx << "," <<
		a_poly.verts[1].idx << "," <<
		a_poly.verts[2].idx << ")";
	return stream;
}




} // namespace smesh






















// HASH

namespace std {

template<>
struct hash<::smesh::g_H_Poly_Vert> { // why const?! bug in libstdc++?
	size_t operator()(const ::smesh::g_H_Poly_Vert& pv) const {
		return (pv.poly << 2) ^ pv.vert;
	}
};

template<>
struct hash<::smesh::g_H_Poly_Edge> { // why const?! bug in libstdc++?
	size_t operator()(const ::smesh::g_H_Poly_Edge& pe) const {
		return (pe.poly << 2) ^ pe.edge;
	}
};

}



