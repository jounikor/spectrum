#
#

cmake_minimum_required(VERSION 3.10)
#

# set the project name
project(lz VERSION 0.1 LANGUAGES CXX)
set (TARGET lz)
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_BUILD_TYPE RELEASE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/build)
set(CMAKE_INSTALL_PREFIX ${PROJECT_BINARY_DIR})

#
if (DEFINED CMAKE_BUILD_TYPE)
	if (CMAKE_BUILD_TYPE STREQUAL "")
		message("Defaulting to Debug")
		set(CMAKE_BUILD_TYPE Debug)
	endif()

	message("Build type define as ${CMAKE_BUILD_TYPE}")
	
	if (CMAKE_BUILD_TYPE STREQUAL "Release")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O -Werror -Wall -Wextra -Wshadow")
	else()
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -Werror -Wall -Wextra -Wshadow")
	endif()
endif()


# Now set emuation stub active
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DEXAMPLE_SET_RX_CONFIG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

# Common files for the adftools
set(COMMON_SRC  bffp_cost.cpp
                mtf256.cpp
                pmr.cpp
                bffp.cpp
                lz_utils.cpp
                z_alg.cpp

                bffp.h
                bffp_cost.h
                bffp_encoding.h
                lz_base.h
                lz_utils.h
                z_alg.h
                mtf256.h
                pmr.h
)

add_executable(${TARGET} ${COMMON_SRC})

target_include_directories(${TARGET}	PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}")
#target_include_directories(mod	PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/nrfipclib")

# To install use 'cmake --install . --prefix "path-of-your-choice"'
install(TARGETS ${TARGET}		DESTINATION bin)
