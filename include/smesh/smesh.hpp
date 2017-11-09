#pragma once

#include <Eigen/Dense>

#include <glog/logging.h>


#include <atablash/member-detector.hpp>





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
enum class SmeshOptions {
	VS_LAZY_DEL = 0x0001,
	PS_LAZY_DEL = 0x0002
};




//
// a triangle mesh structure with edge links (half-edges)
//
// - lazy removal of vertices or polygons (see bool del flag). TODO: remap support when no lazy removal
//
template <class F,
	SmeshOptions OPTS = VS_LAZY_DEL | PS_LAZY_DEL,
	class VertProps = struct {},
	class PolyProps = struct {},
	class PolyVertProps = {}
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
	struct HPoly_vert{
		int32_t polygon = -1;
		char vertex = -1; // 0, 1 or 2
	};






//
// internal structure types
//
private:
	// optional `del` member
	template<bool>  struct Add_member_del { bool del = false; };
	template<false> struct Add_member_del {};

	struct Vert : public VertProps, public Add_member_del<OPTS & VS_LAZY_DEL> {
		Vert() {}
		
		template<class POS_>
		Vert(POS_&& pos_) : pos(std::forward<POS_>(pos_)) {}
		
		POS pos = {0,0,0};
		//std::vector<HPoly_vert> polygons;
	};
	
	struct PolyVert : public PolyVertProps { // vertex and corresponidng edge
		IDX idx; // vertex index
		//int32_t idx_prop; // property-vertex index
		HPoly_vert elink; // link to the other half-edge
	};
	
	struct Poly : public PolyProps, public Member_del<OPTS & PS_LAZY_DEL> {
		PolyVert vs[3];
	};
	
	

//
// internal data
//
private:
	std::vector<Vert> vertices;
	std::vector<Poly> polygons;


//
// copy and move contruction
// (be careful not to copy/move AVertices and A_polys)
//
public:
	Smesh() {}
	
	Smesh(const Smesh& o) : vertices(o.vertices), polygons(o.polygons) {}
	Smesh(Smesh&& o) : vertices(std::move(o.vertices)), polygons(std::move(o.polygons)) {}
	
	Smesh& operator=(const Smesh& o) {
		vertices = o.vertices;
		polygons = o.polygons;
	}
	
	
	
	






	//
	// private typedefs - used for defining iterators to various things
	//
private:
	template<class A, class T> class Iterator;
	template<class A, class T> class CIterator;


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


		bool operator==(const CIterator<A,T>& o) const {
			return p == o.p;
		}

		bool operator!=(const CIterator<A,T>& o) const {
			return ! (*this == o);
		}


		A operator*() const {
			return A(*p); }


	private:
		inline void increment() {
			do ++p; while(p < end && _smesh_detail::is_deleted(*p));
		}

		inline void decrement() {
			do --p; while(p > begin && _smesh_detail::is_deleted(*p)); // I believe decrementing `begin` is undefined anyway
		}

	private:
		// need to store `begin` and `end` because of `del` flags
		Iterator(T* p_, T* begin_, T* end_) : p(p_), begin(begin_), end(end_) {}
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
	class CIterator {
	public:
		CIterator& operator++() {
			++p;
			return *this; }

		CIterator operator++(int) {
			auto old = *this;
			++p;
			return old; }

		CIterator operator--() {
			--p;
			return *this; }

		CIterator operator--(int) {
			auto old = *this;
			--p;
			return old; }


		bool operator==(const CIterator& o) const {
			return p == o.p; }

		bool operator!=(const CIterator& o) const {
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
		CIterator(const Smesh& smesh_,
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
	
	class AVertices;
	class A_polys;


	class  A_vertex;
	using  IVertex =  Iterator<A_vertex, Vertex>;
	using CIVertex = CIterator<A_vertex, Vertex>;


	class  A_poly;
	using  IPoly =  Iterator<A_poly, Poly>;
	using CIPoly = CIterator<A_poly, Poly>;



	class A_poly_verts;

	class  A_poly_vert;
	using  IPoly_vert =  Iterator<A_poly_vert, Poly_vert>;
	using CIPoly_vert = CIterator<A_poly_vert, Poly_vert>;



	class A_polyEdges;

	class A_polyEdge;
	



	
	class AVertices {
	public:
		A_vertex add() {
			smesh.vertices.emplace_back();
			return A_vertex{smesh, smesh.vertices.back()};
		}
		
		template<class POS_>
		A_vertex add(POS_&& pos) {
			smesh.vertices.emplace_back(pos);
			return A_vertex{smesh, smesh.vertices.back()};
		}
		
		template<class X, class Y, class Z>
		A_vertex add(X&& x, Y&& y, Z&& z) {
			smesh.vertices.emplace_back(POS{
				std::forward<X>(x),
				std::forward<Y>(y),
				std::forward<Z>(z)});
		}
		
		void reserve(int n) {
			smesh.vertices.reserve(n);
		}

		int size() const {
			return smesh.vertices.size();
		}

		
		A_vertex operator[](int idx) {
			DCHECK(0 <= idx && idx < (int)smesh.vertices.size()) << "vertex index out of range";
			return A_vertex{smesh, smesh.vertices[idx]};
		}

		const A_vertex operator[](int idx) const {
			DCHECK(0 <= idx && idx < (int)smesh.vertices.size()) << "vertex index out of range";
			return A_vertex{smesh, smesh.vertices[idx]};
		}


		// iterator
		IVertex begin() {
			return IVertex(smesh, &*smesh.vertices.begin(), &*smesh.vertices.begin(), &*smesh.vertices.end());
		}

		IVertex end() {
			return IVertex(smesh, &*smesh.vertices.end(), &*smesh.vertices.begin(), &*smesh.vertices.end());
		}

		// const iterator
		CIVertex begin() const {
			return CIVertex(smesh, &*smesh.vertices.begin(), &*smesh.vertices.begin(), &*smesh.vertices.end());
		}

		CIVertex end() const {
			return CIVertex(smesh, &*smesh.vertices.end(), &*smesh.vertices.begin(), &*smesh.vertices.end());
		}


	// store environment:
	private:
		AVertices(Smesh& smesh_) : smesh(smesh_) {}
		Smesh& smesh;
		
		friend Smesh;
	};
	
	AVertices vs = AVertices(*this);
	
	
	
	
	
	
	class A_polys {
	public:
		A_poly add(IDX i0, IDX i1, IDX i2) {
			Poly p;
			p.vs[0].idx = i0;
			p.vs[1].idx = i1;
			p.vs[2].idx = i2;
			smesh.polygons.emplace_back(std::move(p));
			return A_poly(smesh, smesh.polygons.back());
		}
		
		void reserve( int n ) {
			smesh.polygons.reserve(n);
		}

		int size() const {
			return smesh.polygons.size();
		}

		
		A_poly operator[]( int idx ) {
			DCHECK(0 <= idx && idx < (int)smesh.polygons.size()) << "polygon index out of range";
			return A_poly(smesh, smesh.polygons[idx]);
		}

		const A_poly operator[]( int idx ) const {
			DCHECK( 0 <= idx  &&  idx < (int)smesh.polygons.size() ) << "polygon index out of range";
			return A_poly(smesh, smesh.polygons[idx]);
		}


		// iterator
		IPoly begin() {
			return IPoly(smesh, &*smesh.polygons.begin(), &*smesh.polygons.begin(), &*smesh.polygons.begin());
		}

		IPoly end() {
			return IPoly(smesh, &*smesh.polygons.end(), &*smesh.polygons.begin(), &*smesh.polygons.begin());
		}

		// const iterator
		CIPoly begin() const {
			return CIPoly(smesh, &*smesh.polygons.begin(), &*smesh.polygons.begin(), &*smesh.polygons.begin());
		}

		CIPoly end() const {
			return CIPoly(smesh, &*smesh.polygons.end(), &*smesh.polygons.begin(), &*smesh.polygons.begin());
		}
		
	// store environment:
	private:
		A_polys( Smesh& a_smesh ) : smesh(a_smesh) {}
		Smesh& smesh;
		
		friend Smesh;
	};
	
	A_polys ps = A_polys(*this);
	
	
	



	
	// accessor
	class A_vertex {
	public:

		POS& pos;
		VertProps& props;

		int idx;

		void remove() {
			DCHECK( !vertex.del );
			vertex.del = true;
		}

	// store environment:
	private:
		A_vertex( Smesh& a_smesh,  Vertex& a_vertex)  :
				pos(    a_vertex.pos ),
				props(  a_vertex ),
				idx(    &a_vertex - &a_smesh.vertices[0] ),
				vertex( a_vertex ) {

			DCHECK(!vertex.del) << "vertex is already deleted";
		}
		
		//Smesh& smesh;
		Vertex& vertex;
		
		friend Smesh;
	};


	
















public:
	class A_poly {
	public:

		PolyProps& props;

		void remove() {
			DCHECK( !polygon.del );
			polygon.del = true;
		}

		A_poly_verts verts;


	private:
		A_poly( Smesh& a_smesh,  Poly& a_polygon )  :
				props(a_polygon),
				verts(a_smesh, a_polygon),
				polygon(a_polygon) {
			DCHECK( !polygon.del ) << "polygon is deleted";
		}
		Poly& polygon;

		friend Smesh;
	};










public:
	class A_poly_verts {
	public:
		int size() const {
			return 3;
		}

		A_poly_vert operator[]( int i ) {
			return A_poly_vert( smesh,  polygon.vs[i] );
		}

		const A_poly_vert operator[]( int i ) const {
			return A_poly_vert( smesh,  polygon.vs[i] );
		}

		IPoly_vert begin() {
			return IPoly_vert( &polygon.vs[0],  &polygon.vs[0],  &polygon.vs[3] );
		}

		IPoly_vert end() {
			return IPoly_vert( &polygon.vs[3],  &polygon.vs[0],  &polygon.vs[3] );
		}

	private:
		A_poly_verts( Smesh& a_smesh,  Poly& a_polygon )  :  smesh( a_smesh ),  polygon( a_polygon )  {}
		Smesh&  smesh;
		Poly&  polygon;

		friend Smesh;
	};











public:
	class A_poly_vert {
	public:
		POS& pos;

		A_vert vert;

		Poly_vert_props& props;

	private:
		A_poly_vert( Smesh& a_smesh,  Poly_vert&  a_poly_vert ) :
			pos( a_smesh.vertices[ a_poly_vert.idx ].pos ),
			vert( a_smesh, a_smesh.vertices[ a_poly_vert.idx ] ),
			props( a_poly_vert ),
			smesh( a_smesh ),
			poly_vert( a_poly_vert ) {}

		Smesh& smesh;
		Poly_vert& poly_vert;

		friend Smesh;
	};


















	
public:
	class A_poly_edge { // or half-edge
	public:
		A_poly_edge link() {
			const auto& l = polygon_vertex.elink;
			auto& poly = polygons[l.polygon];
			return A_polyEdge( smesh,  poly,  poly.vs[l.vertex] );
		}

		const A_poly_edge link() const {
			const auto& l = polygon_vertex.elink;
			auto& poly = polygons[l.polygon];
			return A_poly_edge( smesh,  poly,  poly.vs[l.vertex] );
		}


		A_polygon polygon() {
			return A_poly( smesh,  polygon );
		}

		const A_polygon polygon() const {
			return A_poly( smesh,  polygon );
		}
 
	private:
		A_poly_edge( Smesh& a_smesh,  Poly& a_polygon,  Poly_vert& a_poly_vert ) :
			smesh( a_smesh ),
			polygon( a_polygon),
			polygon_vertex( a_poly_vert )  {}

		Smesh& smesh;
		Poly& polygon;
		Poly_vert&  polygon_vertex;

		friend Smesh;
	};
	
	
}; // class Smesh
	



