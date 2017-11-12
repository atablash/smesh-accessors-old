#pragma once

#include <vector>





//
// compute_normals - fast version, no weighting
//
template< class MESH, class GET_V_NORMAL >
void fast_compute_normals( MESH& mesh,
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
std::cout << "A" << std::endl;
		for(auto pv : p.verts) {
std::cout << "B " << pv.vert.idx << std::endl;
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
void fast_compute_normals( MESH& mesh ) {
	fast_compute_normals( mesh, [&mesh](int iv) -> auto& { return mesh.verts[iv].props.normal; } );
}
















template<class POLY_VERT>
auto get_angle(const POLY_VERT& pv) {
	auto v0 = pv.prev().pos - pv.pos;
	auto v1 = pv.next().pos - pv.pos;
	return acos(v0.dot(v1) / (v0.norm() * v1.norm()));
}


//
// compute normals - slower version with weighting
//
template< class MESH, class GET_V_NORMAL >
void compute_normals( MESH& mesh,
		const GET_V_NORMAL& get_v_normal ) {

	std::vector<typename MESH::Pos_Float> weights(mesh.verts.size_including_deleted());

	for(auto v : mesh.verts) {
		get_v_normal(v.idx) = {0,0,0};
	}

	for(auto p : mesh.polys) {
		auto v01 = p.verts[1].pos - p.verts[0].pos;
		auto v02 = p.verts[2].pos - p.verts[0].pos;
		auto normal = v01.cross(v02);
		normal.normalize();

		for(auto pv : p.verts) {
			auto angle = get_angle(pv);
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
void compute_normals( MESH& mesh ) {
	compute_normals( mesh, [&mesh](int iv) -> auto& { return mesh.verts[iv].props.normal; } );
}
