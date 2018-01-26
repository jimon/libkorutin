
cmake_minimum_required(VERSION 3.3)

# -----------------------------------------------------------------------------

option(BACKEND_SWITCH  "enable stack switch (ASM based) backend (very fast)" ON)
option(BACKEND_THREADS "enable pthread based backend (slow)" OFF)
option(EXAMPLES        "build examples" ON)

# -----------------------------------------------------------------------------

set(PRJ_TARGET "libkorutin")
set(ROOT ${CMAKE_CURRENT_SOURCE_DIR})
project(${PRJ_TARGET})

set(headers
	${ROOT}/includes/libkorutin.h
	${ROOT}/src/scheduling.h
	${ROOT}/src/backend.h
)
set(src
	${ROOT}/src/libkorutin.c
	${ROOT}/src/scheduling.c
)

# -----------------------------------------------------------------------------

if(BACKEND_SWITCH)
	list(APPEND headers
		${ROOT}/src/backends/switch/slp_platformselect.h
		${ROOT}/src/backends/switch/switch_arm_gcc.h
		${ROOT}/src/backends/switch/switch_mips64_gcc.h
		${ROOT}/src/backends/switch/switch_ppc64_gcc.h
		${ROOT}/src/backends/switch/switch_s390x_gcc.h
		${ROOT}/src/backends/switch/switch_x64_msvc.h
		${ROOT}/src/backends/switch/switch_x86_64_gcc.h
		${ROOT}/src/backends/switch/switch_x86_gcc.h
		${ROOT}/src/backends/switch/switch_x86_msvc.h
	)
	list(APPEND src
		${ROOT}/src/backends/switch/switch_backend.c
	)
	if(WIN32)
		enable_language(ASM_MASM)
		if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
			list(APPEND src ${ROOT}/src/backends/switch/switch_x64_msvc.asm)
			set(CMAKE_ASM_MASM_COMPILER ml64) # TODO
		else()
			list(APPEND src ${ROOT}/src/backends/switch/switch_x86_msvc.asm)
		endif()
	endif()
endif()

# -----------------------------------------------------------------------------

if(BACKEND_THREADS)
	list(APPEND headers
		${ROOT}/src/backends/threads/tinycthread.h
	)
	list(APPEND src
		${ROOT}/src/backends/threads/tinycthread.c
		${ROOT}/src/backends/threads/threads_backend.c
	)
endif()

# -----------------------------------------------------------------------------

add_library(${PRJ_TARGET} STATIC ${src} ${headers})
target_include_directories(${PRJ_TARGET} PUBLIC
	${ROOT}/includes
)
target_include_directories(${PRJ_TARGET} PRIVATE
	${ROOT}/src
)

# -----------------------------------------------------------------------------

if(BACKEND_SWITCH)
	target_include_directories(${PRJ_TARGET} PRIVATE
		${ROOT}/src/backends/switch
	)
	target_compile_definitions(${PRJ_TARGET} PRIVATE KORO_BACKEND_SWITCH)
endif()

# -----------------------------------------------------------------------------

if(BACKEND_THREADS)
	target_include_directories(${PRJ_TARGET} PRIVATE
		${ROOT}/src/backends/threads
	)
	target_compile_definitions(${PRJ_TARGET} PRIVATE KORO_BACKEND_THREADS)
endif()

# -----------------------------------------------------------------------------

if(EXAMPLES)
	add_executable(${PRJ_TARGET}_test examples/test.c)
	target_link_libraries(${PRJ_TARGET}_test PUBLIC ${PRJ_TARGET})
endif()
