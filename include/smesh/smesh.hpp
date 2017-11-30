#pragma once

#include <Eigen/Dense>

#include <glog/logging.h>

#include "common.hpp"
#include "accessors-common.hpp"
#include "storage.hpp"

#include "segment.hpp"

//#include <atablash/member-detector.hpp>


#include <unordered_set>



namespace smesh {













//
// mesh static flags
//
enum class Smesh_Flags {
	NONE = 0,
	VERTS_ERASABLE =  0x0001,
	POLYS_ERASABLE =  0x0002,
	EDGE_LINKS =      0x0004,
	VERT_POLY_LINKS = 0x0008
};

namespace {
	constexpr auto VERTS_ERASABLE  = Smesh_Flags::VERTS_ERASABLE;
	constexpr auto POLYS_ERASABLE  = Smesh_Flags::POLYS_ERASABLE;
	constexpr auto EDGE_LINKS      = Smesh_Flags::EDGE_LINKS;
	constexpr auto VERT_POLY_LINKS = Smesh_Flags::VERT_POLY_LINKS;
};


ENABLE_BITWISE_OPERATORS( Smesh_Flags );









namespace internal {

	// link to the other half-edge
	template<bool, class MESH> struct Add_Member_edge_link {
		typename MESH::H_Poly_Vert edge_link;
	};
	template<      class MESH> struct Add_Member_edge_link <false, MESH> {};



	// links from verts to poly-verts
	// optim TODO: replace with small object optimization vector
	template<bool, class MESH> struct Add_Member_poly_links { std::unordered_set<typename MESH::H_Poly_Vert> poly_links; };
	template<      class MESH> struct Add_Member_poly_links <false, MESH> {};

}













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
		return typename MESH::template A_Poly_Vert<MUTAB> (mesh, poly, vert);
	}

	template<class MESH>
	auto operator()(const MESH& mesh) const {
		return typename MESH::template A_Poly_Vert<CONST> (mesh, poly, vert);
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
		return typename MESH::template A_Poly_Edge<MUTAB> (mesh, poly, edge);
	}

	template<class MESH>
	auto operator()(const MESH& mesh) const {
		return typename MESH::template A_Poly_Edge<CONST> (mesh, poly, edge);
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





namespace {

	struct _Default__Smesh_Props {
		using Vert = void;
		using Poly = void;
		using Poly_Vert = void;
	};

	constexpr Smesh_Flags _default__smesh_flags =
		VERTS_ERASABLE | POLYS_ERASABLE |
		EDGE_LINKS | VERT_POLY_LINKS;
}



//
// a triangle mesh structure with edge links (half-edges)
//
// - lazy removal of vertices or polygons (see bool del flag). TODO: remap support when no lazy removal
//
template <
	class SCALAR,
	auto FLAGS = _default__smesh_flags,
	class SMESH_PROPS = _Default__Smesh_Props
>
class Smesh {

public:
	using Scalar = SCALAR;
	using Pos = Eigen::Matrix<Scalar,3,1>;

private:
	struct Void {};
	template<class T> using Type_Or_Void = std::conditional_t< std::is_same_v<T,void>, Void, T >;

public:
	static constexpr Smesh_Flags Flags = FLAGS;

	using Vert_Props =      Type_Or_Void< typename SMESH_PROPS::Vert >;
	using Poly_Props =      Type_Or_Void< typename SMESH_PROPS::Poly >;
	using Poly_Vert_Props = Type_Or_Void< typename SMESH_PROPS::Poly_Vert >;

	static constexpr bool Has_Edge_Links = bool(Flags & EDGE_LINKS);
	static constexpr bool Has_Vert_Poly_Links = bool(Flags & VERT_POLY_LINKS);

	static constexpr bool Has_Vert_Props = !std::is_same_v<Vert_Props, Void>;
	static constexpr bool Has_Poly_Props = !std::is_same_v<Poly_Props, Void>;
	static constexpr bool Has_Poly_Vert_Props = !std::is_same_v<Poly_Vert_Props, Void>;


	static const int POLY_SIZE = 3;














	//
	// internal raw structs for storage
	//
private:
	struct Vert;
	struct Poly;
	struct Poly_Vert;













public:
	using H_Poly_Vert = g_H_Poly_Vert;
	using H_Poly_Edge = g_H_Poly_Edge;










public:
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
	// VERTS
	//

private:
	template<Const_Flag C, class OWNER, class BASE>
	class A_Vert_Template : public BASE {
	public:
		using BASE::operator=;

		using Mesh = Smesh;

		using Context = Const<Smesh,C>&;

		Const<Pos,C>& pos;
		Const<Vert_Props,C>& props;

		const A_Poly_Links<C> poly_links;

		// add erase that erases neighbor polys too?

		A_Vert_Template( Context m, Const<OWNER,C>& o, const int i) : BASE(o, i),
				pos( o.raw(i).pos ),
				props( o.raw(i) ),
				poly_links(m, i) {}
	};




private:

	using _Verts_Builder = typename Storage_Builder<Vert>
		::template Accessor_Template<A_Vert_Template>;

	using Verts_Storage_Base = typename std::conditional_t<bool(Flags & VERTS_ERASABLE),
		typename _Verts_Builder::template Add_Flags<STORAGE_ERASABLE>,
		typename _Verts_Builder::template Rem_Flags<STORAGE_ERASABLE>
	> :: Storage;

	class Verts_Storage : public Verts_Storage_Base {
		Verts_Storage(Smesh& m) : Verts_Storage_Base(m) {}
		friend Smesh;
	};

public:
	Verts_Storage verts = Verts_Storage(*this);



public:
	template<Const_Flag C>
	using A_Vert = typename Verts_Storage::template Accessor<C>;











	//
	// POLYS
	//

public:
	template<Const_Flag C, class OWNER, class BASE>
	class A_Poly_Template : public BASE {
	public:
		using BASE::operator=;
		//using BASE::operator==;
		//using BASE::operator!=;

		using Context = Const<Smesh,C>&;

		Const<Poly_Props,C>& props;

		void erase() const {
			BASE::erase();

			// unlink edges
			if constexpr(bool(Flags & EDGE_LINKS)) {
				for(auto pe : edges) {
					if(pe.has_link) pe.unlink();
				}
			}

			// unlink vertices
			if constexpr(bool(Flags & VERT_POLY_LINKS)) {
				for(auto pv : verts) {
					pv.vert.raw.poly_links.erase(pv.handle);
				}
			}
		}

		const A_Poly_Verts<C> verts;
		const A_Poly_Edges<C> edges;


		A_Poly_Template( Context m, Const<OWNER,C>& o, const int i ) : BASE ( o, i ),
				props( o.raw(i) ),
				verts( m, i ),
				edges( m, i ) {
			//DCHECK_NE(raw().verts[0].idx, raw().verts[1].idx) << "polygon is degenerate";
			//DCHECK_NE(raw().verts[1].idx, raw().verts[2].idx) << "polygon is degenerate";
			//DCHECK_NE(raw().verts[2].idx, raw().verts[0].idx) << "polygon is degenerate";
		}
	};





private:

	using _Polys_Builder = typename Storage_Builder<Poly>
		::template Accessor_Template<A_Poly_Template>;

	using Polys_Storage_Base = typename std::conditional_t<bool(Flags & POLYS_ERASABLE),
		typename _Polys_Builder::template Add_Flags<STORAGE_ERASABLE>,
		typename _Polys_Builder::template Rem_Flags<STORAGE_ERASABLE>
	> :: Storage;

	class Polys_Storage : public Polys_Storage_Base {
		Polys_Storage(Smesh& m) : Polys_Storage_Base(m) {}
		friend Smesh;
	};

public:
	Polys_Storage polys = Polys_Storage(*this);



public:
	template<Const_Flag C>
	using A_Poly = typename Polys_Storage::template Accessor<C>;
















	//
	// internal raw structs
	//
private:
	struct Vert : public Vert_Props,
			public internal::Add_Member_poly_links <bool(Flags & VERT_POLY_LINKS), Smesh> {

		Vert() {}
		
		template<class... Args>
		Vert(Args&&... args) : pos( std::forward<Args>(args)... ) {}
		
		Pos pos = {0,0,0};
	};
	
	struct Poly_Vert : public Poly_Vert_Props,
			public internal::Add_Member_edge_link<bool(Flags & EDGE_LINKS), Smesh> { // vertex and corresponidng edge
		typename Verts_Storage::Key key; // vertex index
	};
	
	struct Poly : public Poly_Props {

		Poly(
				typename Verts_Storage::Key a,
				typename Verts_Storage::Key b,
				typename Verts_Storage::Key c) {

			verts[0].key = a;
			verts[1].key = b;
			verts[2].key = c;
		}

		std::array<Poly_Vert, POLY_SIZE> verts;
	};
























public:
	template<Const_Flag C>
	class A_Poly_Links {
	public:
		void add(const A_Poly_Vert<C>& pv) const {
			DCHECK(smesh.verts.raw(vert).poly_links.find(pv.handle) == smesh.verts.raw(vert).poly_links.end())
				<< "handle already in set";

			smesh.verts.raw(vert).poly_links.insert(pv.handle);
		}

		int size() const {
			return (int)smesh.verts.raw(vert).poly_links.size();
		}

		bool empty() const {
			return size() == 0;
		}

		void clear() const {
			smesh.verts.raw(vert).poly_links.clear();
		}

		auto operator[](int i) const {
			const auto& poly_link = smesh.verts.raw(vert).poly_links[i];
			return A_Poly_Vert<C>(smesh, poly_link.poly, poly_link.vert);
		}


		// iterator
		auto begin() const {
			return I_Poly_Link<C>(smesh, smesh.verts.raw(vert).poly_links.begin());
		}

		auto end() const {
			return I_Poly_Link<C>(smesh, smesh.verts.raw(vert).poly_links.end());
		}

	// store environment:
	private:
		A_Poly_Links( Const<Smesh,C>& m, const int v ) : smesh(m), vert(v) {}
		Const<Smesh,C>& smesh;
		const int vert;
		
		friend Smesh;
	};
	


























public:
	template<Const_Flag C>
	class A_Poly_Verts {
	public:
		constexpr int size() const {
			return POLY_SIZE;
		}

		auto operator[]( decltype(H_Poly_Vert::vert) i ) const {
			check_idx(i);
			return A_Poly_Vert<C>( smesh, poly, i );
		}

		auto begin() const {
			return I_Poly_Vert<C>( {smesh, poly}, smesh.polys.raw(poly).verts, 0 );
		}

		auto end() const {
			return I_Poly_Vert<C>( {smesh, poly}, smesh.polys.raw(poly).verts, POLY_SIZE );
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

		auto operator[]( int i ) const {
			check_idx(i);
			return A_Poly_Edge<C>( smesh, poly, i );
		}

		auto begin() const {
			return I_Poly_Edge<C>( {smesh, poly}, smesh.polys.raw(poly).verts, 0 );
		}

		auto end() const {
			return I_Poly_Edge<C>( {smesh, poly}, smesh.polys.raw(poly).verts, POLY_SIZE );
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

		Const<typename Verts_Storage::Key,C>& key;

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
			return mesh.polys.raw(handle.poly).verts[handle.vert];
		}

		inline void check_idx(int i) const {
			DCHECK_GE(i, 0) << "A_Poly_Verts::operator[]: index out of range";
			DCHECK_LT(i, POLY_SIZE) << "A_Poly_Verts::operator[]: index out of range";
		}

	private:
		A_Poly_Vert( Const<Smesh,C>& m, int p, decltype(H_Poly_Vert::vert) pv ) :
				pos( m.verts.raw( m.polys.raw(p).verts[pv].key ).pos ),
				key(              m.polys.raw(p).verts[pv].key ),
				idx_in_poly( pv ),
				vert(    m.verts[ m.polys.raw(p).verts[pv].key ] ),
				poly( m.polys[p] ),
				props(            m.polys.raw(p).verts[pv] ),
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
			DCHECK_GE(l.poly, 0); DCHECK_LT(l.poly, mesh.polys.domain_end());
			DCHECK_GE(l.vert, 0); DCHECK_LT(l.vert, 3);
			return A_Poly_Edge( mesh, l.poly, l.vert );
		}

		void link(const A_Poly_Edge& other_poly_edge) const {
			DCHECK(!update().has_link) << "edge is already linked";
			DCHECK(!other_poly_edge().has_link) << "other edge is already linked";

			DCHECK(poly != other_poly_edge.poly) << "can't link to the same poly";

			// check if common vertices are shared
			DCHECK_EQ(update().verts[0].key, other_poly_edge().verts[1].key);
			DCHECK_EQ(update().verts[1].key, other_poly_edge().verts[0].key);

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
			return mesh.polys.raw(handle.poly).verts[handle.edge];
		}


		static inline H_Poly_Vert edge_to_vert(const H_Poly_Edge& x) {
			return {x.poly, x.edge};
		}

	private:
		A_Poly_Edge( Const<Smesh,C>& m, const int p, const int8_t pv ) :
			mesh(m),
			poly( m.polys[p] ),
			handle{p,pv},
			verts({m.verts[ m.polys.raw(p).verts[pv].key ],
			       m.verts[ m.polys.raw(p).verts[(pv+1)%POLY_SIZE].key ]}),
			segment(
				m.verts.raw( m.polys.raw(p).verts[pv].key ).pos,
				m.verts.raw( m.polys.raw(p).verts[(pv+1)%POLY_SIZE].key ).pos),
			has_link(m.polys.raw(p).verts[pv].edge_link.poly != -1) {}

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
template<class SCALAR, Smesh_Flags FLAGS, class OPTIONS, Const_Flag C>
std::ostream& operator<<(std::ostream& stream, const typename Smesh<SCALAR, FLAGS, OPTIONS>::template A_Poly<C>& a_poly) {
	stream << "poly(key:" << a_poly.key << ",indices:" <<
		a_poly.verts[0].key << "," <<
		a_poly.verts[1].key << "," <<
		a_poly.verts[2].key << ")";
	return stream;
}






















// BUILDER
template<class SCALAR,
	auto  FLAGS = _default__smesh_flags,
	class PROPS = _Default__Smesh_Props
>
class Smesh_Builder {
private:
	template<class S, Smesh_Flags F, class P>
	using _Smesh = Smesh<S,F,P>;

	template<class VERT, class POLY, class POLY_VERT>
	struct Props {
		using Vert = VERT;
		using Poly = POLY;
		using Poly_Vert = POLY_VERT;
	};

public:
	using Smesh = _Smesh<SCALAR, FLAGS, PROPS>;

	template<class NEW_VERT_PROPS>
	using Vert_Props      = Smesh_Builder<SCALAR, FLAGS,
		Props<NEW_VERT_PROPS, typename PROPS::Poly, typename PROPS::Poly_Vert>>;
	
	template<class NEW_POLY_PROPS>
	using Poly_Props      = Smesh_Builder<SCALAR, FLAGS,
		Props<typename PROPS::Vert, NEW_POLY_PROPS, typename PROPS::Poly_Vert>>;

	template<class NEW_POLY_VERT_PROPS>
	using Poly_Vert_Props = Smesh_Builder<SCALAR, FLAGS,
		Props<typename PROPS::Vert, typename PROPS::Poly, NEW_POLY_VERT_PROPS>>;

	template<Smesh_Flags NEW_FLAGS>
	using Flags           = Smesh_Builder<SCALAR, NEW_FLAGS, PROPS>;

	template<Smesh_Flags NEW_FLAGS>
	using Add_Flags       = Smesh_Builder<SCALAR, FLAGS |  NEW_FLAGS, PROPS>;

	template<Smesh_Flags NEW_FLAGS>
	using Rem_Flags    = Smesh_Builder<SCALAR, FLAGS & ~NEW_FLAGS, PROPS>;
};















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




















