#pragma once

#include "mesh-utils.hpp"

#include <vector>





//
// compute_normals - fast version, no weighting
//
template< class MESH, class GET_V_NORMAL >
void fast_compute_vert_normals( MESH& mesh,
		const GET_V_NORMAL& get_v_normal ) {

	std::vector<int> nums(mesh.verts.domain_end());

	for(auto v : mesh.verts) {
		get_v_normal(v.key) = {0,0,0};
	}

	for(auto p : mesh.polys) {
		auto v01 = p.verts[1].pos - p.verts[0].pos;
		auto v02 = p.verts[2].pos - p.verts[0].pos;
		auto normal = v01.cross(v02);
		normal.normalize();

		for(auto pv : p.verts) {
			get_v_normal(pv.vert.key) += normal;
			++nums[pv.vert.key];
		}
	}

	for(auto v : mesh.verts) {
		if(nums[v.key] > 0) {
			get_v_normal(v.key) /= nums[v.key];
			get_v_normal(v.key).normalize();
		}
	}
}



template<class MESH>
void fast_compute_vert_normals( MESH& mesh ) {
	fast_compute_vert_normals( mesh, [&mesh](int iv) -> auto& { return mesh.verts[iv].props().normal; } );
}


















//
// compute normals - slower version with weighting
//
template< class MESH, class GET_V_NORMAL >
void compute_vert_normals( MESH& mesh,
		const GET_V_NORMAL& get_v_normal ) {

	std::vector<typename MESH::Scalar> weights(mesh.verts.domain_end());

	for(auto v : mesh.verts) {
		get_v_normal(v.key) = {0,0,0};
	}

	for(auto p : mesh.polys) {
		auto normal = compute_poly_normal(p);

		for(auto pv : p.verts) {
			auto angle = compute_poly_vert_angle(pv);
			get_v_normal(pv.vert.key) += normal * angle;
			weights[pv.vert.key] += angle;
		}
	}

	for(auto v : mesh.verts) {
		if(weights[v.key] > 0) {
			get_v_normal(v.key) /= weights[v.key];
			get_v_normal(v.key).normalize();
		}
	}
}



template<class MESH>
void compute_vert_normals( MESH& mesh ) {
	compute_vert_normals( mesh, [&mesh](int iv) -> auto& { return mesh.verts[iv].props().normal; } );
}
