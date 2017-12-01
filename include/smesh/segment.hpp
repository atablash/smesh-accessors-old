#pragma once


namespace smesh {


template<class Scalar, int Dim>
struct Segment {

	template<class U, class V>
	Segment(U&& v0, V&& v1) : v{{v0,v1}} {}

	std::array<Eigen::Matrix<Scalar, Dim, 1>, 2> v;

	inline auto trace() const {
		return v[1] - v[0];
	}

// TODO: eigen memory alignment
};


} // namespace smesh

