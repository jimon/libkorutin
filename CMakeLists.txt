
cmake_minimum_required(VERSION 3.3)

option(EXAMPLES "build examples" ON)

set(PRJ_TARGET "libkorutin")
set(ROOT ${CMAKE_CURRENT_SOURCE_DIR})
project(${PRJ_TARGET})

add_library(${PRJ_TARGET} STATIC
	${ROOT}/includes/libkorutin.h
	${ROOT}/src/libkorutin.c
	${ROOT}/src/switch/slp_platformselect.h
	${ROOT}/src/switch/switch_arm_gcc.h
	${ROOT}/src/switch/switch_mips64_gcc.h
	${ROOT}/src/switch/switch_ppc64_gcc.h
	${ROOT}/src/switch/switch_s390x_gcc.h
	${ROOT}/src/switch/switch_x64_msvc.asm
	${ROOT}/src/switch/switch_x64_msvc.h
	${ROOT}/src/switch/switch_x86_64_gcc.h
	${ROOT}/src/switch/switch_x86_gcc.h
	${ROOT}/src/switch/switch_x86_msvc.asm
	${ROOT}/src/switch/switch_x86_msvc.h
)
target_include_directories(${PRJ_TARGET} PRIVATE
	${ROOT}/src
	${ROOT}/src/switch
)
target_include_directories(${PRJ_TARGET} PUBLIC
	${ROOT}/includes
)

if(EXAMPLES)
	add_executable(${PRJ_TARGET}_test examples/test.c)
	target_link_libraries(${PRJ_TARGET}_test PUBLIC ${PRJ_TARGET})
endif()
