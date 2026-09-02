cmake_minimum_required(VERSION 3.5)
project(spider LANGUAGES CXX)

message ("Will create project for decomp! ${CMAKE_SYSTEM_PROCESSOR}")


if (MSVC)
	message("Creating Windows project")
	if(NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
		message(FATAL_ERROR "Please switch to x32 build.")
	endif()
else()
	string (APPEND CMAKE_CXX_FLAGS " -w -fpermissive -std=c++11")
	if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		message("Creating for G++")
		# MSVC __inline functions are still callable from other translation
		# units. Make GCC emit them too (see INLINE in my_types.h).
		string (APPEND CMAKE_CXX_FLAGS " -fkeep-inline-functions")
	elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
		message("Creating for Clang++")
		string (APPEND CMAKE_CXX_FLAGS " -fms-extensions -Wno-address-of-temporary")
	else()
		message(FATAL_ERROR "Could not detect compiler")
	endif()

	if (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
		string (APPEND CMAKE_CXX_FLAGS  " -m32")
		set (LIBZ_NAME ${CMAKE_SOURCE_DIR}/lib/libz_x86.a)
	elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
		set (LIBZ_NAME ${CMAKE_SOURCE_DIR}/lib/libz_aarch64.a)
	else()
		message(FATAL_ERROR "Could not detect architecture")
	endif()
endif()



file(GLOB SRC_FILES *.cpp)

# Phase 2: the standalone game. OFF = the old assertion-only sanity build.
option(SPIDEY_STANDALONE "Build the standalone game (platform/ + a backend)" OFF)
set(SPIDEY_BACKEND "null" CACHE STRING "Platform backend: null (headless) or sdl3")

if (SPIDEY_STANDALONE)
	message("Standalone build, backend: ${SPIDEY_BACKEND}")
	add_compile_definitions(SPIDEY_STANDALONE)
	file(GLOB PLAT_FILES platform/*.cpp platform/${SPIDEY_BACKEND}/*.cpp)
	list(APPEND SRC_FILES ${PLAT_FILES})
endif()

add_executable(${PROJECT_NAME} ${SRC_FILES})

if (UNIX)
	target_link_libraries(${PROJECT_NAME} ${LIBZ_NAME})
elseif(WIN32)
	target_link_libraries(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/lib/safesehzlib.lib winmm.lib dinput8.lib dxguid.lib dsound.lib ddraw.lib)
endif ()

if (SPIDEY_STANDALONE)
	if (UNIX)
		# exemem.cpp maps the exe's data range at 0x53B000, below the
		# 0x08048000 load address of a non-PIE 32-bit binary.
		target_compile_options(${PROJECT_NAME} PRIVATE -fno-pie)
		target_link_options(${PROJECT_NAME} PRIVATE -no-pie)
		# implicit linker script: places the G_*-twinned variables at their
		# exe addresses (see platform/exemem.h)
		target_link_libraries(${PROJECT_NAME} pthread ${CMAKE_SOURCE_DIR}/platform/exemem_syms.ld)
	endif()
	if (SPIDEY_BACKEND STREQUAL "sdl3")
		find_package(PkgConfig REQUIRED)
		if (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
			# 32 bit build on a 64 bit host: ask pkg-config for the i386 libs
			set(ENV{PKG_CONFIG_LIBDIR} "/usr/lib/i386-linux-gnu/pkgconfig:/usr/share/pkgconfig")
		endif()
		pkg_check_modules(SDL3 REQUIRED sdl3)
		target_include_directories(${PROJECT_NAME} PRIVATE ${SDL3_INCLUDE_DIRS})
		target_link_libraries(${PROJECT_NAME} ${SDL3_LIBRARIES} GL)
	endif()
endif()
