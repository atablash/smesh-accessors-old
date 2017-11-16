#pragma once

#include "mesh-utils.hpp"

#include <vector>





//
// compute_normals - fast version, no weighting
//
template< class MESH, class GET_V_NORMAL >
void fast_compute_vert_normals( MESH& mesh,
		const GET_V_NORMAL& get_v_normal ) {

	std::vector<int> nums(mesh.verts.size_including_deleted());

	for(auto v : mesh.verts) {
		get_v_normal(v.idx) = {0,0,0};
	}

	for(auto p : mesh.polys) {
		auto v01 = p.verts[1].pos - p.verts[0].pos;
		auto v02 = p.verts[2].pos - p.verts[0].pos;
		auto normal = v01.cross(v02);
		normal.normalize();

		for(auto pv : p.verts) {
			get_v_normal(pv.vert.idx) += normal;
			++nums[pv.vert.idx];
		}
	}

	for(auto v : mesh.verts) {
		if(nums[v.idx] > 0) {
			get_v_normal(v.idx) /= nums[v.idx];
			get_v_normal(v.idx).normalize();
		}
	}
}



template<class MESH>
void fast_compute_vert_normals( MESH& mesh ) {
	fast_compute_vert_normals( mesh, [&mesh](int iv) -> auto& { return mesh.verts[iv].props.normal; } );
}


















//
// compute normals - slower version with weighting
//
template< class MESH, class GET_V_NORMAL >
void compute_vert_normals( MESH& mesh,
		const GET_V_NORMAL& get_v_normal ) {

	std::vector<typename MESH::Scalar> weights(mesh.verts.size_including_deleted());

	for(auto v : mesh.verts) {
		get_v_normal(v.idx) = {0,0,0};
	}

	for(auto p : mesh.polys) {
		auto normal = compute_poly_normal(p);

		for(auto pv : p.verts) {
			auto angle = compute_poly_vert_angle(pv);
			get_v_normal(pv.vert.idx) += normal * angle;
			weights[pv.vert.idx] += angle;
		}
	}

	for(auto v : mesh.verts) {
		if(weights[v.idx] > 0) {
			get_v_normal(v.idx) /= weights[v.idx];
			get_v_normal(v.idx).normalize();
		}
	}
}



template<class MESH>
void compute_vert_normals( MESH& mesh ) {
	compute_vert_normals( mesh, [&mesh](int iv) -> auto& { return mesh.verts[iv].props.normal; } );
}
