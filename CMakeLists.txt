
cmake_minimum_required(VERSION 3.16)
project(bimba VERSION 1.0) #Project name

# set(CMAKE_CXX_STANDARD 17) #setting C++ 14 standard
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++17")

find_package ( PkgConfig REQUIRED )
# find_package ( Threads REQUIRED )
pkg_check_modules(MIVO REQUIRED gstreamer-1.0 gtkmm-3.0 gtk+-3.0 gstreamer-video-1.0 gdk-3.0 libftdi1 libusb-1.0 opencv4)


#including GStreamer header files directory
include_directories(
        ${MIVO_INCLUDE_DIRS}
)

#linking GStreamer library directory
link_directories(
        ${MIVO_LIBRARY_DIRS}
)

# Add other flags to the compiler
add_definitions(${MIVO_CFLAGS_OTHER})

#building target executable
include_directories(include)

file(GLOB SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp")
add_executable(${PROJECT_NAME} ${SOURCES})

#linking library with target executable

target_link_libraries( ${PROJECT_NAME}  ${MIVO_LIBRARIES} )


