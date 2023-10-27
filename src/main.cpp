// Copyright 2023 ITD Lab Corp.All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http ://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// isc-dpl is a data processing library project for our ISC series camera.
// For more information about it, see https://github.com/ITDLab/isc-dpl.
//
// The GUI for this application is implemented using Dear ImGui.
// For more information about Dear ImGui, see https://github.com/ocornut/imgui.
// Dear ImGui is released under the MIT License.
// Dear ImGui is included in the repository and is redistributable.
//
// The Point cloud Library is used to process and display point clouds.
// PCL is not for redistribution, please refer to README.md and https://pointclouds.org/ to build the library.
// PCL is released under the BSD license.
//
// This application uses OpenCV.
// OpenCV is not for redistribution, please download OpenCV from the official OpenCV website.(https://opencv.org/)
// OpenCV is open source and released under the Apache 2 License.(OpenCV 4.5.0 and higher)

/**
 * @file main.cpp
 * @brief This is the main process of the 3D display application
 * @author Takayuki
 * @date 2023.10.1
 * @version 0.1
 *
 * @details This application is for 3D display using the isc-dpl library.
 * 
 */

#include <iostream>

#include "isc_dpl_error_def.h"
#include "isc_dpl_def.h"
#include "isc_dpl.h"
#include "dpl_controll.h"
#include "dpl_support.h"
#include "pcl_def.h"
#include "pcl_support.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "GLFW/glfw3.h" 

#include "gui_support.h"
#include "win_support.h"

#pragma comment (lib, "shlwapi")
#pragma comment (lib, "opengl32")
#pragma comment (lib, "glfw3")
#pragma comment (lib, "IscDpl")

#ifdef _DEBUG
#pragma comment (lib, "opencv_world480d")
#else
#pragma comment (lib, "opencv_world480")
#endif

/**
 * 各モジュールを初期化します
 *
 * @param[in] module_path 現在実行中の実行ファイルのフルパス
 * @param[in] window GLFW windowのポインタ
 * @param[in] image_state dpl制御用構造体
 * 
 * @retval 0 成功
 * @retval other 失敗
 */
int Initialize(const wchar_t* module_path, GLFWwindow** window, ImageState* image_state)
{
    // initizlize
    int ret = InitializeDplControl(module_path, image_state);
    if (ret != 0) {
        return -1;
    }

    const int camera_model = GetCameraModel(image_state);
    const bool enabled_camera = GetCameraEnabled(image_state);

    InitializeWindowParameter initialze_window_parameter = {};
    initialze_window_parameter.max_width                        = image_state->width;
    initialze_window_parameter.max_height                       = image_state->height;
    initialze_window_parameter.enable_camera                    = enabled_camera;
    initialze_window_parameter.enabled_3d_viewer                = true;
    initialze_window_parameter.enable_data_processing_library   = false;
    initialze_window_parameter.dra_min_distance                 = GetDrawMinDistance(image_state);
    initialze_window_parameter.dra_max_distance                 = GetDrawMaxDistance(image_state);

    switch (camera_model) {
    case 0:// VM
        initialze_window_parameter.enable_data_processing_library = true;
        break;

    case 1:// XC
        initialze_window_parameter.enable_data_processing_library = true;
        break;
    }

    *window = InitializeWindow(&initialze_window_parameter);
    if (*window == NULL) {
        return -1;
    }

    int xpos = 0;
    int ypos = 0;
    glfwGetWindowPos(*window, &xpos, &ypos);

    VizParameters viz_parameters = {};
    viz_parameters.viz_position_x       = xpos + 420;
    viz_parameters.viz_position_y       = ypos + 20;
    viz_parameters.viz_width            = image_state->width;
    viz_parameters.viz_height           = image_state->height;
    viz_parameters.width                = image_state->width;
    viz_parameters.height               = image_state->height;
    viz_parameters.d_inf                = image_state->dinf;
    viz_parameters.base_length          = image_state->b;
    viz_parameters.bf                   = image_state->bf;
    viz_parameters.min_distance         = GetDrawMinDistance(image_state);
    viz_parameters.max_distance         = GetDrawMaxDistance(image_state);
    viz_parameters.coordinate_system    = true;
    viz_parameters.full_screen_request  = false;

    wchar_t record_path[_MAX_PATH] = {};
    bool ret_getpath = GetDataRecordPath(image_state, record_path, _MAX_PATH);

    char pcd_write_folder[_MAX_PATH] = {};
    ConvertWidecharToMbcs(record_path, pcd_write_folder, _MAX_PATH);

    sprintf(viz_parameters.pcd_file_write_folder, "%s", pcd_write_folder);

    ret = InitializePclViz(&viz_parameters);
    if (ret != 0) {
        return -1;
    }

    ret = InitializeWindowCameraControl(enabled_camera, image_state);
    if (ret != 0) {
        return -1;
    }

    return 0;
}

/**
 * リソースを解放します
 *
 * @param[in] window GLFW windowのポインタ
 * @param[in] image_state dpl制御用構造体
 * 
 * @retval 0 成功
 * @retval other 失敗
 */
int Terminate(GLFWwindow** window, ImageState* image_state)
{
    // ended
    int ret = TerminatePclViz();

    ret = TerminateWindow();

    ret = TerminateDplControl(image_state);

    return 0;
}

/**
 * 一連の処理を実行します
 *
 * @param[in] window GLFW windowのポインタ
 * @param[in] image_state dpl制御用構造体
 * 
 * @retval 0 成功
 * @retval other 失敗
 */
int Run(GLFWwindow* window, ImageState* image_state)
{
    // Main loop
    while (!glfwWindowShouldClose(window)) {
        //const int display_scale = 1;
        //const int display_mode = 0;
        // for debug,  display only using OpenCV
        //int ret = ImageHandlerForDplControl(display_scale, display_mode, image_state);
        //if (ret != 0) {
        //    return -1;
        //}

        // draw window
        int ret = DrawWindow(image_state);
        if (ret != 0) {
            return -1;
        }
    }

    return 0;
}

/**
 * main関数です
 *
 * @param[in] argc 引数の数
 * @param[in] argv 実行時引数
 * 
 * @retval 0 成功
 * @retval other 失敗
 */
int main(int argc, char* argv[]) try
{
    /*
        カメラの指定などの実行時パラメータは、DPLGuiConfig.ini　より取得します
    */


    InitForWinConsole();

    // get operating environment
    wchar_t module_path[_MAX_PATH] = {};
    GetModulePath(module_path, _MAX_PATH);

    // initialize modules
    ImageState image_state = {};
    GLFWwindow* window = nullptr;
    int ret = Initialize(module_path, &window, &image_state);
    if (ret != 0) {
        return 0;
    }

    // main process
    ret = Run(window, &image_state);

    if (ret != 0) {
        Terminate(&window, &image_state);
        return 0;
    }

    // ended
    ret = Terminate(&window, &image_state);

    return 0;
}
catch (const std::exception& e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}

