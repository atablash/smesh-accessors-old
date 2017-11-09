



# make #include <smesh/...> work
include_directories( # SYSTEM
	../include
)


add_executable(	test
	compute-normals.cpp
)

target_link_libraries( test
	#${CMAKE_THREAD_LIBS_INIT} # required by GoogleTest
	${GTEST_LIBRARIES}
)

add_test(test
	test
)
