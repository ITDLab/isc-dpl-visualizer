cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(dpl_visualizer_viewports)

find_package(PCL 1.2 REQUIRED)

include_directories(${PCL_INCLUDE_DIRS}
 $ENV{PCL_DIR}/include/
 $ENV{PCL_DIR}/include/eigen3/
 $ENV{PCL_DIR}/include/vtk-9.2/
 $ENV{GLEW_DIR}/include/
 $ENV{GLFW_DIR}/include/
 $ENV{OpenCV_DIR}/include/
 third-party/imgui/
 third-party/itdlab/include/
 src
)

link_directories(${PCL_LIBRARY_DIRS}
 $ENV{PCL_DIR}/lib
 $ENV{PCL_DIR}/debug/lib
 $ENV{GLEW_DIR}/lib/Release/x64
 $ENV{GLFW_DIR}/lib-vc2022
 $ENV{OpenCV_DIR}/x64/vc16/lib
 third-party/itdlab/lib
)

add_definitions(${PCL_DEFINITIONS})

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

file(GLOB imguiSrcFiles 
 ./third-party/imgui/imconfig.h
 ./third-party/imgui/imgui.cpp
 ./third-party/imgui/imgui.h
 ./third-party/imgui/imgui_demo.cpp
 ./third-party/imgui/imgui_draw.cpp
 ./third-party/imgui/imgui_internal.h
 ./third-party/imgui/imgui_tables.cpp
 ./third-party/imgui/imgui_widgets.cpp
 ./third-party/imgui/imstb_rectpack.h
 ./third-party/imgui/imstb_textedit.h
 ./third-party/imgui/imstb_truetype.h
)

file(GLOB imguibackendsSrcFiles 
 ./third-party/imgui/backends/imgui_impl_glfw.cpp
 ./third-party/imgui/backends/imgui_impl_glfw.h
 ./third-party/imgui/backends/imgui_impl_opengl3.cpp
 ./third-party/imgui/backends/imgui_impl_opengl3.h
 ./third-party/imgui/backends/imgui_impl_opengl3_loader.h
)

source_group(imgui FILES ${imguiSrcFiles})
source_group(imgui-backends FILES ${imguibackendsSrcFiles})

add_executable (dpl_visualizer 
 ${imguiSrcFiles}
 ${imguibackendsSrcFiles}
 ./src/main.cpp
 ./src/dpl_control.cpp
 ./src/dpl_controll.h
 ./src/dpl_gui_configuration.cpp
 ./src/dpl_gui_configuration.h
 ./src/dpl_support.cpp
 ./src/dpl_support.h
 ./src/gui_support.cpp
 ./src/gui_support.h
 ./src/pcl_data_ring_buffer.cpp
 ./src/pcl_data_ring_buffer.h
 ./src/pcl_def.h
 ./src/pcl_support.cpp
 ./src/pcl_support.h
 ./src/win_support.cpp
 ./src/win_support.h
)

target_link_libraries (dpl_visualizer ${PCL_LIBRARIES})


