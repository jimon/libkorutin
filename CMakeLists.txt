
cmake_minimum_required(VERSION 3.3)

# -----------------------------------------------------------------------------

option(BACKEND_SWITCH  "build switch (asm based) backend (fast)"  ON)
option(BACKEND_CONTEXT "build boost context based backend (fast)" ON)
option(BACKEND_THREADS "build pthread based backend (slow)"       ON)
option(EXAMPLES        "build examples"                           ON)

# -----------------------------------------------------------------------------

set(PRJ_TARGET "libkorutin")
set(ROOT ${CMAKE_CURRENT_SOURCE_DIR})
project(${PRJ_TARGET})

set(headers_core
	${ROOT}/includes/libkorutin.h
	${ROOT}/src/scheduling.h
	${ROOT}/src/debug.h
	${ROOT}/src/backend.h
)
set(src_core
	${ROOT}/src/libkorutin.c
	${ROOT}/src/scheduling.c
	${ROOT}/src/debug.c
)
set(public_includes_core
	${ROOT}/includes
)
set(private_includes_core
	${ROOT}/src
)

if(WIN32)
	enable_language(ASM_MASM)
	if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
		set(CMAKE_ASM_MASM_COMPILER ml64) # TODO
	endif()
else()
	enable_language(ASM-ATT)
endif()

# -----------------------------------------------------------------------------

if(BACKEND_SWITCH)
	set(headers_switch
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
	set(src_switch
		${ROOT}/src/backends/switch/switch_backend.c
	)
	set(private_includes_switch
		${ROOT}/src/backends/switch
	)
	if(WIN32)
		if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
			list(APPEND src_switch ${ROOT}/src/backends/switch/switch_x64_msvc.asm)
		else()
			list(APPEND src_switch ${ROOT}/src/backends/switch/switch_x86_msvc.asm)
		endif()
	endif()

	add_library(${PRJ_TARGET}_switch STATIC
		${headers_core}
		${src_core}
		${headers_switch}
		${src_switch}
	)
	target_include_directories(${PRJ_TARGET}_switch PUBLIC
		${public_includes_core}
	)
	target_include_directories(${PRJ_TARGET}_switch PRIVATE
		${private_includes_core}
		${private_includes_switch}
	)
	target_compile_definitions(${PRJ_TARGET}_switch PUBLIC
		KORO_BACKEND_SWITCH
	)
endif()

# -----------------------------------------------------------------------------

if(BACKEND_CONTEXT)
	# boost context asm folder have quite a nice name convention:
	set(CONTEXT_ARCH "x86_64") # possible values: arm, arm64, i386, mips32, ppc32, ppc64, x86_64
	set(CONTEXT_ABI  "ms")     # possible values: aapcs, ms, sysv, o32, ppc64
	set(CONTEXT_EXE  "pe")     # possible values: elf, macho, pe, xcoff
	set(CONTEXT_ASM  "masm")   # possible values: gas, masm, armasm
	set(CONTEXT_EXT  "asm")    # possible values: asm, S

	set(src_context
		${ROOT}/src/backends/context/context_backend.c
		${ROOT}/src/backends/context/asm/jump_${CONTEXT_ARCH}_${CONTEXT_ABI}_${CONTEXT_EXE}_${CONTEXT_ASM}.${CONTEXT_EXT}
		${ROOT}/src/backends/context/asm/make_${CONTEXT_ARCH}_${CONTEXT_ABI}_${CONTEXT_EXE}_${CONTEXT_ASM}.${CONTEXT_EXT}
	)

	add_library(${PRJ_TARGET}_context STATIC
		${headers_core}
		${src_core}
		${src_context}
	)
	target_include_directories(${PRJ_TARGET}_context PUBLIC
		${public_includes_core}
	)
	target_include_directories(${PRJ_TARGET}_context PRIVATE
		${private_includes_core}
	)
	target_compile_definitions(${PRJ_TARGET}_context PUBLIC
		KORO_BACKEND_CONTEXT
	)
endif()

# -----------------------------------------------------------------------------

if(BACKEND_THREADS)
	list(APPEND headers_threads
		${ROOT}/src/backends/threads/tinycthread.h
	)
	list(APPEND src_threads
		${ROOT}/src/backends/threads/tinycthread.c
		${ROOT}/src/backends/threads/threads_backend.c
	)
	set(private_includes_threads
		${ROOT}/src/backends/threads
	)

	add_library(${PRJ_TARGET}_threads STATIC
		${headers_core}
		${src_core}
		${headers_threads}
		${src_threads}
	)
	target_include_directories(${PRJ_TARGET}_threads PUBLIC
		${public_includes_core}
	)
	target_include_directories(${PRJ_TARGET}_threads PRIVATE
		${private_includes_core}
		${private_includes_threads}
	)
	target_compile_definitions(${PRJ_TARGET}_threads PUBLIC
		KORO_BACKEND_THREADS
	)
endif()

# -----------------------------------------------------------------------------

if(EXAMPLES)
	if(BACKEND_SWITCH)
		add_executable(${PRJ_TARGET}_test_switch examples/test.c)
		target_link_libraries(${PRJ_TARGET}_test_switch PUBLIC ${PRJ_TARGET}_switch)

		add_executable(${PRJ_TARGET}_benchmark_switch examples/benchmark.c)
		target_link_libraries(${PRJ_TARGET}_benchmark_switch PUBLIC ${PRJ_TARGET}_switch)
	endif()

	if(BACKEND_CONTEXT)
		add_executable(${PRJ_TARGET}_test_context examples/test.c)
		target_link_libraries(${PRJ_TARGET}_test_context PUBLIC ${PRJ_TARGET}_context)

		add_executable(${PRJ_TARGET}_benchmark_context examples/benchmark.c)
		target_link_libraries(${PRJ_TARGET}_benchmark_context PUBLIC ${PRJ_TARGET}_context)
	endif()

	if(BACKEND_THREADS)
		add_executable(${PRJ_TARGET}_test_threads examples/test.c)
		target_link_libraries(${PRJ_TARGET}_test_threads PUBLIC ${PRJ_TARGET}_threads)

		add_executable(${PRJ_TARGET}_benchmark_threads examples/benchmark.c)
		target_link_libraries(${PRJ_TARGET}_benchmark_threads PUBLIC ${PRJ_TARGET}_threads)
	endif()
endif()
