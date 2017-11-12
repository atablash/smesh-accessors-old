#include <smesh/smesh.hpp>
#include <smesh/compute-elinks.hpp>

#include <gtest/gtest.h>

//#include <smesh/io.hpp>

#include "common.hpp"

using namespace smesh;




using Mesh = Smesh<double, Smesh_Flags::NONE, Smesh_Options>;





TEST(Test_elinks, fast_compute_elinks_cube) {

	auto mesh = get_cube_mesh<Mesh>();

	auto r = fast_compute_elinks(mesh);

	EXPECT_EQ(18, r.num_matched_edges);
	EXPECT_EQ(0, r.num_open_edges);
}









struct init_sdfgsdfg {
	init_sdfgsdfg(){
		google::InstallFailureSignalHandler();
	}
} init_sdfgsdfg;

