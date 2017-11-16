[![Build Status](https://travis-ci.org/atablash/smesh.svg?branch=master)](https://travis-ci.org/atablash/smesh)

# Why?

* Easy to use modern C++ 3D mesh interface
* Algorithms separated from mesh structure using templates
* Optimized for speed

# Using the library

# Accessors

Accessors, or views, are objects that provide simple interface for underlying raw structures.

Where possible, data is exposed using member references instead of member functions, to minimize number of `()`s in code.

	for(auto v : mesh.verts) {
		cout << "vertex " << v.idx << "position:" << endl;
		cout << v.pos << endl;
	}

Note that accessors are constructed on demand and passed **by value**, like in the above `for`-loop.

One exception is the `mesh` object itself - **it is returned by reference**, not accessor, so make sure to account for this when retrieving parent mesh object from e.g. vertices:

	auto& mesh = vertex.mesh;

# Eigen

Google's `Eigen` library is used for storing vertex positions and other vectors.

# Edge links

Each polygon contains its edges (or, *half-edges*), and each *polygon-edge* can be linked to adjacent polygon's edge.

Enabled using `Mesh_Flags::EDGE_LINKS`.

# Vertex->Polygon links

Each vertex can optionally link polygons (or, to be precise, *polygon-vertices*) that reference it.

Enabled using `Mesh_Flags::VERT_POLY_LINKS`.

# Lazy removal

Several entities can be lazy removed, including:

* Vertices
* Polygons

When lazy deletion happens, the objects are not really removed, but just marked as deleted.

Lazy deleted objects still account for time needed to iterate over all objects, so arrays have to be compacted often enough.

When arrays are compacted, indices are changed and both handles and accessors are invalidated, so compaction requests have to be manually made by user.

**TODO: implement compaction**

# Mesh entities

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

	auto polygon_edge_handle = polygon_edge.handle;

To get back an accessor from handle, you need the parent mesh object:

	auto polygon_edge = polygon_edge_handle(mesh)

# Accessor const-ness

Accessors are immutable. If you want a mutable reference to some entity, you have to use handles instead.

Because accessor objects are immutable, they are const.

Accessors usually come in 2 variants: `Accessor_Type<Const_Flag::TRUE>` and `Accessor_Type<Const_Flag::FALSE`. The first one points to a const entity, while the second one can modify the entity that it points to.

# Tests

There are some unit tests in `test` directory. Use them as a reference.

