#include <smesh/smesh.hpp>

#include <gtest/gtest.h>

using namespace smesh;


TEST(Const, modify_vertex) {
	Smesh<double> mesh;

	auto v = mesh.verts.add();

	const auto& const_v = v;

	(void)const_v; // avoid unused variable warning

	// should not compile:
	//const_v.pos = {1,2,3};
}
