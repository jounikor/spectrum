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
add_definitions(-DWINDOW_IS_POWER_OF_TWO)

#
if (DEFINED CMAKE_BUILD_TYPE)
	if (CMAKE_BUILD_TYPE STREQUAL "")
		message("Defaulting to Debug")
		set(CMAKE_BUILD_TYPE Debug)
	endif()

	message("Build type define as ${CMAKE_BUILD_TYPE}")
	
	if (CMAKE_BUILD_TYPE STREQUAL "Release")
		#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O -Werror -Wall -Wextra -Wshadow")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O -Wall -Wextra -Wshadow")
	else()
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -Wall -Wextra -Wshadow")
	endif()
endif()

include_directories(inc)
#target_include_directories(${TARGET}	PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}")
#target_include_directories(${TARGET} PUBLIC "${PROJECT_BINARY_DIR}" "${PROJECT_SOURCE_DIR}" "${PROJECT_SOURCE_DIR}/inc")

# Common files for the adftools
set(COMMON_SRC  src/cost4.cpp
                src/cost4_32k.cpp
                src/cost4b.cpp
                src/zxpac4.cpp
                src/zxpac4_32k.cpp
                src/zxpac4b.cpp
                src/hash.cpp
                src/main.cpp
                src/hunk.cpp
                src/target_bin.cpp
                src/target_asc.cpp
                src/target_bbc.cpp
                src/target_amiga.cpp
                src/target_spectrum.cpp

                inc/lz_util.h
                inc/zxpac4.h
                inc/zxpac4_32k.h
                inc/cost4.h
                inc/cost4_32k.h
                inc/zxpac4b.h
                inc/cost4b.h
                inc/lz_base.h
                inc/hash.h
                inc/hunk.h
                inc/target.h
)

add_executable(${TARGET} ${COMMON_SRC})

# Add m68k decompressor targets
add_subdirectory(m68k)

# Now set emuation stub active
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DEXAMPLE_SET_RX_CONFIG")
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

# Add cmakelist for Amiga target decompressors
#set (ZXPAC4_DECOMPRESSOR_ASM_LIST zxpac4_exe zxpac4_32k_exe zxpac4_abs zxpac4_32k_abs) 
#
#foreach(BASE ${ZXPAC4_DECOMPRESSOR_ASM_LIST})
#    string(CONCAT FILE_ASM ${BASE} ".asm")
#    string(CONCAT FILE_BIN ${BASE} ".bin")
#    string(CONCAT FILE_H ${BASE} ".h")
#    string(CONCAT FILE_LABEL ${BASE} "_bin")
#    string(CONCAT FILE_TARGET ${BASE} "_target")
#
#    add_custom_target(${FILE_TARGET}
#        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/m68k/${FILE_H}
#        COMMENT "Target ${FILE_TARGET} for  ${FILE_H}"
#        VERBATIM
#    )
#    
#    add_dependencies(${TARGET} ${FILE_TARGET})
#    
#    add_custom_command(
#        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/m68k/${FILE_H}
#        COMMAND vasmm68k_mot -Fbin ${FILE_ASM} -o ${FILE_BIN}
#        COMMAND python3 bin2c.py ${FILE_LABEL} ${FILE_BIN} ${FILE_H}
#        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/m68k/
#        COMMENT "Creating ${FILE_H} from ${FILE_ASM} and ${FILE_BIN} for target ${FILE_TARGET}"
#        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/m68k/${FILE_ASM}
#        VERBATIM
#    )
#endforeach()


# To install use 'cmake --install . --prefix "path-of-your-choice"'
install(TARGETS ${TARGET}		DESTINATION bin)
