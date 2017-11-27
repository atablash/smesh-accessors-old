#include <smesh/smesh.hpp>

#include <smesh/edge-links.hpp>
#include <smesh/vert-poly-links.hpp>

#include <smesh/solid.hpp>

#include <gtest/gtest.h>

#include "common.hpp"

using namespace smesh;







TEST(Edge_links, fast_compute_edge_links_cube) {

	auto mesh = get_cube_mesh<
		Smesh_Builder<double>::Flags<EDGE_LINKS>::Smesh
	>();

	auto r = fast_compute_edge_links(mesh);

	EXPECT_EQ(18, r.num_matched_edges);
	EXPECT_EQ(0, r.num_open_edges);

	EXPECT_TRUE( is_solid(mesh) );
}





TEST(Vert_poly_links, compute_vert_poly_links_cube) {

	auto mesh = get_cube_mesh<
		Smesh_Builder<double>::Flags<VERT_POLY_LINKS>::Smesh
	>();

	compute_vert_poly_links(mesh);

	EXPECT_TRUE( is_solid(mesh) );
}




TEST(Links, compute_links_cube) {

	auto mesh = get_cube_mesh<
		Smesh_Builder<double>::Flags< EDGE_LINKS | VERT_POLY_LINKS >::Smesh
	>();

	{
		auto r = fast_compute_edge_links(mesh);

		EXPECT_EQ(18, r.num_matched_edges);
		EXPECT_EQ(0, r.num_open_edges);
	}

	compute_vert_poly_links(mesh);

	EXPECT_TRUE( is_solid(mesh) );
}





