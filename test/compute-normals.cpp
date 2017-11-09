#include <smesh/smesh.hpp>
#include <smesh/compute-normals.hpp>

using namespace smesh;



template<class MESH>
void add_quad(MESH& mesh, int a, int b, int c, int d) {
	mesh.polys.emplace_back(a,b,c);
	mesh.polys.emplace_back(a,c,d);
}

Smesh get_cube_mesh() {
	Smesh mesh;

	mesh.verts.emplace_back(-1, -1, -1);
	mesh.verts.emplace_back(-1, -1,  1);
	mesh.verts.emplace_back(-1,  1, -1);
	mesh.verts.emplace_back(-1,  1,  1);
	mesh.verts.emplace_back( 1, -1, -1);
	mesh.verts.emplace_back( 1, -1,  1);
	mesh.verts.emplace_back( 1,  1, -1);
	mesh.verts.emplace_back( 1,  1,  1);

	add_quad(mesh, 0,1,3,2);
	add_quad(mesh, 5,4,6,7);
	
	add_quad(mesh, 0,1,5,4);
	add_quad(mesh, 3,2,6,7);

	add_quad(mesh, 1,3,7,5);
	add_quad(mesh, 2,0,4,6);

	return smesh;
}


TEST(Test_compute_normals, compute_normals_cube) {
	Smesh mesh = get_cube_mesh();

	save_ply(mesh, "test.ply");
}
