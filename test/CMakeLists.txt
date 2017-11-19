
# required by GTest
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)



set(sources
	gtest-init.cpp
	compute-normals.cpp
	links.cpp
	cap-holes.cpp
	collapse-edges.cpp
)

if (SMESH_WITH_TINYPLY)
	set(sources ${sources} ../third-party/tinyply.cpp)
	add_definitions(-Wno-unused-parameter)
endif (SMESH_WITH_TINYPLY)

# make #include <smesh/...> work
include_directories( # SYSTEM
	../include
)

include_directories( SYSTEM
	../third-party
)


add_executable(	all-tests ${sources} )

target_link_libraries( all-tests
	${CMAKE_THREAD_LIBS_INIT} # required by GoogleTest
	${GTEST_LIBRARIES}
	${GTEST_MAIN_LIBRARIES}
)

# cotire(all-tests)

add_test( all-tests all-tests )


