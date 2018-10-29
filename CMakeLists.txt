cmake_minimum_required(VERSION 3.5)
project(main)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_CXX_STANDARD 11)

SET(VERSION_MAJOR "0")
SET(VERSION_MINOR "1")
SET(PATCH_NUMBER  "0")

SET(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${PATCH_NUMBER}")

find_package(LIBWEBSOCKETS REQUIRED)
find_package(CURL REQUIRED)
find_package(Boost COMPONENTS system filesystem program_options iostreams unit_test_framework REQUIRED)
find_package(TBB REQUIRED)
find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)
#find_package(OpenSSL REQUIRED)

#set(LIBWEBSOCKETS_LIBRARIES /home/vladimir/work/trading/libwebsockets/build/lib/libwebsockets.a)

set(LINK_LIBS ${Boost_LIBRARIES} ${LIBWEBSOCKETS_LIBRARIES} 
  ${CURL_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${TBB_LIBRARIES} ${ZLIB_LIBRARIES} -lrt -lssl -lcrypto)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -p -Wall -Wno-unused-variable -Wno-braced-scalar-init -O3 ${RAPIDJSON_CXX_FLAGS} -march=native")

include_directories(${PROJECT_SOURCE_DIR}/pjson/)
include_directories(${PROJECT_SOURCE_DIR}/src/exchange/)
include_directories(${PROJECT_SOURCE_DIR}/src ${TBB_INCLUDE_DIRS} ${Boost_INCLUDE_DIR} ${LIBWEBSOCKETS_INCLUDE_DIR} 
                    ${CURL_INCLUDE_DIR} ${RAPIDJSON_INCLUDE_DIRS})
link_directories(${LIBWEBSOCKETS_LIBRARY_DIR} ${CURL_LIBRARY_DIR})

file(GLOB_RECURSE FIN_SOURCES ${PROJECT_SOURCE_DIR}/src/fin/*.cpp)
file(GLOB_RECURSE PLATFORM_SOURCES ${PROJECT_SOURCE_DIR}/src/platform/*.cpp)
file(GLOB_RECURSE EXCHANGE_SOURCES ${PROJECT_SOURCE_DIR}/src/exchange/**/*.cpp)
set (COMMON_SOURCES ${FIN_SOURCES} ${PLATFORM_SOURCES})

add_subdirectory(tests)
add_subdirectory(doxygen)

configure_file(${PROJECT_SOURCE_DIR}/src/version.h.in 
               ${PROJECT_SOURCE_DIR}/src/version.h
               @ONLY )
