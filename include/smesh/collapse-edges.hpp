#pragma once




//
// merge 'b' into 'a'
//
template<class VERT>
void merge_verts(VERT a, VERT b, const typename VERT::Mesh::Scalar& alpha) {
	a.pos = a.pos * (1-alpha)  +  b.pos * alpha;

	if constexpr(VERT::Mesh::Has_Vert_Props) {
		a.props = a.props * (1-alpha)  +  b.props * alpha;
	}

	// update polygons containing 'b': replace 'b'->'a'
	if constexpr(VERT::Mesh::Has_Vert_Poly_Links) {
		for(auto pv : b.poly_links) {
			pv.idx = a.idx;
			a.poly_links.add(pv);
		}

		b.poly_links.clear();
	}

	b.remove();
}




//
//     C
//    / \
//   /   \
//  A-----B
//   \   /
//    \ /
//     D
//
template<class EDGE>
void collapse_edge(EDGE ab, const typename EDGE::Mesh::Scalar& alpha) {
	auto ba = ab.link();

	auto cb = ab.next().link();
	auto ac = ab.prev().link();

	auto da = ba.next().link();
	auto bd = ba.prev().link();


	// optim TODO: don't break links here
	//ab.unlink();

	//cb.unlink();
	//ac.unlink();

	//da.unlink();
	//bd.unlink();


	ab.poly.remove();
	ba.poly.remove();

	// merge vertices
	merge_verts(ab.verts[0], ab.verts[1], alpha);

	cb.link(ac);
	da.link(bd);

	// TODO: implement V_IProp - indexed vertex properties:
	// i.e. vert props that can be shared by multiple vertices
	// this way V_IProps can be blended on edge collapse (like e.g. texcoords)
}





struct Collapse_Edges_Result {
	int num_edges_collapsed = 0;
	int num_passes = 0;
};


template<class MESH, class GET_V_WEIGHT>
auto collapse_edges(MESH& mesh, const typename MESH::Scalar& max_edge_length, const GET_V_WEIGHT& get_v_weight) {
	Collapse_Edges_Result r;

	bool change = true;

	while(change) {
		change = false;
		++r.num_passes;

		for(auto p : mesh.polys) {
			for(auto e : p.edges) {
				if(e.segment.trace().squaredNorm() >= max_edge_length * max_edge_length) {
					auto weight_sum = get_v_weight(e.verts[0].idx) + get_v_weight(e.verts[1].idx);
					collapse_edge(e, get_v_weight(e.verts[1].idx) / weight_sum);
					++r.num_edges_collapsed;

					// if weights can be modified
					if constexpr(std::is_lvalue_reference_v<decltype(get_v_weight(0))>) {
						get_v_weight(e.verts[0].idx) += get_v_weight(e.verts[1].idx);
					}

					change = true;
				}
			}
		}
	}

	return r;
}


template<class MESH>
auto collapse_edges(MESH& mesh, const typename MESH::Scalar& max_edge_length) {
	std::vector<int32_t> weights(mesh.verts.size_including_deleted(), 1);
	return collapse_edges(mesh, max_edge_length, [&weights](auto i) -> auto& { return weights[i]; });
}



