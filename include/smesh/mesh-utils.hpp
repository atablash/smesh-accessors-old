#pragma once














//
// check if mesh has isolated vertices
// no vlinks information available
// TODO: another version using vlinks. potentially faster
//
template< class MESH >
bool has_isolated_vertices__novlinks( const MESH& mesh ) {

	std::vector<bool> used(mesh.vs.size());

	for( auto& poly : mesh.ps ) {
		for( auto& poly_vert : poly.vs ) {
			used[poly_vert.idx] = true;
		}
	}

	for( auto& b : used ) if(!b) return true;

	return false;
}






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









template< class MESH, class GET_V_NORMAL >
void grow( MESH& mesh, double amount,
		const GET_V_NORMAL& get_v_normal = [&mesh](int iv){ return mesh.vs(iv).props.normal; } ) {
	
	for(auto& v : mesh.verts) {
		v.pos += amount * get_v_normal(v.idx);
	}
}



