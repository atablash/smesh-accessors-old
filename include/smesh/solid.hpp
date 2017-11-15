#pragma once


#include "edge-links.hpp"
#include "vert-poly-links.hpp"


template<class MESH>
bool is_solid(MESH& mesh) {

	if constexpr(MESH::Has_Edge_Links) {
		if(!has_valid_edge_links(mesh)) return false;
		if(!has_all_edge_links(mesh)) return false;
	}

	if constexpr(MESH::Has_Vert_Poly_Links) {
		if(!has_valid_vert_poly_links(mesh)) return false;
	}

	return true;
}


