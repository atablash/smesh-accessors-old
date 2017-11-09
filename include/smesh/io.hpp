#pragma once

#ifdef WITH_TINYPLY




#include <tinyply.h>






template<class MESH>
inline void save_ply(const MESH& mesh, std::string filename, bool binary = true) {

	// Tinyply does not perform any file i/o internally
	std::filebuf fb;
	fb.open(filename, std::ios::out | std::ios::binary);
	std::ostream outputStream(&fb);

	tinyply::PlyFile myFile;

	std::vector<float> verts;
	verts.reserve(mesh.verts.size() * 3);
	for(auto& v : mesh.verts) {
		verts.push_back(v.pos[0]);
		verts.push_back(v.pos[1]);
		verts.push_back(v.pos[2]);
	}


	std::vector<int32_t> vertexIndicies;
	vertexIndicies.reserve(mesh.polys.size() * 3);
	for(auto& p : mesh.polys) {
		vertexIndicies.push_back(p.verts[0].vert.idx);
		vertexIndicies.push_back(p.verts[1].vert.idx);
		vertexIndicies.push_back(p.verts[2].vert.idx);
	}

	//std::vector<float> faceTexcoords;
	//for(auto& p : mesh.polys) {
	//	for(int i=0; i<3; ++i) {
	//		for(int j=0; j<2; ++j) {
	//			faceTexcoords.push_back(p.texcoords[i][j]);
	//		}
	//	}
	//}

	//myFile.comments.push_back("TextureFile " + texture_path);

	myFile.add_properties_to_element("vertex", { "x", "y", "z" }, verts);

	myFile.add_properties_to_element("face", { "vertex_indices" }, vertexIndicies, 3, tinyply::PlyProperty::Type::UINT8);
	//myFile.add_properties_to_element("face", { "texcoord" }, faceTexcoords, 6, tinyply::PlyProperty::Type::UINT8);

	myFile.write(outputStream, binary);

	fb.close();
}






#endif
