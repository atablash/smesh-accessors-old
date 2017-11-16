#pragma once






template<class V0, class V1>
auto compute_angle(const V0& v0, const V1& v1) {
	return acos(v0.dot(v1) / (v0.norm() * v1.norm()));
}


template<class POLY_VERT>
auto compute_poly_vert_angle(const POLY_VERT& pv) {
	auto v0 = pv.prev().pos - pv.pos;
	auto v1 = pv.next().pos - pv.pos;
	return compute_angle(v0, v1);
}






template<class POLY>
auto compute_poly_normal(POLY p) {
	auto v01 = p.verts[1].pos - p.verts[0].pos;
	auto v02 = p.verts[2].pos - p.verts[0].pos;
	auto normal = v01.cross(v02);
	normal.normalize();
	return normal;
}










//
// no test coverage
//
template< class MESH >
std::vector<bool> compute_isolated_vertices( const MESH& mesh, bool use_vert_poly_links = false ) {

	if(use_vert_poly_links) {
		throw "not implemented";
	}
	else {
		std::vector<bool> isolated(mesh.verts.size_including_removed(), true);

		for( auto p : mesh.polys ) {
			for( auto pv : p.verts ) {
				isolated[pv.idx] = false;
			}
		}

		return isolated;
	}
}


//
// no test coverage
//
template< class MESH >
bool has_isolated_vertices( const MESH& mesh, bool use_vert_poly_links = false ) {
	auto isolated = compute_isolated_vertices(mesh, use_vert_poly_links);

	for(const auto& b : isolated) {
		if(b) return true;
	}

	return false;
}










//
// no test coverage
//
template< class MESH, class GET_V_NORMAL >
void grow( MESH& mesh, double amount, const GET_V_NORMAL& get_v_normal ) {
	
	for(auto& v : mesh.verts) {
		v.pos += amount * get_v_normal(v.idx);
	}
}



template< class MESH >
void grow( MESH& mesh, double amount) {
	grow(mesh, amount, [&mesh](int iv){ return mesh.verts[iv].props.normal; });
}










