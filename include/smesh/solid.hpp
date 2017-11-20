#pragma once


#include "edge-links.hpp"
#include "vert-poly-links.hpp"






template<class MESH>
bool has_degenerate_polys(const MESH& mesh) {
	for(auto p : mesh.polys) {
		for(auto pv : p.verts) {
			if(pv.idx == pv.next().idx) return true;
		}
	}
	return false;
}







struct Check_Solid_Result {

	enum class Failure {
		SUCCESS = 0,
		DEGENERATE_POLYS,
		INVALID_EDGE_LINKS,
		INVALID_VERT_POLY_LINKS
	};

	bool is_solid = false;
	Failure failure = Failure::SUCCESS;
};


enum class Check_Solid_Flags {
	NONE = 0,
	ALLOW_HOLES = 0x0001
};

ENABLE_BITMASK_OPERATORS(Check_Solid_Flags)


template<class MESH>
auto check_solid(const MESH& mesh, Check_Solid_Flags flags = Check_Solid_Flags::NONE) {

	Check_Solid_Result r;

	if(has_degenerate_polys(mesh)) {
		r.failure = Check_Solid_Result::Failure::DEGENERATE_POLYS;
		return r;
	}

	if constexpr(MESH::Has_Edge_Links) {
		if(!has_valid_edge_links(mesh)) {
			r.failure = Check_Solid_Result::Failure::INVALID_EDGE_LINKS;
			return r;
		}
		if(!CHECK_FLAG(flags, ALLOW_HOLES) && !has_all_edge_links(mesh)) {
			r.failure = Check_Solid_Result::Failure::INVALID_EDGE_LINKS;
			return r;
		}
	}

	if constexpr(MESH::Has_Vert_Poly_Links) {
		if(!has_valid_vert_poly_links(mesh)) {
			r.failure = Check_Solid_Result::Failure::INVALID_VERT_POLY_LINKS;
			return r;
		}
	}

	r.is_solid = true;
	return r;
}



template <class MESH>
auto is_solid(const MESH& mesh, Check_Solid_Flags flags = Check_Solid_Flags::NONE) {
	return check_solid(mesh, flags).is_solid;
}
