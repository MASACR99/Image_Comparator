cmake_minimum_required(VERSION 3.19)
project(untitled)

set(CMAKE_CXX_STANDARD 17)

set(unofficial-sqlite3_DIR "<path to vcpkg>/vcpkg/installed/x64-linux/share/unofficial-sqlite3")
find_package(unofficial-sqlite3 REQUIRED)

set(Boost_DIR "<path to vcpkg>/vcpkg/installed/x64-linux/share/boost")
find_package(Boost COMPONENTS filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIB_DIR})

set(OpenCV_DIR "<path to vcpkg>/vcpkg/installed/x64-linux/share/opencv")
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
link_directories(${OpenCV_LIB_DIR})

add_executable(untitled main.cpp main.h)
target_link_libraries(untitled ${OpenCV_LIBS})
target_link_libraries(untitled unofficial::sqlite3::sqlite3)
target_link_libraries(untitled ${Boost_LIBRARIES})

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "Image_Comparator")