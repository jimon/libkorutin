
cmake_minimum_required(VERSION 3.3)

# -----------------------------------------------------------------------------

option(BACKEND_SWITCH     "build switch (asm based) backend (fast)"  ON)
option(BACKEND_CONTEXT    "build boost context based backend (fast)" ON)
option(BACKEND_THREADS    "build pthread based backend (slow)"       ON)
option(BACKEND_EMSCRIPTEN "build emscripten based backend (slow)"    OFF)
option(EXAMPLES           "build examples"                           ON)

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

# -----------------------------------------------------------------------------
# enable asm in cmake

if(NOT EMSCRIPTEN)
	if(WIN32 AND NOT MINGW)
		enable_language(ASM_MASM)
		if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
			set(CMAKE_ASM_MASM_COMPILER ml64) # TODO
		endif()
	else()
		enable_language(ASM)
	endif()
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
	# CONTEXT_ARCH possible values: arm, arm64, i386, mips32, ppc32, ppc64, x86_64
	# CONTEXT_ABI  possible values: aapcs, ms, sysv, o32, ppc64
	# CONTEXT_EXE  possible values: elf, macho, pe, xcoff
	# CONTEXT_ASM  possible values: gas, masm, armasm
	# CONTEXT_EXT  possible values: asm, S

	if(WIN32)
		if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
			set(CONTEXT_ARCH "x86_64")
		else()
			set(CONTEXT_ARCH "i386")
		endif()
		set(CONTEXT_ABI "ms")
	elseif(APPLE)
		# chances are you will only run cmake on MacOS
		if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
			set(CONTEXT_ARCH "x86_64")
		else()
			set(CONTEXT_ARCH "i386")
			set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} -arch i386") # TODO
		endif()
		set(CONTEXT_ABI "sysv")
	elseif(ANDROID_PLATFORM)
		# see https://developer.android.com/ndk/guides/cmake.html
		# "armeabi", "armeabi-v7a", "armeabi-v7a with NEON", "arm64-v8a", "mips", "mips64", "x86", "x86_64"
		if("${ANDROID_ABI}" STREQUAL "armeabi" OR "${ANDROID_ABI}" STREQUAL "armeabi-v7a" OR "${ANDROID_ABI}" STREQUAL "armeabi-v7a with NEON")
			set(CONTEXT_ARCH "arm")
			set(CONTEXT_ABI "aapcs")
		elseif("${ANDROID_ABI}" STREQUAL "arm64-v8a")
			set(CONTEXT_ARCH "arm64")
			set(CONTEXT_ABI "aapcs")
		elseif("${ANDROID_ABI}" STREQUAL "mips")
			set(CONTEXT_ARCH "mips32")
			set(CONTEXT_ABI "o32")
		elseif("${ANDROID_ABI}" STREQUAL "x86")
			set(CONTEXT_ARCH "i386")
			set(CONTEXT_ABI "sysv")
		elseif("${ANDROID_ABI}" STREQUAL "x86_64")
			set(CONTEXT_ARCH "x86_64")
			set(CONTEXT_ABI "sysv")
		else()
			message(FATAL_ERROR "unsupported flavour of android '${ANDROID_ABI}'")
		endif()
	else()
		message(WARNING "unknown ARCH/ABI, fallback to i386/x86_64/sysv")
		if("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
			set(CONTEXT_ARCH "x86_64")
		else()
			set(CONTEXT_ARCH "i386")
		endif()
		set(CONTEXT_ABI "sysv")
	endif()

	if(WIN32)
		set(CONTEXT_EXE "pe")
	elseif(APPLE)
		set(CONTEXT_EXE "macho")
	else()
		set(CONTEXT_EXE "elf")
		# TODO xcoff
	endif()

	if(WIN32 AND NOT MINGW)
		set(CONTEXT_ASM "masm") # TODO when we need armasm?
		set(CONTEXT_EXT "asm")
	else()
		set(CONTEXT_ASM "gas")
		set(CONTEXT_EXT "S")
	endif()

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
	target_compile_definitions(${PRJ_TARGET}_context PRIVATE
		BOOST_CONTEXT_EXPORT # TODO remove this later
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
	if(NOT WIN32)
		target_link_libraries(${PRJ_TARGET}_threads PRIVATE pthread)
	endif()

	if(EMSCRIPTEN)
		# it should work in theory, but https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/SharedArrayBuffer says:
		# "Note that SharedArrayBuffer was disabled by default in all major browsers on 5 January, 2018 in response to Spectre."
		# and looks like the only browsers that supported this out of the box was Chrome 60+ and Firefox 57+
		target_compile_options(${PRJ_TARGET}_threads PUBLIC "-s" "USE_PTHREADS=1")
		set_target_properties(${PRJ_TARGET}_threads PROPERTIES LINK_FLAGS "-s USE_PTHREADS=1")
	endif()
endif()

# -----------------------------------------------------------------------------

if(BACKEND_EMSCRIPTEN)
	if(NOT EMSCRIPTEN)
		message(FATAL_ERROR emscripten backend only builds with emscripten)
	endif()

	set(src_emscripten
		${ROOT}/src/backends/emscripten/emscripten_backend.c
	)

	add_library(${PRJ_TARGET}_emscripten STATIC
		${headers_core}
		${src_core}
		${src_emscripten}
	)
	target_include_directories(${PRJ_TARGET}_emscripten PUBLIC
		${public_includes_core}
	)
	target_include_directories(${PRJ_TARGET}_emscripten PRIVATE
		${private_includes_core}
	)
	target_compile_definitions(${PRJ_TARGET}_emscripten PUBLIC
		KORO_BACKEND_EMSCRIPTEN
	)

	target_compile_options(${PRJ_TARGET}_emscripten PUBLIC "-s" "ASYNCIFY=1")
	set_target_properties(${PRJ_TARGET}_emscripten PROPERTIES LINK_FLAGS "-s ASYNCIFY=1")
endif()

# -----------------------------------------------------------------------------

if(EXAMPLES)
	if(EMSCRIPTEN)
		set(CMAKE_EXECUTABLE_SUFFIX ".html")
	endif()

	if(BACKEND_SWITCH)
		add_executable(${PRJ_TARGET}_test_switch ${ROOT}/examples/test.c)
		target_link_libraries(${PRJ_TARGET}_test_switch PUBLIC ${PRJ_TARGET}_switch)

		add_executable(${PRJ_TARGET}_benchmark_switch ${ROOT}/examples/benchmark.c)
		target_link_libraries(${PRJ_TARGET}_benchmark_switch PUBLIC ${PRJ_TARGET}_switch)
	endif()

	if(BACKEND_CONTEXT)
		add_executable(${PRJ_TARGET}_test_context ${ROOT}/examples/test.c)
		target_link_libraries(${PRJ_TARGET}_test_context PUBLIC ${PRJ_TARGET}_context)

		add_executable(${PRJ_TARGET}_benchmark_context ${ROOT}/examples/benchmark.c)
		target_link_libraries(${PRJ_TARGET}_benchmark_context PUBLIC ${PRJ_TARGET}_context)
	endif()

	if(BACKEND_THREADS)
		add_executable(${PRJ_TARGET}_test_threads ${ROOT}/examples/test.c)
		target_link_libraries(${PRJ_TARGET}_test_threads PUBLIC ${PRJ_TARGET}_threads)

		add_executable(${PRJ_TARGET}_benchmark_threads ${ROOT}/examples/benchmark.c)
		target_link_libraries(${PRJ_TARGET}_benchmark_threads PUBLIC ${PRJ_TARGET}_threads)

		if(EMSCRIPTEN)
			target_compile_options(${PRJ_TARGET}_test_threads PUBLIC "-s" "USE_PTHREADS=1")
			set_target_properties(${PRJ_TARGET}_test_threads PROPERTIES LINK_FLAGS "-s USE_PTHREADS=1")

			target_compile_options(${PRJ_TARGET}_benchmark_threads PUBLIC "-s" "USE_PTHREADS=1")
			set_target_properties(${PRJ_TARGET}_benchmark_threads PROPERTIES LINK_FLAGS "-s USE_PTHREADS=1")
		endif()
	endif()

	if(BACKEND_EMSCRIPTEN)
		add_executable(${PRJ_TARGET}_test_emscripten ${ROOT}/examples/test.c)
		target_link_libraries(${PRJ_TARGET}_test_emscripten PUBLIC ${PRJ_TARGET}_emscripten)
		target_compile_options(${PRJ_TARGET}_test_emscripten PUBLIC "-s" "ASYNCIFY=1")
		set_target_properties(${PRJ_TARGET}_test_emscripten PROPERTIES LINK_FLAGS "-s ASYNCIFY=1 -s TOTAL_MEMORY=268435456")

		add_executable(${PRJ_TARGET}_benchmark_emscripten ${ROOT}/examples/benchmark.c)
		target_link_libraries(${PRJ_TARGET}_benchmark_emscripten PUBLIC ${PRJ_TARGET}_emscripten)
		target_compile_options(${PRJ_TARGET}_benchmark_emscripten PUBLIC "-s" "ASYNCIFY=1")
		set_target_properties(${PRJ_TARGET}_benchmark_emscripten PROPERTIES LINK_FLAGS "-s ASYNCIFY=1 -s TOTAL_MEMORY=268435456")
	endif()
endif()

# -----------------------------------------------------------------------------

if(ANDROID_PLATFORM)
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

	add_library(${PRJ_TARGET} SHARED
		${ROOT}/android/app/src/cpp/main.c
		${ROOT}/examples/benchmark.c
		${ROOT}/examples/benchmark.h
	)
	target_include_directories(${PRJ_TARGET} PRIVATE
		${ROOT}/examples
		${ANDROID_NDK}/sources/android/native_app_glue
	)
	target_link_libraries(${PRJ_TARGET} PUBLIC
		${PRJ_TARGET}_context
		android
		log
	)
	target_compile_definitions(${PRJ_TARGET} PRIVATE
		KORO_BENCHMARK_NO_MAIN
	)
endif()
