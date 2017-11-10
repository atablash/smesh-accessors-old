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

	return mesh;
}

struct V_Props {
	Eigen::Matrix<double,3,1> normal;
};

TEST(Test_compute_normals, compute_normals_cube) {
	auto mesh = get_cube_mesh< Smesh<double, Smesh_Options::NONE, V_Props, Void, Void> >();

	//save_ply(mesh, "test.ply");

	compute_normals(mesh);
}
