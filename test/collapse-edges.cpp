#include <smesh/smesh.hpp>

#include <smesh/edge-links.hpp>
#include <smesh/vert-poly-links.hpp>

#include <smesh/solid.hpp>

#include <smesh/collapse-edges.hpp>

#include <smesh/io.hpp>

#include <gtest/gtest.h>

#include "common.hpp"

using namespace smesh;




using Mesh = Smesh<double>;





TEST(Collapse_edges, bunny_ply) {

	auto mesh = load_ply<Mesh>("bunny-holes.ply");
	EXPECT_FALSE(mesh.verts.empty());

	fast_compute_edge_links(mesh);
	compute_vert_poly_links(mesh);

	cap_holes(mesh);

	EXPECT_TRUE( is_solid(mesh) );

	
}





