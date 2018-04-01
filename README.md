[![Build Status](https://travis-ci.org/atablash/smesh-accessors-old.svg?branch=master)](https://travis-ci.org/atablash/smesh-accessors-old)

# Why?

* Easy to use modern C++ interface
* Algorithms separated from mesh structure using templates
* Can write adapters for other libraries' mesh objects
* Optimized for speed (well, not yet, but library design makes it possible)

# Using the library

## Accessors

Accessors, or views (or proxy objects), are objects that provide a simple interface for accessing underlying raw structures.

This concept is present in the standard C++ library too, for pseudo-reference objects returned by `std::bitset<N>::operator[]` and `std::vector<bool>::operator[]`.

Where possible, data is exposed using member references instead of member functions, to minimize number of `()`s in code.

```cpp
	for(auto v : mesh.verts) {
		cout << "vertex " << v.idx << "position:" << endl;
		cout << v.pos << endl;
	}
```

Note that accessors are constructed on demand and passed **by value**, like in the above `for`-loop.

One exception is the `mesh` object itself - **it is returned by reference**, not accessor, so make sure to account for this when retrieving parent mesh object from e.g. vertices:

```cpp
	auto& mesh = vertex.mesh;
```

### Pitfails

While some member variables of accessor objects are real references, some of them are not, and can be invalidated. E.g.:

```cpp
	for(auto p : mesh.polys) {
		for(auto pe : p.edges) {
			ASSERT( pe.has_link ); // assume this half-edge is linked to other half edge
			pe.unlink();
			ASSERT( !pe.has_link ); // FAILS! `has_link` is not updated
		}
	}
```

Generally, you should avoid storing accessors, but use them as temporary objects instead.

If you have accessor that might have been invalidated, call `update()` or `operator()()` on it, to get a new updated accessor:

```cpp
	ASSERT( !pe().has_link ); // now it works as desired
```

## Construction

The `Smesh` template has one mandatory parameter - `SCALAR`, i.e. vertex position coordinate type:

```cpp
	auto mesh = Smesh<double>();
```

### Builder

To customize the `Smesh` object, use `Smesh_Builder` template:

```cpp
	// Define some custom vertex attributes
	struct My_Vert_Props {
		Eigen::Matrix<double,3,1> normal;
	};

	// Define the mesh type using above vertex properties, and clear default flags
	using My_Mesh = Smesh_Builder<double>::Vert_Props( My_Vert_Props )::Flags( Smesh_Flags::NONE )::Smesh;
	auto mesh = My_Mesh();
```

To make prototyping easier, *Smesh* has some functionality enabled by default. Use `Smesh_Flags::NONE` to disable all this stuff and make your program run faster.

**TODO:** Implement `Indexed_Vert_Props` - vertex properties that are owned by *vertices* and assigned to one or more *polygon-vertices*. The idea is to have usual vertex properties blending during e.g. edge collapsing, while allowing several classes of polygons, with e.g. different textures/materials that don't interfere with each other.

### Flags

A combination of bit flags of type `Smehs_Flags`:

* `Smesh_Flags::NONE = 0` representing no flags
* `VERTS_LAZY_DEL` (default: on) - turns on vertices lazy removal
* `POLYS_LAZY_DEL` (default: on) - turns on polygons lazy removal
* `EDGE_LINKS` (default: on) - turns on *edge links*
* `VERT_POLY_LINKS` (default: on) - turns on *vertex-polygon* links (or *vertex-(polygon-vertex)* to be precise)

Flags are defined using `enum class` with some bitwise and boolean operators defined, e.g. `|`, `&`, `~`, `!`. Conversion to *bool* requires an implicit cast:

```cpp
	Mesh_Flags flags = EDGE_LINKS | VERT_POLY_LINKS;
	if(bool( flags & EDGE_LINKS )) {
		...
```

## Special properties

Some properties have special meaning, e.g.:

* For vertices
	* `normal` - 3D vector representing vertex normal

Although these properties have special meaning, most functions can be configured to use different property names, or even external arrays (see e.g. `test/compute-normals.cpp`, look for *external* keyword).

## Edge links

Each polygon contains its edges (or, *half-edges*), and each *polygon-edge* can be linked to adjacent polygon's edge.

Enabled using `Mesh_Flags::EDGE_LINKS`.

## Vertex->Polygon links

Each vertex can optionally link polygons (or, to be precise, *polygon-vertices*) that reference it.

Enabled using `Mesh_Flags::VERT_POLY_LINKS`.

## Lazy removal

Several entities can be lazy removed, including:

* Vertices
* Polygons

When lazy deletion happens, the objects are not really removed, but just marked as deleted.

Lazy deleted objects still account for time needed to iterate over all objects, so arrays have to be compacted often enough.

When arrays are compacted, indices are changed and both handles and accessors are invalidated, so compaction requests have to be manually made by user.

To enable lazy removal, use `Mesh_Flags::VERTS_LAZY_DEL` and/or `Mesh_Flags::POLYS_LAZY_DEL`.

**TODO: implement compaction**

## Mesh entities

Some terminology of things that meshes consist of:

* `vertex`
* `polygon`
* `polygon-edge` - reference to edge inside polygon (often called *half-edge*)
* `polygon-vertex` - reference to vertex inside polygon
* `vertex-(polygon-vertex)` - reference to `polygon-vertex` inside vertex

# Handles

Accessors are big objects that hopefully will be mostly optimized out during compilation.

If you want to store references to vertices, polygons or other entities, use `handles` instead. They are optimized for storage.

To get a handle:

```cpp
	auto polygon_edge_handle = polygon_edge.handle;
```

To get back an accessor from handle, you need the parent mesh object:

```cpp
	auto polygon_edge = polygon_edge_handle(mesh);
```

# Accessor const-ness

Accessor objects are immutable. If you want to store accessor in an assignable variable, use handles instead.

Accessors usually come in 2 variants: `Accessor_Type<Const_Flag::TRUE>` and `Accessor_Type<Const_Flag::FALSE`. The first one points to a const entity, while the second one can modify the entity that it points to.

One exception is `mesh.verts` and `mesh.polys` accessors. In order to have them as `Smesh` member variables rather than functions, pointed-to object const-ness is decided to be the same as accessor object const-ness.

# Tests

There are some unit tests in `test` directory. Use them as a reference.


# Eigen

Google's `Eigen` library is used for storing vertex positions and other vectors.

