#pragma once

#include <smesh/>

#include <unordered_map>
#include <vector>


//
// weld vertices that have exactly the same coords
// - MESH::POS needs std::hash
//
template< class MESH >
void weld_exact( MESH& mesh ) {

	std::vector<int32_t> vremap( mesh.vs.size() );

	std::unordered_map<MESH::POS, int32_t> by_pos;

	for( auto& vert : mesh.verts ) {
		auto itr = by_pos.insert({vert.POS, vert.idx}).first;
		remap[vert.idx] = itr->second;
	}

	apply_vremap_and_shrink(mesh, vremap);
}
