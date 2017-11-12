#include <smesh/smesh.hpp>
#include <smesh/compute-normals.hpp>

#include <gtest/gtest.h>

//#include <smesh/io.hpp>

using namespace smesh;



template<class MESH>
void add_quad(MESH& mesh, int a, int b, int c, int d) {
	mesh.polys.add(a,b,c);
	mesh.polys.add(a,c,d);
}


template<class MESH>
MESH get_cube_mesh() {
	MESH mesh;

	mesh.verts.add(-1, -1, -1);
	mesh.verts.add(-1, -1,  1);
	mesh.verts.add(-1,  1, -1);
	mesh.verts.add(-1,  1,  1);
	mesh.verts.add( 1, -1, -1);
	mesh.verts.add( 1, -1,  1);
	mesh.verts.add( 1,  1, -1);
	mesh.verts.add( 1,  1,  1);

	add_quad(mesh, 0,1,3,2);
	add_quad(mesh, 5,4,6,7);
	
	add_quad(mesh, 1,0,4,5);
	add_quad(mesh, 2,3,7,6);

	add_quad(mesh, 3,1,5,7);
	add_quad(mesh, 0,2,6,4);

	//save_ply(mesh, "test.ply");

	return mesh;
}


struct V_Props {
	Eigen::Matrix<double,3,1> normal;
};


TEST(Test_fast_compute_normals, fast_compute_normals_cube) {

	struct Opts : Smesh_Options {
		V_Props Vert_Props();
	};

	auto mesh = get_cube_mesh< Smesh<double, Smesh_Flags::NONE, Opts> >();

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

	struct Opts : Smesh_Options {
		V_Props Vert_Props();
	};
	
	auto mesh = get_cube_mesh< Smesh<double, Smesh_Flags::NONE, Opts> >();

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
	
	auto mesh = get_cube_mesh< Smesh<double, Smesh_Flags::NONE> >();

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






struct Init {
	Init(){
		google::InstallFailureSignalHandler();
	}
} init;

