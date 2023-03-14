cmake_minimum_required (VERSION 3.0 FATAL_ERROR)
project (zxpac2 LANGUAGES C)
set(TARGET zxpac2)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/build)
set(CMAKE_INSTALL_PREFIX ${PROJECT_BINARY_DIR})

set(CMAKE_C_STANDARD 99)
set(CMAKE_BUILD_TYPE DEBUG)
#set(CMAKE_BUILD_TYPE RELEASE)
set(CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG} )
#set(CMAKE_C_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE} -O3 )

# Turn off additional debug..
add_definitions(-DNDEBUG)

set(SOURCE_FILES
    zxpac_v2.c
)



add_executable(${TARGET} ${SOURCE_FILES})

if (WIN32)
        message(STATUS "Windows system")
        #set(STRIPTARGET ${TARGET}.exe)
else()
        message(STATUS "non-Windows system")
        #set(STRIPTARGET ${TARGET})
endif(WIN32)

if (CMAKE_BUILD_TYPE MATCHES RELEASE)
    message(STATUS "kukkuu pulu")
    add_custom_command(TARGET ${TARGET}
        POST_BUILD 
        COMMAND strip $<TARGET_FILE:${TARGET}>
)
endif (CMAKE_BUILD_TYPE MATCHES RELEASE)
