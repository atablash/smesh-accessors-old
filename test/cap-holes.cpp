#include <smesh/smesh.hpp>

#include <smesh/edge-links.hpp>
#include <smesh/vert-poly-links.hpp>

#include <smesh/solid.hpp>

#include <smesh/cap-holes.hpp>

#include <smesh/io.hpp>

#include <gtest/gtest.h>

#include "common.hpp"

using namespace smesh;




using Mesh = Smesh<double>;





TEST(Cap_holes, sphere_holes_ply) {

	auto mesh = load_ply<Mesh>("sphere-holes.ply");
	EXPECT_FALSE(mesh.verts.empty());

	fast_compute_edge_links(mesh);
	compute_vert_poly_links(mesh);

	cap_holes(mesh);

	EXPECT_TRUE( has_valid_edge_links(mesh) );
	EXPECT_TRUE( has_all_edge_links(mesh) );

	EXPECT_TRUE( has_valid_vert_poly_links(mesh) );

	EXPECT_TRUE( is_solid(mesh) );
}




TEST(Cap_holes, bunny_holes_ply) {

	auto mesh = load_ply<Mesh>("bunny-holes.ply");
	EXPECT_FALSE(mesh.verts.empty());

	fast_compute_edge_links(mesh);
	compute_vert_poly_links(mesh);

	auto r = cap_holes(mesh);

	EXPECT_EQ(5, r.num_holes_capped);

	EXPECT_TRUE( has_valid_edge_links(mesh) );
	EXPECT_TRUE( has_all_edge_links(mesh) );

	EXPECT_TRUE( has_valid_vert_poly_links(mesh) );

	EXPECT_TRUE( is_solid(mesh) );
}







