#pragma once


//
// remap references to vertex indices
//
template< class MESH, class VREMAP >
void apply_vremap( MESH& mesh,  const VREMAP& vremap ) {

	for( auto& poly : mesh.ps ) {
		for( auto& poly_vert : poly.vs ) {
			auto& idx = poly_vert.idx;
			if( idx != vremap[idx] )  idx = vremap[idx]; // write only if value different (faster?)
		}
	}
}






//
// * remap references to vertex indices
// * shrink vertices vector
//
template< class MESH, class VREMAP >
void apply_vremap_and_shrink( MESH& mesh,  const VREMAP& vremap ) {

	apply_vremap( mesh,  vremap );

	auto max_entry = std::max(vremap);
	int new_num_vertices = max_entry + 1;

	DCHECK_LE( new_num_vertices,  mesh.vs.size() ) << "vremap entry out of bounds: " << new_num_vertices;

	mesh.vs.resize(new_num_vertices);
}

