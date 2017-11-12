#pragma once





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



