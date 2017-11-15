#pragma once




//
// merge 'b' into 'a'
//
template<class VERT>
void merge_verts(VERT a, VERT b, const typename VERT::MESH::PosFloat& alpha) {
	a.pos = a.pos * (1-alpha)  +  b.pos * alpha;

	if constexpr(VERT::Mesh::Has_V_Props) {
		a.props = a.props * (1-alpha)  +  b.props * alpha;
	}

	// update polygons containing 'b': replace 'b'->'a'
	if constexpr(VERT::Mesh::Has_Poly_Vert_Links) {
		for(auto pv : b.plinks) {
			pv.idx = a.idx;
		}
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
void collapse_edge(EDGE ab, const typename EDGE::Mesh::PosFloat& alpha) {
	auto ba = ab.link();

	auto cb = ab.next().link();
	auto ac = ab.prev().link();

	auto da = ba.next().link();
	auto bd = ba.prev().link();


	// optim TODO: don't break links here
	ab.unlink();

	cb.unlink();
	ac.unlink();

	da.unlink();
	bd.unlink();


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



template<class MESH, class GET_V_WEIGHT>
void collapse_edges(MESH& mesh, const MESH::PosFloat& max_edge_length,
		const V_WEIGHT& get_v_weight = [](int){return 1.0;}) {

	bool change = true;

	while(change) {
		change = false;

		for(auto p : mesh.polys) {
			for(auto e : p.edges) {
				if(e.segment.length_sqr() >= max_edge_length * max_edge_length) {
					auto weight_sum = get_v_weight(e.verts[0].idx) + get_v_weight(e.verts[1].idx);
					collapse_edge(e, get_v_weight(e.verts[1].idx) / weight_sum);

					// if weights can be modified
					if constexpr(std::is_lvalue_reference_v<decltype(get_v_weight(0))>) {
						get_v_weight(e.verts[0].idx) += get_v_weight(e.verts[1].idx);
					}

					change = true;
				}
			}
		}
	}
}



