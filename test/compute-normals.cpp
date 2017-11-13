#include <smesh/smesh.hpp>
#include <smesh/compute-normals.hpp>

#include <gtest/gtest.h>

//#include <smesh/io.hpp>

#include "common.hpp"

using namespace smesh;



struct V_Props_normal {
	Eigen::Matrix<double,3,1> normal;
};

struct Smesh_Options_vnormal : Smesh_Options {
	V_Props_normal Vert_Props();
};

using Mesh = Smesh<double, Smesh_Flags::NONE, Smesh_Options_vnormal>;

TEST(Test_fast_compute_normals, fast_compute_normals_cube) {

	auto mesh = get_cube_mesh<Mesh>();

	fast_compute_normals(mesh);

	for(int x=0; x<2; ++x) {
		for(int y=0; y<2; ++y) {
			for(int z=0; z<2; ++z) {
				int idx = x*4 + y*2 + z;

				if(x == 0) EXPECT_LT(mesh.verts[idx].props.normal[0], -0.1);
				else EXPECT_GT(mesh.verts[idx].props.normal[0], 0.1);

				if(y == 0) EXPECT_LT(mesh.verts[idx].props.normal[1], -0.1);
				else EXPECT_GT(mesh.verts[idx].props.normal[1], 0.1);

				if(z == 0) EXPECT_LT(mesh.verts[idx].props.normal[2], -0.1);
				else EXPECT_GT(mesh.verts[idx].props.normal[2], 0.1);
			}
		}
	}
}



TEST(Test_compute_normals, compute_normals_cube) {
	
	auto mesh = get_cube_mesh<Mesh>();

	compute_normals(mesh);

	auto s = 1.0 / sqrt(3);

	for(int x=0; x<2; ++x) {
		for(int y=0; y<2; ++y) {
			for(int z=0; z<2; ++z) {
				int idx = x*4 + y*2 + z;

				if(x == 0) EXPECT_DOUBLE_EQ(-s, mesh.verts[idx].props.normal[0]);
				else EXPECT_DOUBLE_EQ(s, mesh.verts[idx].props.normal[0]);

				if(y == 0) EXPECT_DOUBLE_EQ(-s, mesh.verts[idx].props.normal[1]);
				else EXPECT_DOUBLE_EQ(s, mesh.verts[idx].props.normal[1]);

				if(z == 0) EXPECT_DOUBLE_EQ(-s, mesh.verts[idx].props.normal[2]);
				else EXPECT_DOUBLE_EQ(s, mesh.verts[idx].props.normal[2]);
			}
		}
	}
}






TEST(Test_compute_normals, compute_normals_cube_external) {
	
	const auto mesh = get_cube_mesh<Mesh>();

	std::vector<Eigen::Matrix<double,3,1>> normals(mesh.verts.size_including_deleted());

	compute_normals(mesh, [&normals](int i) -> auto& { return normals[i]; });

	auto s = 1.0 / sqrt(3);

	for(int x=0; x<2; ++x) {
		for(int y=0; y<2; ++y) {
			for(int z=0; z<2; ++z) {
				int idx = x*4 + y*2 + z;

				if(x == 0) EXPECT_DOUBLE_EQ(-s, normals[idx][0]);
				else EXPECT_DOUBLE_EQ(s, normals[idx][0]);

				if(y == 0) EXPECT_DOUBLE_EQ(-s, normals[idx][1]);
				else EXPECT_DOUBLE_EQ(s, normals[idx][1]);

				if(z == 0) EXPECT_DOUBLE_EQ(-s, normals[idx][2]);
				else EXPECT_DOUBLE_EQ(s, normals[idx][2]);
			}
		}
	}
}





