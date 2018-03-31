
#===============================================================================
# Additional cmake modules
#===============================================================================
set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
message("CMAKE_MODULE_PATH: " ${CMAKE_MODULE_PATH})




# required by GTest
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)







find_package(gflags REQUIRED)
if (GFLAGS_FOUND)
    include_directories(${GFLAGS_INCLUDE_DIRS})
    link_libraries(${GFLAGS_LIBRARIES})
endif()



set(sources
	main.cpp
	compute-normals.cpp
	links.cpp
	cap-holes.cpp
	collapse-edges.cpp
	const.cpp
)

if (SMESH_WITH_TINYPLY)
	set(sources ${sources} ../third-party/tinyply/source/tinyply.cpp)
	#add_definitions(-Wno-unused-parameter)
endif (SMESH_WITH_TINYPLY)

# make #include <smesh/...> work
include_directories( # SYSTEM
	../include
)

include_directories( SYSTEM
	../third-party/tinyply/source
	../third-party/salgo/include
)


add_executable(	all-tests ${sources} )

target_link_libraries( all-tests
	${CMAKE_THREAD_LIBS_INIT} # required by GoogleTest
	${GTEST_LIBRARIES}
	${GTEST_MAIN_LIBRARIES}
)

# cotire(all-tests)

add_test( all-tests all-tests )


