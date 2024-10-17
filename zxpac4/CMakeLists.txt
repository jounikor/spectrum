#
#

cmake_minimum_required(VERSION 3.10)
#

# set the project name
project(lz VERSION 0.1 LANGUAGES CXX)
set (TARGET zxpac4)
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_BUILD_TYPE DEBUG)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/build)
set(CMAKE_INSTALL_PREFIX ${PROJECT_BINARY_DIR})

# Debug mode?
#add_definitions(-DPMR_UPDATE_WITH_SHORT_OFFSET)
#add_definitions(-DMTF_CTX_SAVING_ENABLED)
add_compile_options(-Wall -Wextra -Wpedantic)

# Own configurations
#add_definitions(-DFORWARD_PARSER)
add_definitions(-DWINDOW_IS_POWER_OF_TWO)
#add_definitions(-DDEBUG_COST)

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

include_directories(inc)
#target_include_directories(${TARGET}	PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}")
#target_include_directories(${TARGET} PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/inc")

# Now set emuation stub active
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DEXAMPLE_SET_RX_CONFIG")
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

# Common files for the adftools
set(COMMON_SRC  src/cost4.cpp
                src/cost4_32k.cpp
                src/cost4b.cpp
                src/zxpac4.cpp
                src/zxpac4_32k.cpp
                src/zxpac4b.cpp
                src/hash.cpp
                src/main.cpp

                inc/lz_util.h
                inc/zxpac4.h
                inc/zxpac4_32k.h
                inc/cost4.h
                inc/cost4_32k.h
                inc/zxpac4b.h
                inc/cost4b.h
                inc/lz_base.h
                inc/hash.h
)

add_executable(${TARGET} ${COMMON_SRC})


# To install use 'cmake --install . --prefix "path-of-your-choice"'
install(TARGETS ${TARGET}		DESTINATION bin)
