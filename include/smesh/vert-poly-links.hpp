#pragma once



#include <unordered_set>






template<class MESH>
bool has_valid_vert_poly_links(MESH& mesh) {
	std::unordered_set<std::pair<int,int>> checked;

	for(auto v : mesh.verts) {
		for(auto pv : v.poly_links) {

			if(v.key != pv.key) return false;

			bool inserted = checked.insert({pv.poly.key, pv.idx_in_poly}).second;
			if(!inserted) {
				// some vertex already linked to this poly_vert
				return false;
			}
		}
	}

	for(auto p : mesh.polys) {
		for(auto pv : p.verts) {
			auto it = checked.find({pv.poly.key, pv.idx_in_poly});
			if(it == checked.end()) {
				// this poly_vert is not pointed by its vert
				return false;
			}
		}
	}

	return true;
}













template<class MESH>
void compute_vert_poly_links(MESH& mesh) {

	for(auto v : mesh.verts) {
		DCHECK(v.poly_links.empty()) << "compute_plinks expects empty plinks";
	}

	for(auto p : mesh.polys) {
		for(auto pv : p.verts) {
			pv.vert.poly_links.add( pv );
		}
	}
}

