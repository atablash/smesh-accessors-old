#pragma once

#include <vector>


/*
template<class MESH>
class Get_Vert_Prop_normal {
	MESH& mesh;

public:
	Get_Vert_Prop_normal(MESH& a_mesh) : mesh(a_mesh) {}

	decltype(auto) operator()(int iv) {
		return mesh.verts(iv).props.normal;
	}
};
*/




template< class MESH, class GET_V_NORMAL >
void compute_normals( MESH& mesh,
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
void compute_normals( MESH& mesh ) {
	compute_normals( mesh, [&mesh](int iv) { return mesh.verts[iv].props.normal; } );
}

