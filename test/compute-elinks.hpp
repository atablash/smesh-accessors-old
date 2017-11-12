#pragma once



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
	unordered_map< pair<int,int>, MESH::H_Poly_Edge > open_edges;

	for(auto p : mesh.polys) {
		for(auto pe : p.edges) {
			auto edge_key = {pe.verts[0].idx, pe.verts[1].idx};

			auto it_b = open_edges.insert({edge_key, pe.handle()});
			auto it = it_b->first;
			auto inserted = it_b->second

			if(!inserted) {
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
