cmake_minimum_required(VERSION 3.0.0)

project(asio_test VERSION 0.1.0)

#============ private environment setting  ===================
#set(CMAKE_C_COMPILER "clang.exe" )
#set(CMAKE_CXX_COMPILER "clang++.exe")
#set(VCPKG_ROOT "D:/proj/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE PATH "")
#set(CMAKE_TOOLCHAIN_FILE ${VCPKG_ROOT})
#============================================================

#set(PROJECT_SOURCE_DIR ../)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

include_directories(${PROJECT_SOURCE_DIR}/include)

find_package(asio CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

function(set_lib_link execute )
target_link_libraries(${execute} PRIVATE asio asio::asio)
target_link_libraries(${execute} PRIVATE fmt::fmt fmt::fmt-header-only)
endfunction(set_lib_link)

option(build_examples "Build examples?" ON)
if(build_examples)
    add_subdirectory(examples)
endif()