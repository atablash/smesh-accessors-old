#pragma once




/*
     C
    / \
   /   \
  A-----B
   \   /
    \ /
     D
*/
//
// merge 'b' into 'a'
//
// if links are present, this effectively collapses edge (if present)
//
template<class VERT>
void merge_verts(const VERT& a, const VERT& b, const typename VERT::Mesh::Scalar& alpha) {

	// LOG(INFO) << "merge_verts(" << a.idx << ", " << b.idx << ", alpha:" << alpha << ")";

	a.pos = a.pos * (1-alpha)  +  b.pos * alpha;

	if constexpr(VERT::Mesh::Has_Vert_Props) {
		a.props = a.props * (1-alpha)  +  b.props * alpha;
	}

	// update polygons containing 'b': replace 'b'->'a'
	if constexpr(VERT::Mesh::Has_Vert_Poly_Links) {
		for(auto pv : b.poly_links) {
			pv.idx = a.idx;
		}

		for(auto pv : b.poly_links) {

			// we got degenerate triangle
			if(pv.idx == pv.next().idx) {
				if(pv.prev_edge().has_link && pv.prev_edge().prev_edge().has_link) {
					auto e0 = pv.prev_edge().link();
					auto e1 = pv.prev_edge().prev_edge().link();
					e0.unlink();
					e1.unlink();
					if(e0.poly != e1.poly) e0.link(e1);
				}

				pv.poly.remove();
			}
			else if(pv.idx == pv.prev().idx) {
				if(pv.next_edge().has_link && pv.next_edge().next_edge().has_link) {
					auto e0 = pv.next_edge().link();
					auto e1 = pv.next_edge().next_edge().link();
					e0.unlink();
					e1.unlink();
					if(e0.poly != e1.poly) e0.link(e1);
				}

				pv.poly.remove();
			}
			else {
				a.poly_links.add(pv);
			}
		}

		b.poly_links.clear();
	}

	b.remove();
}










/*
     C
    / \
   /   \
  A-----B
   \   /
    \ /
     D
*/

struct Clean_Flat_Surfaces_On_Edges_Result {
	int num_passes = 0;
	int num_polys_removed = 0;
};

template<class MESH>
auto clean_flat_surfaces_on_edges(MESH& mesh) {

	Clean_Flat_Surfaces_On_Edges_Result r;

	bool change = true;

	while(change) {

		++r.num_passes;
		change = false;

		for(auto p : mesh.polys) {
			for(auto ab : p.edges) {
				if(!ab.has_link) continue;

				auto ba = ab.link();

				if(ba.next_vert().next_vert().idx == ab.next_vert().next_vert().idx) {

					if(ab.next().has_link && ba.prev().has_link) {

						auto cb = ab.next().link();
						auto bd = ba.prev().link();

						if(ba.poly != cb.poly) {
							cb.unlink();
							bd.unlink();

							cb.link(bd);
						}
					}

					if(ab.prev().has_link && ba.next().has_link) {

						auto ac = ab.prev().link();
						auto da = ba.next().link();

						if(ab.poly != da.poly) {
							ac.unlink();
							da.unlink();

							ac.link(da);
						}
					}

					ab.poly.remove();
					ba.poly.remove();
					r.num_polys_removed += 2;
					change = true;
					break; // skip the rest edges of this poly (it's removed and invalid now)
				}
			}
		}
	}

	return r;
}






struct Fast_Collapse_Edges_Result {
	int num_edges_collapsed = 0;
	int num_passes = 0;
};

//
// it's good to call clean_flat_surfaces_on_edges after this
//
template<class MESH, class GET_V_WEIGHT>
auto fast_collapse_edges(MESH& mesh, const typename MESH::Scalar& max_edge_length, const GET_V_WEIGHT& get_v_weight) {
	Fast_Collapse_Edges_Result r;

	bool change = true;

	while(change) {
		change = false;
		++r.num_passes;

		for(auto p : mesh.polys) {
			for(auto e : p.edges) {
				if(e.segment.trace().squaredNorm() <= max_edge_length * max_edge_length) {

					auto weight_sum = get_v_weight(e.verts[0].idx) + get_v_weight(e.verts[1].idx);

					merge_verts(e.verts[0], e.verts[1], (typename MESH::Scalar)get_v_weight(e.verts[1].idx) / weight_sum);
					++r.num_edges_collapsed;

					// if weights can be modified
					if constexpr(std::is_lvalue_reference_v<decltype(get_v_weight(0))>) {
						get_v_weight(e.verts[0].idx) += get_v_weight(e.verts[1].idx);
					}

					//if(r.num_edges_collapsed >= Dupa::get()) {
					//	return r;
					//}

					change = true;
					break; // this poly does not exist now, so break!
				}
			}
		}
	}

	return r;
}


template<class MESH>
auto fast_collapse_edges(MESH& mesh, const typename MESH::Scalar& max_edge_length) {
	std::vector<int32_t> weights(mesh.verts.size_including_deleted(), 1);
	return fast_collapse_edges(mesh, max_edge_length, [&weights](auto i) -> auto& { return weights[i]; });
}








