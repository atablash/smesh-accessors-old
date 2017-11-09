#pragma once

#include <vector>

template< class MESH, class GET_V_NORMAL >
void compute_normals( MESH& mesh,
		const GET_V_NORMAL& get_v_normal = [&mesh](int iv){ return mesh.vs(iv).props.normal; } ) {

	std::vector<int> nums(mesh.verts.size());

	for(auto& v : mesh.verts) {
		get_v_normal(v.idx) = 0;
	}

	for(auto& p : mesh.polys) {
		auto normal = (p.verts[1] - p.verts[0]).cross(p.verts[2] - p.verts[0]);
		normal.normalize();

		for(auto& pv : p.verts) {
			get_v_normal(pv.vert.idx) += normal;
			++nums(pv.vert.idx);
		}
	}

	for(auto& v : mesh.verts) {
		if(nums[v.idx] > 0) {
			get_v_normal(v.idx) /= nums[v.idx];
			get_v_normal(v.idx).normalize();
		}
	}
}
