#pragma once














//
// check if edge links are valid and 2-way
//
template< class MESH >
bool has_valid_edge_links( const MESH& mesh ) {

	for( auto p : mesh.polys ) {
		for( auto pe : p.edges ) {
			if( !pe.has_link() ) continue;

			if( !pe.link().has_link() ) return false;

			if( pe.link().link() != pe ) return false;
		}
	}

	return true;
}


//
// check if all edges have links
// (assumes links are valid)
//
template< class MESH >
bool has_all_edge_links( const MESH& mesh ) {

	for( auto p : mesh.polys ) {
		for( auto pe : p.edges ) {
			if( !pe.has_link() ) return false;
		}
	}

	return true;
}

















//
// hash pairs:
//

namespace ab {
	template<class T>inline T cycle_bits_left(T x,int i){return (x<<i) ^ (x>>(sizeof(T)*8-i));} // cycle bits left
}

namespace std {
	template<class A, class B> struct hash<pair<A,B> > {
		size_t operator()(const pair<A,B>& p) const {
			return hash<A>()(p.first) ^ ab::cycle_bits_left(hash<B>()(p.second),sizeof(size_t)*8/2);
	    }
	};
}



//
// if one edge is shared among more than 2 polys, pairing order is undefined
//
struct Fast_Compute_Elinks_Result {
	int num_matched_edges = 0;
	int num_open_edges = 0;
};

template<class MESH>
auto fast_compute_elinks(MESH& mesh) {

	Fast_Compute_Elinks_Result result;

	using namespace std;
	unordered_multimap< pair<int,int>, typename MESH::H_Poly_Edge > open_edges;

	for(auto p : mesh.polys) {
		for(auto pe : p.edges) {
			auto edge_key = std::pair(pe.verts[0].idx, pe.verts[1].idx);

			auto rev_edge_key = edge_key;
			std::swap(rev_edge_key.first, rev_edge_key.second);

			auto it = open_edges.find(rev_edge_key);

			if(it == open_edges.end()) {
				open_edges.insert({edge_key, pe.handle});
			}
			else {
				auto other_pe = it->second.get(mesh);
				open_edges.erase(it);

				pe.link(other_pe);
				++result.num_matched_edges;
			}
		}
	}

	result.num_open_edges = open_edges.size();

	return result;
}

