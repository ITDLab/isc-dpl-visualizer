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

/**
 * @file gui_support.cpp
 * @brief Controls Gui components and screen rendering.
 * @author Takayuki
 * @date 2023.10.30
 * @version 0.1
 * 
 * @details Controls GUI components using ImGui and calls for 2D drawing and 3D drawing.
 */

#include <windows.h>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>

#include "opencv2/opencv.hpp"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "GLFW/glfw3.h" 

#include "isc_dpl_error_def.h"
#include "isc_dpl_def.h"
#include "isc_dpl.h"
#include "dpl_controll.h"
#include "dpl_support.h"
#include "pcl_def.h"
#include "pcl_support.h"

#include "gui_support.h"
#include "win_support.h"

// 
// imgui content
// 

GLFWwindow* window_ = NULL;                                 /**< GLFW Window */
GLuint texture_[2] = {};                                    /**< Texture Buffer */
ImVec4 clear_color_ = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);   /**< 画面のクリアー色 RGBA(rga/256) */

// 
// GUI controls
// 

/** @enum  GrabMode
 *  @brief カメラの取り込みモード
 */
enum class GrabMode {
    Disparity,      /**< 視差モード */
    Corrected,      /**< 補正後画像 */
    NotCorrected    /**< 補正前画像 */
};

/** @struct  GuiLocationInfo
 *  @brief GUIコンポーネントの設定用
 */
struct GuiLocationInfo {
    POINT position;
    SIZE size;
};

/** @struct  GuiComponent
 *  @brief GUIコンポーネントの設定用
 */
struct GuiComponent {
    int min_value;
    int max_value;
};

/** @struct  GuiControls
 *  @brief GUIコンポーネントの制御用
 */
struct GuiControls {
    // Window display position and size
    GuiLocationInfo gui_loc_main_window;
    GuiLocationInfo gui_loc_control;
    GuiLocationInfo gui_loc_images[2];
    GuiLocationInfo gui_loc_3d_image;

    // GUI components
    bool enabled_viz_mode_3d;           /**< 3D function on/off true:enabled false:disabled */
    bool viz_mode_3d;                   /**< false:2D true:3D */
    bool viz_mode_3d_im_src_depth_heat; /**< 3D base image is -> false:camera input true:distance heat map */
    bool viz_mode_3d_full_screen;       /**< 3D full screen on */

    bool grab;                          /**< start grab request*/
    bool play;                          /**< start playback from a file */
    bool record;                        /**< start recording */

    int grab_mode;                      /**< grab mode 0:disparity 1:correct image 2:before correct image */

    bool enable_data_processing_library;    /**< data processing lib on/off */
    bool stereo_matching;                   /**< enable function */
    bool disparity_filter;                  /**< enable function */
    bool color;                             /**< enable function */

    // camera
    bool enable_camera;                     /** < with or without camera connection */
    int shutter_mode;                       /**< select mode: { "Manual", "Single", "Double1", "Double2"} */
    int shutter_mode_item_count;            /**< support function , max index */
    int exposure_value;                     /**< set exposure time */
    GuiComponent exposure_value_component;  /**< range of values */
    int gain_value;                         /**< set gain value */
    GuiComponent gain_value_component;      /**< range of values */
    bool auto_adjust;                       /**< enable auto-adjustment */
    bool post_camera_status_request;        /**< Request to update camera information later */
    bool update_camera_status_request;      /**< Request to update camera information */

    // play data information
    wchar_t play_file_name[_MAX_PATH];		/**< file name for read data */

    // PCL filter
    PclFilterParameter pcl_filter_parameter;    /**< filter parameter for PCL vivualization*/

    // PCL visualizer request flags
    bool viz_mode_3d_full_screen_req;       /**< 3D full screen on */
    bool viz_mode_3d_restore_screen_req;    /**< 3D full screen off */

    // current status
    bool is_grab_in_operation;          /**< current operating status */
    bool is_3d_viz;                     /**< current operating status */
};
GuiControls gui_control_ = {};          /**< GUIコンポーネント制御パラメータ */

PclVizInputArgs input_args_ = {};       /**< PCL表示用　入力データ */
PclVizOutputArgs output_args_ = {};     /**< PCL表示モジュールからの出力データ */

// 
// DPL mode
// 
DplControl::StartMode dpl_control_start_mode_ = {}; /**< 開始モード */

// 
// image buffers
// 
constexpr int kBUFFER_COUNT_MAX = 8;    /**< 作業用バッファー最大数 */

/** @struct  ImageDataBuffers
 *  @brief 画像処理用Buffer
 */
struct ImageDataBuffers {
    struct ImageType {
        int width;
        int height;
        int channel_count;
        unsigned char* image;
    };

    struct DepthType {
        int width;
        int height;
        float* depth;
    };

    int max_width, max_height;

    int buffer_image_count;
    ImageType buffer_image[kBUFFER_COUNT_MAX];

    int buffer_depth_count;
    DepthType buffer_depth[kBUFFER_COUNT_MAX];

    int draw_image_count;
    ImageType draw_image[kBUFFER_COUNT_MAX];

};
ImageDataBuffers image_buffers_ = {};   /**< 作業用バッファー */

// 
// functions
// 
int DrawControl(GuiControls& gui_control, ImageState* image_state);
int ProcedureControl(GuiControls& gui_control_previous, GuiControls& gui_control_latest, DplControl::StartMode& dpl_control_start_mode_latest, ImageState* image_state);
int DrawDplImages(GuiControls& gui_control_latest, ImageState* image_state, GLuint* texture, ImageDataBuffers* image_buffers);
int DrawPCLVizImage(GuiControls& gui_control_latest, DplControl::StartMode& dpl_control_start_mode_latest, ImageState* image_state, ImageDataBuffers* image_buffers,
                    PclVizInputArgs* input_args, PclVizOutputArgs* output_args);

// 
// implementations
// 

/**
 * GLFWのError Call Back関数.
 *
 * @param[in] error Error Code
 * @param[in] description 追加説明文
 *
 * @retval true 成功
 * @retval false 失敗
 */
static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "[ERROR]Glfw Error %d: %s\n", error, description);
}

/**
 * GLFW Windowの生成と初期化.
 *
 * @param[in] initialze_window_parameter 初期化設定
 *
 * @retval GLFWwindow* 成功
 * @retval NULL 失敗
 */
GLFWwindow* InitializeWindow(InitializeWindowParameter* initialze_window_parameter)
{
    // initialize parameter
    gui_control_.gui_loc_main_window.position   = { 0, 32 };
    gui_control_.gui_loc_main_window.size       = { 1900, 1000 };

    gui_control_.gui_loc_control.position       = { 0, 0 };
    gui_control_.gui_loc_control.size           = { 400, 800 };

    gui_control_.gui_loc_images[0].position     = {gui_control_.gui_loc_control.size.cx, 0};
    gui_control_.gui_loc_images[0].size         = { 1280, 720 };

    gui_control_.gui_loc_images[1].position     = {gui_control_.gui_loc_control.size.cx + 100, gui_control_.gui_loc_control.position.y + 100 };
    gui_control_.gui_loc_images[1].size         = { 1280, 720 };

    gui_control_.grab       = false;
    gui_control_.play       = false;
    gui_control_.record     = false;
    gui_control_.grab_mode  = (int)GrabMode::Corrected;

    gui_control_.enable_data_processing_library = initialze_window_parameter->enable_data_processing_library;
    gui_control_.stereo_matching                = true;
    gui_control_.disparity_filter               = true;
    gui_control_.color                          = false;

    gui_control_.enable_camera  = initialze_window_parameter->enable_camera;
    gui_control_.shutter_mode   = 0;
    gui_control_.exposure_value = 0;
    gui_control_.gain_value     = false;
    gui_control_.auto_adjust    = false;

    gui_control_.enabled_viz_mode_3d                = initialze_window_parameter->enabled_3d_viewer;
    gui_control_.viz_mode_3d                        = false;
    gui_control_.viz_mode_3d_im_src_depth_heat      = false;
    gui_control_.viz_mode_3d_full_screen            = false;

    gui_control_.viz_mode_3d_full_screen_req        = false;
    gui_control_.viz_mode_3d_restore_screen_req     = false;


    gui_control_.is_grab_in_operation           = false;
    gui_control_.is_3d_viz                      = false;

    gui_control_.pcl_filter_parameter.enabled_remove_nan                            = true;
    gui_control_.pcl_filter_parameter.enabled_pass_through_filter                   = true;
    gui_control_.pcl_filter_parameter.pass_through_filter_range.min                 = std::max(0.1, initialze_window_parameter->dra_min_distance);
    gui_control_.pcl_filter_parameter.pass_through_filter_range.max                 = std::min(40.0, initialze_window_parameter->dra_max_distance);
    gui_control_.pcl_filter_parameter.enabled_down_sampling                         = false;
    gui_control_.pcl_filter_parameter.down_sampling_boxel_size                      = 0.01f;
    gui_control_.pcl_filter_parameter.enabled_radius_outlier_removal                = false;
    gui_control_.pcl_filter_parameter.radius_outlier_removal_param.radius_search    = 0.15;
    gui_control_.pcl_filter_parameter.radius_outlier_removal_param.min_neighbors    = 100;
    gui_control_.pcl_filter_parameter.enabled_plane_detection                       = false;
    gui_control_.pcl_filter_parameter.plane_detection_threshold                     = 0.2;

    input_args_;
    output_args_.pick_information.max_count = 4;
    output_args_.pick_information.count = 0;
    for (int i = 0; i < output_args_.pick_information.max_count; i++) {
        output_args_.pick_information.pick_data[i].valid = false;
        output_args_.pick_information.pick_data[i].x = 0.0f;
        output_args_.pick_information.pick_data[i].y = 0.0f;
        output_args_.pick_information.pick_data[i].z = 0.0f;
    }

    // initialize buufer
    image_buffers_.max_width = initialze_window_parameter->max_width;
    image_buffers_.max_height = initialze_window_parameter->max_height;

    image_buffers_.buffer_image_count   = MIN(kBUFFER_COUNT_MAX, 6);
    image_buffers_.buffer_depth_count   = MIN(kBUFFER_COUNT_MAX, 2);
    image_buffers_.draw_image_count     = MIN(kBUFFER_COUNT_MAX, 2);

    const size_t frame_size = image_buffers_.max_width * image_buffers_.max_height;
    for (int i = 0; i < image_buffers_.buffer_image_count; i++) {
        image_buffers_.buffer_image[i].image = new unsigned char[frame_size * 4];
    }
    for (int i = 0; i < image_buffers_.buffer_depth_count; i++) {
        image_buffers_.buffer_depth[i].depth = new float[frame_size];
    }
    for (int i = 0; i < image_buffers_.draw_image_count; i++) {
        image_buffers_.draw_image[i].image = new unsigned char[frame_size * 4];
    }

    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return NULL;

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only

    // Create window with graphics context
    int main_window_width = gui_control_.gui_loc_main_window.size.cx;
    int main_window_height = gui_control_.gui_loc_main_window.size.cy;

    window_ = glfwCreateWindow(main_window_width, main_window_height, "Data Processing Library Exsample Gui", NULL, NULL);
    if (window_ == NULL)
        return NULL;

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1); // Enable vsync

    int main_window_xp = gui_control_.gui_loc_main_window.position.x;
    int main_window_yp = gui_control_.gui_loc_main_window.position.y;
    glfwSetWindowPos(window_, main_window_xp, main_window_yp);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    glGenTextures(2, texture_);

    glBindTexture(GL_TEXTURE_2D, texture_[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    glBindTexture(GL_TEXTURE_2D, texture_[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    return window_;
}

/**
 * GLFW Windowの破棄とリソースの解放.
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int TerminateWindow()
{
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window_);
    glfwTerminate();

    // delete buffers
    for (int i = 0; i < image_buffers_.buffer_image_count; i++) {
        delete[] image_buffers_.buffer_image[i].image;
        image_buffers_.buffer_image[i].image = nullptr;
    }
    for (int i = 0; i < image_buffers_.buffer_depth_count; i++) {
        delete[] image_buffers_.buffer_depth[i].depth;
        image_buffers_.buffer_depth[i].depth = nullptr;
    }
    for (int i = 0; i < image_buffers_.draw_image_count; i++) {
        delete[] image_buffers_.draw_image[i].image;
        image_buffers_.draw_image[i].image = nullptr;
    }

    return 0;
}

/**
 * GUIの内容を現在のカメラの状態で更新する.
 *
 * @param[in] is_update 現在のカメラのデータでパラメータを更新する
 * @param[in] image_state DPLControlを含むDPL制御用構造体
 *
 * @retval GLFWwindow* 成功
 * @retval NULL 失敗
 */
int InitializeWindowCameraControl(const bool is_update, ImageState* image_state)
{
    gui_control_;

    if (is_update) {
        ns_isc_dpl::IscDpl* iscdpl = image_state->dpl_control->GetDplObgkect();

        if (iscdpl != nullptr) {
            IscShutterMode mode_read = IscShutterMode::kManualShutter;
            int ret = iscdpl->DeviceGetOption(IscCameraParameter::kShutterMode, &mode_read);
            if (ret == 0) {
                switch (mode_read) {
                case IscShutterMode::kManualShutter:
                    gui_control_.shutter_mode = 0;
                    break;

                case IscShutterMode::kSingleShutter:
                    gui_control_.shutter_mode = 1;
                    break;

                case IscShutterMode::kDoubleShutter:
                    gui_control_.shutter_mode = 2;
                    break;
                }
            }
            else {
                printf("[ERROR]DeviceGetOption(kShutterMode) faild, ret=%d\n", ret);
            }

            gui_control_.shutter_mode_item_count = 0;

            bool is_enabled = iscdpl->DeviceOptionIsImplemented(IscCameraParameter::kManualShutter);
            if (is_enabled) {
                // 手動
                gui_control_.shutter_mode_item_count++;
            }

            is_enabled = iscdpl->DeviceOptionIsImplemented(IscCameraParameter::kSingleShutter);
            if (is_enabled) {
                // シングルシャッター
                gui_control_.shutter_mode_item_count++;
            }

            is_enabled = iscdpl->DeviceOptionIsImplemented(IscCameraParameter::kDoubleShutter);
            if (is_enabled) {
                // ダブルシャッター
                gui_control_.shutter_mode_item_count++;
            }

            is_enabled = iscdpl->DeviceOptionIsImplemented(IscCameraParameter::kDoubleShutter2);
            if (is_enabled) {
               // ダブルシャッター2
                gui_control_.shutter_mode_item_count++;
            }

        }

        /*

        // XC camera
        // The following values can be retrieved while the camera is acquiring.

        if (iscdpl != nullptr) {
            int read_value = 0;
            int ret = iscdpl->DeviceGetOption(IscCameraParameter::kExposure, &read_value);
            if (ret == 0) {
                gui_control_.exposure_value = read_value;
            }
            else {
                printf("[ERROR]DeviceGetOption(kExposure) faild, ret=%d\n", ret);
            }
        }

        if (iscdpl != nullptr) {
            int read_value = 0;
            int ret = iscdpl->DeviceGetOption(IscCameraParameter::kGain, &read_value);
            if (ret == 0) {
                gui_control_.gain_value = read_value;
            }
            else {
                printf("[ERROR]DeviceGetOption(kGain) faild, ret=%d\n", ret);
            }
        }
        */

        if (iscdpl != nullptr) {
            bool read_value = false;
            int ret = iscdpl->DeviceGetOption(IscCameraParameter::kAutoCalibration, &read_value);
            if (ret == 0) {
                gui_control_.auto_adjust = read_value;
            }
            else {
                printf("[ERROR]DeviceGetOption(auto_adjust) faild, ret=%d\n", ret);
            }
        }

        if (iscdpl != nullptr) {
            int min_value = 0;
            int ret = iscdpl->DeviceGetOptionMin(IscCameraParameter::kExposure, &min_value);

            int max_value = 0;
            ret = iscdpl->DeviceGetOptionMax(IscCameraParameter::kExposure, &max_value);

            gui_control_.exposure_value_component.min_value = min_value;
            gui_control_.exposure_value_component.max_value = max_value;
        }

        if (iscdpl != nullptr) {

            int min_value = 0;
            int ret = iscdpl->DeviceGetOptionMin(IscCameraParameter::kGain, &min_value);

            int max_value = 0;
            ret = iscdpl->DeviceGetOptionMax(IscCameraParameter::kGain, &max_value);
 
            gui_control_.gain_value_component.min_value = min_value;
            gui_control_.gain_value_component.max_value = max_value;
        }

    }

    return 0;
}

/**
 * GUIコンポーネントの更新と画像の描画（2D/3D)を行う.
 *
 * @param[in] image_state DPLControlを含むDPL制御用構造体
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int DrawWindow(ImageState* image_state)
{
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    glfwPollEvents();

    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // GUI controls
    GuiControls gui_control_previous = {};
    memcpy(&gui_control_previous, &gui_control_, sizeof(GuiControls));
    int ret = DrawControl(gui_control_, image_state);

    // camera control
    ret = ProcedureControl(gui_control_previous, gui_control_, dpl_control_start_mode_, image_state);

    // draw image
    if (gui_control_.is_grab_in_operation) {
        if (gui_control_.is_3d_viz) {
            // 3D
            ret = DrawPCLVizImage(gui_control_, dpl_control_start_mode_, image_state, &image_buffers_, &input_args_, &output_args_);
        
            // show pikc point information
            bool show_3d_pick_info = false;
            if (show_3d_pick_info) {
                float xp = (float)gui_control_.gui_loc_control.position.x;
                float yp = (float)gui_control_.gui_loc_control.position.y + (float)gui_control_.gui_loc_control.size.cy + 10.0F;
                ImGui::SetNextWindowPos(ImVec2(xp, yp), ImGuiCond_Once);
                ImGui::SetNextWindowSize(ImVec2(150, 120), ImGuiCond_Once);

                ImGui::Begin("3D pick inforamtion Window", &show_3d_pick_info);
                ImGui::Text("3D information");

                if (output_args_.pick_information.count > 0) {
                    if (output_args_.pick_information.pick_data[0].valid) {

                        ImGui::Text("Pick Point");
                        ImGui::Text("  X: %.03f", output_args_.pick_information.pick_data[0].x);
                        ImGui::Text("  Y: %.03f", output_args_.pick_information.pick_data[0].y);
                        ImGui::Text("  Z: %.03f", output_args_.pick_information.pick_data[0].z);
                    }
                }
                ImGui::End();
            }
        }
        else {
            // 2D
            ret = DrawDplImages(gui_control_, image_state, texture_, &image_buffers_);
        }
    }

#if 0
    // --- for debug ---
    bool show_demo_window = true;
    bool show_another_window = true;

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    // 2. Show mouse state
    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");

        ImGuiIO& io = ImGui::GetIO();
        if (ImGui::TreeNode("Mouse State"))
        {
            if (ImGui::IsMousePosValid())
                ImGui::Text("Mouse pos: (%g, %g)", io.MousePos.x, io.MousePos.y);
            else
                ImGui::Text("Mouse pos: <INVALID>");
            ImGui::Text("Mouse delta: (%g, %g)", io.MouseDelta.x, io.MouseDelta.y);

            int count = IM_ARRAYSIZE(io.MouseDown);
            ImGui::Text("Mouse down:");         for (int i = 0; i < count; i++) if (ImGui::IsMouseDown(i)) { ImGui::SameLine(); ImGui::Text("b%d (%.02f secs)", i, io.MouseDownDuration[i]); }
            ImGui::Text("Mouse clicked:");      for (int i = 0; i < count; i++) if (ImGui::IsMouseClicked(i)) { ImGui::SameLine(); ImGui::Text("b%d (%d)", i, ImGui::GetMouseClickedCount(i)); }
            ImGui::Text("Mouse released:");     for (int i = 0; i < count; i++) if (ImGui::IsMouseReleased(i)) { ImGui::SameLine(); ImGui::Text("b%d", i); }
            ImGui::Text("Mouse wheel: %.1f", io.MouseWheel);
            ImGui::Text("Pen Pressure: %.1f", io.PenPressure); // Note: currently unused
            ImGui::TreePop();
        }

        ImGui::End();
    }
    // -----------------
#endif

#if 0 
    // some code example
    // Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
    if (0) {
        static float f = 0.0f;
        static int counter = 0;

        ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
        ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
        ImGui::Checkbox("Another Window", &show_another_window);

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
        ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    // Show another simple window.
    if (show_another_window)
    {
        ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        if (ImGui::Button("Close Me"))
            show_another_window = false;
        ImGui::End();
    }
#endif

    // Rendering
    ImGui::Render();
    int display_w = 0, display_h = 0;
    glfwGetFramebufferSize(window_, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(clear_color_.x * clear_color_.w, clear_color_.y * clear_color_.w, clear_color_.z * clear_color_.w, clear_color_.w);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window_);

    return 0;
}

/**
 * GUIコンポーネントの更新.
 *
 * @param[in] gui_control GUIコンポーネントの状態
 * @param[in] image_state DPLControlを含むDPL制御用構造体
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int DrawControl(GuiControls& gui_control, ImageState* image_state)
{
    /*
        [Control]
        
        [Run Mode]
        3D      2d/3d       ON/OFF

        [Run]
        Grab	Start/Stop	ON/OFF
        Record	Start/Stop	ON/OFF
        Play	Start/Stop	ON/OFF

        [Camera Capture Mode]
        Grab Mode	Select One
            Disparity	
            Image Corrected
            Image not Corrected

        [Select Function]
        Stereo Matching     ON/OFF
        Disparity Filter    ON/OFF

        Color On/Off        ON/OFF

        [Camera Control]
        Shutter Mode	Select One
        Exposure	    Set the value
        Gain	        Set the value
        Auto Adjust	    ON/OFF

        [PCL Filter]
        Pass Through Filter     ON/OF
            min,max             Set the value(float)
        Down Sampling           ON/OF
            boxel size          Set the value(float)
        Radius Outlier Removal  ON/OF
            Radius Search       Set the value(float)
            Min Nighbors        Set the value(int)
        Plane Detection         ON/OF
            Threshold           Set the value(float)


    */

    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.0f, 0.7f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.0f, 0.3f, 0.1f, 1.0f));
    
    float xp = (float)gui_control.gui_loc_control.position.x;
    float yp = (float)gui_control.gui_loc_control.position.y;
    ImGui::SetNextWindowPos(ImVec2(xp, yp), ImGuiCond_Once);

    float xs = (float)gui_control.gui_loc_control.size.cx;
    float ys = (float)gui_control.gui_loc_control.size.cy;
    ImGui::SetNextWindowSize(ImVec2(xs, ys), ImGuiCond_Once);

    // --------------------------------------------
    ImGui::Begin("Control", nullptr, ImGuiWindowFlags_MenuBar);

    if (gui_control.enabled_viz_mode_3d) {
        ImGui::Text("Visualizer Mode");
        ImGui::Checkbox("3D", &gui_control.viz_mode_3d);
        ImGui::SameLine();
        ImGui::Checkbox("Based on Heat Map", &gui_control.viz_mode_3d_im_src_depth_heat);
        ImGui::SameLine();
        ImGui::Checkbox("Full Screen", &gui_control.viz_mode_3d_full_screen);
    }
    else {
        gui_control.viz_mode_3d = false;
        gui_control.viz_mode_3d_im_src_depth_heat = false;
    }

    ImGui::Text("Run");
    if (gui_control_.enable_camera) {
        ImGui::Checkbox("Grab", &gui_control.grab);
    }
    ImGui::Checkbox("Play", &gui_control.play);
    ImGui::Checkbox("Record", &gui_control.record);

    ImGui::Text("Select Function");
    if (gui_control.enable_data_processing_library) {
        ImGui::Checkbox("Stereo Matching", &gui_control.stereo_matching);
        ImGui::Checkbox("Disparity Filter", &gui_control.disparity_filter);
    }
    else {
        gui_control.stereo_matching = false;
        gui_control.disparity_filter = false;
    }
    ImGui::Checkbox("Color Image", &gui_control.color);

    if (gui_control.stereo_matching) {
        // Fixed to Corrected for the capture mode.
        gui_control.grab_mode = (int)GrabMode::Corrected;
    }
    else {
        ImGui::Text("Camera Capture Mode");
        ImGui::RadioButton("Disparity", &gui_control.grab_mode, (int)GrabMode::Disparity);
        ImGui::SameLine();
        ImGui::RadioButton("Corrected", &gui_control.grab_mode, (int)GrabMode::Corrected);
        ImGui::SameLine();
        ImGui::RadioButton("Not Corrected", &gui_control.grab_mode, (int)GrabMode::NotCorrected);
    }

    if (ImGui::TreeNode("Camera Control")) {
        //ImGui::Text("Camera Control");

        const char* items[] = { "Manual", "Single", "Double1", "Double2"};
        //ImGui::Combo("Shutter Mode", &gui_control.shutter_mode, items, IM_ARRAYSIZE(items));
        if (gui_control.shutter_mode_item_count > IM_ARRAYSIZE(items)) {
            gui_control.shutter_mode_item_count = IM_ARRAYSIZE(items);
        }
        ImGui::Combo("Shutter Mode", &gui_control.shutter_mode, items, gui_control.shutter_mode_item_count);

        int min_value = gui_control.exposure_value_component.min_value;
        int max_value = gui_control.exposure_value_component.max_value;
        ImGui::SliderInt("Exposure Value", &gui_control.exposure_value, min_value, max_value);
 
        min_value = gui_control.gain_value_component.min_value;
        max_value = gui_control.gain_value_component.max_value;
        ImGui::SliderInt("Gain Value", &gui_control.gain_value, min_value, max_value);

        ImGui::Checkbox("Auto Adjust", &gui_control.auto_adjust);

        ImGui::TreePop();
    }

    if (gui_control.enabled_viz_mode_3d) {
        if (ImGui::TreeNode("PCL Filter")) {
            //ImGui::Text("PCL Filter");

            ImGui::Checkbox("Pass Through Filter", &gui_control.pcl_filter_parameter.enabled_pass_through_filter);
            if (gui_control.pcl_filter_parameter.enabled_pass_through_filter) {

                float min_distance = (float)GetDrawMinDistance(image_state);
                float max_distance = (float)GetDrawMaxDistance(image_state);

                ImGui::SliderFloat("Min(m)", &gui_control.pcl_filter_parameter.pass_through_filter_range.min, min_distance, max_distance);
                ImGui::SliderFloat("Max(m)", &gui_control.pcl_filter_parameter.pass_through_filter_range.max, min_distance, max_distance);
            }

            ImGui::Checkbox("Down Sampling", &gui_control.pcl_filter_parameter.enabled_down_sampling);
            if (gui_control.pcl_filter_parameter.enabled_down_sampling) {
                ImGui::SliderFloat("Boxel Size(m)", &gui_control.pcl_filter_parameter.down_sampling_boxel_size, 0.01f, 0.1f);
            }

            ImGui::Checkbox("Radius Outlier Removal", &gui_control.pcl_filter_parameter.enabled_radius_outlier_removal);
            if (gui_control.pcl_filter_parameter.enabled_radius_outlier_removal) {
                float temp_value = (float)gui_control.pcl_filter_parameter.radius_outlier_removal_param.radius_search;
                ImGui::SliderFloat("Radius (m)", &temp_value, 0.01f, 0.5f);
                gui_control.pcl_filter_parameter.radius_outlier_removal_param.radius_search = temp_value;
                ImGui::SliderInt("Min Neighbors", &gui_control.pcl_filter_parameter.radius_outlier_removal_param.min_neighbors, 2, 1000);
            }

            ImGui::Checkbox("Plane Detection", &gui_control.pcl_filter_parameter.enabled_plane_detection);
            if (gui_control.pcl_filter_parameter.enabled_plane_detection) {
                float temp_value = (float)gui_control.pcl_filter_parameter.plane_detection_threshold;
                ImGui::SliderFloat("Threshold", &temp_value, 0.1f, 0.9f);
                gui_control.pcl_filter_parameter.plane_detection_threshold = temp_value;
            }

            ImGui::TreePop();
        }
    }

    // --------------------------------------------
    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleColor();

    return 0;
}

/**
 * GUIコンポーネントの扱いのサンプルコード(実際に使用されていません)
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int DrawControl_Sample()
{
#if 0
    ImGui::Begin("Test Window");

    ImGui::Text("Hello, world %d", 123);

    if (ImGui::Button("OK")) {
        printf("Button\n");
    }

    static char buf[256] = "aaa";
    if (ImGui::InputText("string", buf, 256)) {
        printf("InputText\n");
    }

    static float f = 0.0f;
    if (ImGui::SliderFloat("float", &f, 0.0f, 1.0f)) {
        printf("SliderFloat\n");
    }
    ImGui::End();
#endif


    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.0f, 0.7f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.0f, 0.3f, 0.1f, 1.0f));
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(200, 300), ImGuiCond_Once);

    ImGui::Begin("config 1", nullptr, ImGuiWindowFlags_MenuBar);
    // --------------------------------------------

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Save")) {

            }
            if (ImGui::MenuItem("Load")) {

            }

            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    static float slider1 = 0.0;
    static char text1[64] = "";

    if (ImGui::TreeNode("group 1")) {

        ImGui::Text("fps: %.2f", ImGui::GetIO().Framerate);
        ImGui::SliderFloat("slider 1", &slider1, 0.0f, 1.0f);
        ImGui::InputText("textbox 1", text1, sizeof(text1));
        if (ImGui::Button("button 1")) {
            slider1 = 0.0f;
            strcpy_s(text1, "button 1");
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNode("group 2")) {

        ImGui::Text("fps: %.2f", ImGui::GetIO().Framerate);
        ImGui::SliderFloat("slider 1b", &slider1, 0.0f, 1.0f);
        ImGui::InputText("textbox 1b", text1, sizeof(text1));
        if (ImGui::Button("button 1b")) {
            slider1 = 0.0f;
            strcpy_s(text1, "button 1b");
        }

        if (ImGui::TreeNode("group 3")) {
            if (ImGui::Button("button 2")) {

            }
            ImGui::TreePop();

        }

        ImGui::TreePop();
    }

    static std::vector<float> items(10);

    if (ImGui::Button("add")) {
        items.push_back(0.0f);
    }
    if (ImGui::Button("remove")) {
        if (items.empty() == false) {
            items.pop_back();
        }
    }

    ImGui::BeginChild(ImGui::GetID((void*)0), ImVec2(250, 100), ImGuiWindowFlags_NoTitleBar);
    for (int i = 0; i < items.size(); ++i) {
        char name[32] = {};
        sprintf_s(name, "item %d", i);
        ImGui::SliderFloat(name, &items[i], 0.0f, 10.0f);
    }
    ImGui::EndChild();

    static bool isEnable = false;
    static int mode = 0;

    enum {
        MODE_1,
        MODE_2,
    };

    ImGui::Checkbox("isEnable", &isEnable);

    if (isEnable) {

        ImGui::RadioButton("mode 1", &mode, MODE_1); 
        ImGui::SameLine(); 
        ImGui::RadioButton("mode 2", &mode, MODE_2);

        if (ImGui::TreeNode("inspector")) {
            if (mode == MODE_1) {
                ImGui::Text("Mode 1 Contents");
            }
            else if (mode == MODE_2) {
                ImGui::Text("Mode 2 Contents");
            }

            ImGui::TreePop();
        }
    }

    // --------------------------------------------
    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleColor();

    return 0;
}

/**
 * 取り込み開始.
 *
 * @param[in] gui_control_latest GUIコンポーネントの最新の状態
 * @param[out] dpl_control_start_mode_latest DPL開始設定
 * @param[in] image_state DPLControlを含むDPL制御用構造体
 * @param[out] is_update_camera_data_request カメラの状態でGUIを更新する要求
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int StartGrabProcedure(GuiControls& gui_control_latest, DplControl::StartMode& dpl_control_start_mode_latest, ImageState* image_state, bool& is_update_camera_data_request)
{

    // function
    if (gui_control_latest.stereo_matching) {
        // fix to disparity
        gui_control_latest.grab_mode = (int)GrabMode::Corrected;
        dpl_control_start_mode_latest.grab_mode = (int)GrabMode::Corrected;

        dpl_control_start_mode_latest.enabled_stereo_matching = true;
        dpl_control_start_mode_latest.enabled_disparity_filter = gui_control_latest.disparity_filter;
    }
    else {
        dpl_control_start_mode_latest.enabled_stereo_matching = false;

        switch (gui_control_latest.grab_mode) {
        case 0:
            // diparity
            dpl_control_start_mode_latest.grab_mode = (int)GrabMode::Disparity;
            dpl_control_start_mode_latest.enabled_disparity_filter = gui_control_latest.disparity_filter;
            break;

        case 1:
            // correct image
            dpl_control_start_mode_latest.grab_mode = (int)GrabMode::Corrected;
            dpl_control_start_mode_latest.enabled_disparity_filter = false;
            break;

        case 2:
            // before correct image
            dpl_control_start_mode_latest.grab_mode = (int)GrabMode::NotCorrected;
            dpl_control_start_mode_latest.enabled_disparity_filter = false;
            break;

        default:
            // diparity
            dpl_control_start_mode_latest.grab_mode = (int)GrabMode::Disparity;
            dpl_control_start_mode_latest.enabled_disparity_filter = gui_control_latest.disparity_filter;
            break;
        }
    }

    // check color
    if (gui_control_latest.color) {
        dpl_control_start_mode_latest.enabled_color = true;
        image_state->color_mode = 1;
    }
    else {
        dpl_control_start_mode_latest.enabled_color = false;
        image_state->color_mode = 0;
    }

    // check record
    if (gui_control_latest.record) {
        dpl_control_start_mode_latest.grab_record_mode = true;
    }
    else {
        dpl_control_start_mode_latest.grab_record_mode = false;
    }

    // check play
    if (gui_control_latest.play) {
        dpl_control_start_mode_latest.grab_play_mode = true;
        swprintf_s(dpl_control_start_mode_latest.play_file_name, L"%s", gui_control_latest.play_file_name);
    }
    else {
        dpl_control_start_mode_latest.grab_play_mode = false;
        swprintf_s(dpl_control_start_mode_latest.play_file_name, L"c:\\temp\\dummy.dat");
    }

    bool is_header_valided = true;
    if (dpl_control_start_mode_latest.grab_play_mode) {
        // Get file information and apply it
        IscRawFileHeader raw_file_headaer = {};
        int ret = GetPlayFileInformation(image_state, dpl_control_start_mode_latest.play_file_name, &raw_file_headaer);

        // Update camera-specific parameters
        image_state->b = raw_file_headaer.base_length;
        image_state->bf = raw_file_headaer.bf;
        image_state->dinf = raw_file_headaer.d_inf;

        switch (raw_file_headaer.grab_mode) {
        case(1):
            // IscGrabMode::kParallax:
            dpl_control_start_mode_latest.grab_mode = (int)GrabMode::Disparity;
            break;

        case(2):
            // IscGrabMode::kCorrect:
            dpl_control_start_mode_latest.grab_mode = (int)GrabMode::Corrected;
            break;

        case(3):
            // IscGrabMode::kBeforeCorrect:
            dpl_control_start_mode_latest.grab_mode = (int)GrabMode::NotCorrected;
            break;

        case(4):
            // IscGrabMode::kBayerBase:
            is_header_valided = false;
            break;

        case(5):
            //  IscGrabMode::kBayerCompare:
            is_header_valided = false;
            break;
        default:
            is_header_valided = false;
            break;
        }

        if (raw_file_headaer.color_mode == 0) {
            dpl_control_start_mode_latest.enabled_color = false;
            image_state->color_mode = 0;
        }
        else if (raw_file_headaer.color_mode == 1) {
            dpl_control_start_mode_latest.enabled_color = true;
            image_state->color_mode = 1;
        }
        else {
            is_header_valided = false;
        }

        if (!is_header_valided) {
            dpl_control_start_mode_latest.grab_play_mode = false;

            printf("[ERROR]Cannot start because the file is incorrect!!\n");
            return 0;
        }

        // feed back to gui
        switch (raw_file_headaer.grab_mode) {
        case(1):
            // IscGrabMode::kParallax:
            gui_control_latest.grab_mode = (int)GrabMode::Disparity;
            break;

        case(2):
            // IscGrabMode::kCorrect:
            gui_control_latest.grab_mode = (int)GrabMode::Corrected;
            break;

        case(3):
            // IscGrabMode::kBeforeCorrect:
            gui_control_latest.grab_mode = (int)GrabMode::NotCorrected;
            break;

        case(4):
            // IscGrabMode::kBayerBase:
            break;

        case(5):
            //  IscGrabMode::kBayerCompare:
            break;
        default:
            break;
        }
        
        if (raw_file_headaer.color_mode == 0) {
            gui_control_latest.color = false;
        }
        else if (raw_file_headaer.color_mode == 1) {
            gui_control_latest.color = true;
        }
    }
    else {
        // Update camera parameters
        if (gui_control_latest.enable_camera) {
            int ret = image_state->dpl_control->GetCameraParameter(&(image_state->b), &(image_state->bf), &(image_state->dinf), &(image_state->width), &(image_state->height));
        }
    }

    // start pcl viz
    if (gui_control_latest.viz_mode_3d) {

        // mode error check
        {
            bool is_ok = false;
            if (gui_control_latest.stereo_matching) {
                is_ok = true;
            }
            else if (dpl_control_start_mode_latest.grab_mode == (int)GrabMode::Disparity) {
                is_ok = true;
            }
            if (!is_ok) {
                printf("[ERROR]Mode Error! You have selected the 3D view, but parallax capture is not specified!!\n");
                printf("[ERROR]Please set it up correctly and then restart!!!\n");
            }
        }

        int ret = StartPclViz();
        if (ret == 0) {
            gui_control_latest.is_3d_viz = true;
        }
        else {
            gui_control_latest.is_3d_viz = false;
        }
    }
    else {
        gui_control_latest.is_3d_viz = false;
    }

    // start grab/play/record
    int ret = DplStart(dpl_control_start_mode_latest, image_state);
    if (ret == 0) {
        is_update_camera_data_request = true;
        if (dpl_control_start_mode_latest.grab_play_mode) {
            is_update_camera_data_request = false;
        }
        gui_control_.is_grab_in_operation = true;
    }
    else {
        printf("[ERROR]Failed to start capturing\n");
        is_update_camera_data_request = false;
        gui_control_.is_grab_in_operation = false;
    }

    if (gui_control_latest.is_3d_viz) {
        int cx = gui_control_.gui_loc_control.size.cx + 20;
        int main_window_height = gui_control_.gui_loc_main_window.size.cy;
        glfwSetWindowSize(window_, cx, main_window_height);
    }
    else {
        int main_window_width = gui_control_.gui_loc_main_window.size.cx;
        int main_window_height = gui_control_.gui_loc_main_window.size.cy;
        glfwSetWindowSize(window_, main_window_width, main_window_height);
    }

    return 0;
}

/**
 * 取り込み停止.
 *
 * @param[in] gui_control_latest GUIコンポーネントの最新の状態
 * @param[in] dpl_control_start_mode_latest DPL開始設定
 * @param[in] image_state DPLControlを含むDPL制御用構造体
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int StopGrabProcedure(GuiControls& gui_control_latest, DplControl::StartMode& dpl_control_start_mode_latest, ImageState* image_state)
{
    // stop
    int ret = DplStop(image_state);
    gui_control_latest.is_grab_in_operation = false;

    if (gui_control_latest.is_3d_viz) {
        int ret = StopPclViz();
        gui_control_latest.is_3d_viz = false;
    }

    return 0;
}

/**
 * GUIコンポーネントの変化（要求）を処理し、動作を決定する.
 *
 * @param[in] gui_control_previous GUIコンポーネントの前回の状態
 * @param[in] gui_control_latest GUIコンポーネントの最新の状態
 * @param[in] dpl_control_start_mode_latest DPL開始設定
 * @param[in] image_state DPLControlを含むDPL制御用構造体
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int ProcedureControl(GuiControls& gui_control_previous, GuiControls& gui_control_latest, DplControl::StartMode& dpl_control_start_mode_latest, ImageState* image_state)
{

    bool is_update_camera_data_request = false;

    // check start
    if (gui_control_latest.grab != gui_control_previous.grab) {
        if (gui_control_latest.grab) {
            // start
            int ret = StartGrabProcedure(gui_control_latest, dpl_control_start_mode_latest, image_state, is_update_camera_data_request);

            gui_control_latest.post_camera_status_request = is_update_camera_data_request;
            
            Sleep(0);
        }
        else {
            // stop
            int ret = StopGrabProcedure(gui_control_latest, dpl_control_start_mode_latest, image_state);
        }
    }

    // play
    if (gui_control_latest.play != gui_control_previous.play) {
        if (gui_control_latest.play) {
            // start play
            wchar_t open_folder_name[_MAX_PATH] = {};
            wchar_t open_file_name[_MAX_PATH] = {};

            GetDataRecordPath(image_state, open_folder_name, _MAX_PATH);

            int ret = WsOpenFileDialog(open_folder_name, open_file_name);
            if (ret == 0) {
                swprintf_s(gui_control_latest.play_file_name, L"%s", open_file_name);
                int ret = StartGrabProcedure(gui_control_latest, dpl_control_start_mode_latest, image_state, is_update_camera_data_request);
            
                gui_control_latest.post_camera_status_request = is_update_camera_data_request;
            }
            else {
                gui_control_latest.play = false;
            }
        }
        else {
            // stop play
            int ret = StopGrabProcedure(gui_control_latest, dpl_control_start_mode_latest, image_state);
        }
    }

    // record
    if (gui_control_latest.record != gui_control_previous.record) {
        if (gui_control_latest.record) {
            // start record
            int ret = StartGrabProcedure(gui_control_latest, dpl_control_start_mode_latest, image_state, is_update_camera_data_request);

            gui_control_latest.post_camera_status_request = is_update_camera_data_request;
        }
        else {
            // stop record
            int ret = StopGrabProcedure(gui_control_latest, dpl_control_start_mode_latest, image_state);
        }
    }

    // camera
    if (gui_control_latest.update_camera_status_request) {
        gui_control_latest.update_camera_status_request = false;

        ns_isc_dpl::IscDpl* iscdpl = image_state->dpl_control->GetDplObgkect();

        if (iscdpl != nullptr) {
            IscShutterMode mode_read = IscShutterMode::kManualShutter;
            int ret = iscdpl->DeviceGetOption(IscCameraParameter::kShutterMode, &mode_read);
            if (ret == 0) {
                switch (mode_read) {
                case IscShutterMode::kManualShutter:
                    gui_control_latest.shutter_mode = 0;
                    break;

                case IscShutterMode::kSingleShutter:
                    gui_control_latest.shutter_mode = 1;
                    break;

                case IscShutterMode::kDoubleShutter:
                    gui_control_latest.shutter_mode = 2;
                    break;
                }
            }
            else {
                printf("[ERROR]DeviceGetOption(kShutterMode) faild, ret=%d\n", ret);
            }
        }

        if (iscdpl != nullptr) {
            int read_value = 0;
            int ret = iscdpl->DeviceGetOption(IscCameraParameter::kExposure, &read_value);
            if (ret == 0) {
                gui_control_latest.exposure_value = read_value;
            }
            else {
                printf("[ERROR]DeviceGetOption(kExposure) faild, ret=%d\n", ret);
            }
        }

        if (iscdpl != nullptr) {
            int read_value = 0;
            int ret = iscdpl->DeviceGetOption(IscCameraParameter::kGain, &read_value);
            if (ret == 0) {
                gui_control_latest.gain_value = read_value;
            }
            else {
                printf("[ERROR]DeviceGetOption(kGain) faild, ret=%d\n", ret);
            }
        }

        if (iscdpl != nullptr) {
            bool read_value = false;
            int ret = iscdpl->DeviceGetOption(IscCameraParameter::kAutoCalibration, &read_value);
            if (ret == 0) {
                gui_control_latest.auto_adjust = read_value;
            }
            else {
                printf("[ERROR]DeviceGetOption(auto_adjust) faild, ret=%d\n", ret);
            }
        }
    }


    if (gui_control_latest.shutter_mode != gui_control_previous.shutter_mode) {
        
        IscShutterMode mode = IscShutterMode::kManualShutter;
        switch (gui_control_latest.shutter_mode) {
        case 0:
            mode = IscShutterMode::kManualShutter;
            break;

        case 1:
            mode = IscShutterMode::kSingleShutter;
            break;

        case 2:
            mode = IscShutterMode::kDoubleShutter;
            break;
        }

        ns_isc_dpl::IscDpl* iscdpl = image_state->dpl_control->GetDplObgkect();
        if (iscdpl != nullptr) {
            int ret = iscdpl->DeviceSetOption(IscCameraParameter::kShutterMode, mode);

            IscShutterMode mode_read = IscShutterMode::kManualShutter;
            ret = iscdpl->DeviceGetOption(IscCameraParameter::kShutterMode, &mode_read);
            if (ret == 0) {
                switch (mode_read) {
                case IscShutterMode::kManualShutter:
                    gui_control_latest.shutter_mode = 0;
                    break;

                case IscShutterMode::kSingleShutter:
                    gui_control_latest.shutter_mode = 1;
                    break;

                case IscShutterMode::kDoubleShutter:
                    gui_control_latest.shutter_mode = 2;
                    break;
                }
            }
        }
    }

    if (gui_control_latest.exposure_value != gui_control_previous.exposure_value) {

        ns_isc_dpl::IscDpl* iscdpl = image_state->dpl_control->GetDplObgkect();
        if (iscdpl != nullptr) {
            int ret = iscdpl->DeviceSetOption(IscCameraParameter::kExposure, gui_control_latest.exposure_value);
            int read_value = 0;
            ret = iscdpl->DeviceGetOption(IscCameraParameter::kExposure, &read_value);
            if (ret == 0) {
                gui_control_latest.exposure_value = read_value;
            }
        }
    }

    if (gui_control_latest.gain_value != gui_control_previous.gain_value) {

        ns_isc_dpl::IscDpl* iscdpl = image_state->dpl_control->GetDplObgkect();
        if (iscdpl != nullptr) {
            int ret = iscdpl->DeviceSetOption(IscCameraParameter::kGain, gui_control_latest.gain_value);
            int read_value = 0;
            ret = iscdpl->DeviceGetOption(IscCameraParameter::kGain, &read_value);
            if (ret == 0) {
                gui_control_latest.gain_value = read_value;
            }
        }
    }

    if (gui_control_latest.auto_adjust != gui_control_previous.auto_adjust) {

        ns_isc_dpl::IscDpl* iscdpl = image_state->dpl_control->GetDplObgkect();
        if (iscdpl != nullptr) {
            int ret = iscdpl->DeviceSetOption(IscCameraParameter::kAutoCalibration, gui_control_latest.auto_adjust);
            bool read_value = false;
            ret = iscdpl->DeviceGetOption(IscCameraParameter::kAutoCalibration, &read_value);
            if (ret == 0) {
                gui_control_latest.auto_adjust = read_value;
            }
        }
    }

    if (gui_control_latest.viz_mode_3d_full_screen != gui_control_previous.viz_mode_3d_full_screen) {
        if (gui_control_latest.viz_mode_3d_full_screen) {
            // full screen on
            gui_control_latest.viz_mode_3d_full_screen_req = true;
        }
        else {
            // full screen off
            gui_control_latest.viz_mode_3d_restore_screen_req = true;
        }
    }

    return 0;
}

/**
 * 画像の表示倍率を決定する.
 *
 * @param[in] max_width 表示先の幅
 * @param[in] image_width 画像の幅
 *
 * @retval 倍率
 */
double GetResizeRatio(const int max_width, const int image_width)
{
    double ratio = 1.0;
    if (max_width >= image_width) {
        return 1.0;
    }
    else {
        ratio = (double)max_width / (double)image_width;
    }
    
    return ratio;
}

/**
 * ImGuiを使用して画像を表示する.
 *
 * @param[in] gui_control_latest GUIコンポーネントの最新の状態
 * @param[in] image_state DPLControlを含むDPL制御用構造体
 * @param[in] texture textre buffer
 * @param[in] image_buffers 作業用Buffer
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int DrawDplImages(GuiControls& gui_control_latest, ImageState* image_state, GLuint* texture, ImageDataBuffers* image_buffers)
{
    int mode = 0;
    bool is_show = true;

    if (gui_control_latest.stereo_matching || gui_control_latest.disparity_filter) {
        mode = 0;
    }
    else {
        mode = 1;
    }

    if (mode == 0) {
        // images from camera
        bool camera_status = image_state->dpl_control->GetCameraData(&image_state->isc_image_Info);
        const int fd_inex = kISCIMAGEINFO_FRAMEDATA_LATEST;

        if ((image_state->isc_image_Info.frame_data[fd_inex].p1.width == 0) ||
            (image_state->isc_image_Info.frame_data[fd_inex].p1.height == 0)) {

            camera_status = false;
        }

        if (camera_status) {
            // Process post-processing requests
            if (gui_control_latest.post_camera_status_request) {
                gui_control_latest.post_camera_status_request = false;
                gui_control_latest.update_camera_status_request = true;
            }

            // Do you have a Color image?
            bool is_color_exists = false;
            if (image_state->color_mode == 1) {
                if ((image_state->isc_image_Info.frame_data[fd_inex].color.width != 0) &&
                    (image_state->isc_image_Info.frame_data[fd_inex].color.height != 0)) {

                    is_color_exists = true;
                }
            }

            if (is_color_exists) {
                // color image
                cv::Mat mat_base_image(image_state->isc_image_Info.frame_data[fd_inex].color.height, image_state->isc_image_Info.frame_data[fd_inex].color.width, CV_8UC3, image_state->isc_image_Info.frame_data[fd_inex].color.image);

                double ratio = GetResizeRatio(gui_control_latest.gui_loc_images[0].size.cx, image_state->isc_image_Info.frame_data[fd_inex].color.width);

                image_buffers->buffer_image[0].width = image_state->isc_image_Info.frame_data[fd_inex].color.width;
                image_buffers->buffer_image[0].height = image_state->isc_image_Info.frame_data[fd_inex].color.height;
                image_buffers->buffer_image[0].channel_count = 3;
                cv::Mat mat_base_image_scale(image_buffers->buffer_image[0].height, image_buffers->buffer_image[0].width, CV_8UC3, image_buffers->buffer_image[0].image);
                cv::resize(mat_base_image, mat_base_image_scale, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

                image_buffers->buffer_image[1].width = mat_base_image_scale.cols;
                image_buffers->buffer_image[1].height = mat_base_image_scale.rows;
                image_buffers->buffer_image[1].channel_count = 3;
                cv::Mat mat_base_image_scale_flip_temp(image_buffers->buffer_image[1].height, image_buffers->buffer_image[1].width, CV_8UC3, image_buffers->buffer_image[1].image);
                cv::flip(mat_base_image_scale, mat_base_image_scale_flip_temp, -1);

                image_buffers->draw_image[0].width = mat_base_image_scale_flip_temp.cols;
                image_buffers->draw_image[0].height = mat_base_image_scale_flip_temp.rows;
                image_buffers->draw_image[0].channel_count = 4;
                cv::Mat mat_base_image_scale_flip(image_buffers->draw_image[0].height, image_buffers->draw_image[0].width, CV_8UC4, image_buffers->draw_image[0].image);
                cv::cvtColor(mat_base_image_scale_flip_temp, mat_base_image_scale_flip, cv::COLOR_BGR2RGBA);
            }
            else {
                // base image
                cv::Mat mat_base_image(image_state->isc_image_Info.frame_data[fd_inex].p1.height, image_state->isc_image_Info.frame_data[fd_inex].p1.width, CV_8UC1, image_state->isc_image_Info.frame_data[fd_inex].p1.image);

                double ratio = GetResizeRatio(gui_control_latest.gui_loc_images[0].size.cx, image_state->isc_image_Info.frame_data[fd_inex].p1.width);

                image_buffers->buffer_image[0].width = image_state->isc_image_Info.frame_data[fd_inex].p1.width;
                image_buffers->buffer_image[0].height = image_state->isc_image_Info.frame_data[fd_inex].p1.height;
                image_buffers->buffer_image[0].channel_count = 1;
                cv::Mat mat_base_image_scale(image_buffers->buffer_image[0].height, image_buffers->buffer_image[0].width, CV_8UC1, image_buffers->buffer_image[0].image);
                cv::resize(mat_base_image, mat_base_image_scale, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

                image_buffers->buffer_image[1].width = mat_base_image_scale.cols;
                image_buffers->buffer_image[1].height = mat_base_image_scale.rows;
                image_buffers->buffer_image[1].channel_count = 1;
                cv::Mat mat_base_image_scale_flip_temp(image_buffers->buffer_image[1].height, image_buffers->buffer_image[1].width, CV_8UC1, image_buffers->buffer_image[1].image);
                cv::flip(mat_base_image_scale, mat_base_image_scale_flip_temp, -1);

                image_buffers->draw_image[0].width = mat_base_image_scale_flip_temp.cols;
                image_buffers->draw_image[0].height = mat_base_image_scale_flip_temp.rows;
                image_buffers->draw_image[0].channel_count = 4;
                cv::Mat mat_base_image_scale_flip(image_buffers->draw_image[0].height, image_buffers->draw_image[0].width, CV_8UC4, image_buffers->draw_image[0].image);
                cv::cvtColor(mat_base_image_scale_flip_temp, mat_base_image_scale_flip, cv::COLOR_BGR2RGBA);

            }
        }

        // data processing result
        bool data_proc_status = image_state->dpl_control->GetDataProcessingData(&image_state->isc_data_proc_result_data);

        if ((image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].depth.width == 0) ||
            (image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].depth.height == 0)) {

            data_proc_status = false;
        }

        if (data_proc_status) {
            // depth
            const int width = image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].depth.width;
            const int height = image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].depth.height;
            float* depth = image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].depth.image;

            {
                const double min_distance = gui_control_latest.pcl_filter_parameter.pass_through_filter_range.min;
                const double max_distance = gui_control_latest.pcl_filter_parameter.pass_through_filter_range.max;

                double min_distance_temp = 0.0f;
                double max_distance_temp = 0.0f;
                image_state->dpl_control->GetMinMaxDistance(&min_distance_temp, &max_distance_temp);

                if ((min_distance != min_distance_temp) || (max_distance != max_distance_temp)) {
                    image_state->dpl_control->RebuildDrawColorMap(min_distance, max_distance);
                }
            }

            image_state->dpl_control->ConvertDisparityToImage(  image_state->b, image_state->angle, image_state->bf, image_state->dinf,
                                                                width, height, depth, image_state->bgra_image);

            cv::Mat mat_depth_image_temp(height, width, CV_8UC4, image_state->bgra_image);

            image_buffers->buffer_image[2].width = mat_depth_image_temp.cols;
            image_buffers->buffer_image[2].height = mat_depth_image_temp.rows;
            image_buffers->buffer_image[2].channel_count = 4;
            cv::Mat mat_depth_image(image_buffers->buffer_image[2].height, image_buffers->buffer_image[2].width, CV_8UC4, image_buffers->buffer_image[2].image);
            cv::cvtColor(mat_depth_image_temp, mat_depth_image, cv::COLOR_BGRA2RGBA);   // cv::COLOR_BGR2RGBA);

            double ratio = GetResizeRatio(gui_control_latest.gui_loc_images[1].size.cx, mat_depth_image_temp.cols);

            image_buffers->buffer_image[3].width = mat_depth_image.cols;
            image_buffers->buffer_image[3].height = mat_depth_image.rows;
            image_buffers->buffer_image[3].channel_count = 4;
            cv::Mat mat_depth_image_scale(image_buffers->buffer_image[3].height, image_buffers->buffer_image[3].width, CV_8UC4, image_buffers->buffer_image[3].image);
            cv::resize(mat_depth_image, mat_depth_image_scale, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

            image_buffers->draw_image[1].width = mat_depth_image_scale.cols;
            image_buffers->draw_image[1].height = mat_depth_image_scale.rows;
            image_buffers->draw_image[1].channel_count = 4;
            cv::Mat mat_depth_image_scale_flip(image_buffers->draw_image[1].height, image_buffers->draw_image[1].width, CV_8UC4, image_buffers->draw_image[1].image);
            cv::flip(mat_depth_image_scale, mat_depth_image_scale_flip, -1);
        }

        if ((image_buffers->draw_image[0].width == 0) || (image_buffers->draw_image[0].height == 0)) {
            Sleep(16);
        }
        else {
            ImGui::SetNextWindowSize(ImVec2((float)image_buffers->draw_image[0].width, (float)image_buffers->draw_image[0].height), ImGuiCond_Once);

            float xp = (float)gui_control_latest.gui_loc_images[0].position.x;
            float yp = (float)gui_control_latest.gui_loc_images[0].position.y;
            ImGui::SetNextWindowPos(ImVec2(xp, yp), ImGuiCond_Once);

            ImGui::Begin("imgui image", &is_show);
            //ImGui::Text("This is base image.");

            glBindTexture(GL_TEXTURE_2D, texture[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_buffers->draw_image[0].width, image_buffers->draw_image[0].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_buffers->draw_image[0].image);
            ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(texture[0])), ImVec2((float)image_buffers->draw_image[0].width, (float)image_buffers->draw_image[0].height));
            //ImGui::End();

            // Yellow is content region min/max
            if (false) {
                ImVec2 vMin = ImGui::GetWindowContentRegionMin();
                ImVec2 vMax = ImGui::GetWindowContentRegionMax();

                vMin.x += ImGui::GetWindowPos().x;
                vMin.y += ImGui::GetWindowPos().y;
                vMax.x += ImGui::GetWindowPos().x;
                vMax.y += ImGui::GetWindowPos().y;

                ImGui::GetForegroundDrawList()->AddRect(vMin, vMax, IM_COL32(255, 255, 0, 255));
            }
            ImGui::End();
        }

        if ((image_buffers->draw_image[1].width == 0) || (image_buffers->draw_image[1].height == 0)) {
            Sleep(16);
        }
        else {
            ImGui::SetNextWindowSize(ImVec2((float)image_buffers->draw_image[1].width, (float)image_buffers->draw_image[1].height), ImGuiCond_Once);
            float xp = (float)gui_control_latest.gui_loc_images[1].position.x;
            float yp = (float)gui_control_latest.gui_loc_images[1].position.y;
            ImGui::SetNextWindowPos(ImVec2(xp, yp), ImGuiCond_Once);

            ImGui::Begin("imgui depth", &is_show);
            //ImGui::Text("This is depth image.");

            glBindTexture(GL_TEXTURE_2D, texture[1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_buffers->draw_image[1].width, image_buffers->draw_image[1].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_buffers->draw_image[1].image);
            ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(texture[1])), ImVec2((float)image_buffers->draw_image[1].width, (float)image_buffers->draw_image[1].height));
            ImGui::End();
        }
    }
    else if (mode == 1) {
        // images from camera
        bool camera_status = image_state->dpl_control->GetCameraData(&image_state->isc_image_Info);
        const int fd_inex = kISCIMAGEINFO_FRAMEDATA_LATEST;

        if ((image_state->isc_image_Info.frame_data[fd_inex].p1.width == 0) ||
            (image_state->isc_image_Info.frame_data[fd_inex].p1.height == 0)) {

            camera_status = false;
        }

        if (camera_status) {
            // Process post-processing requests
            if (gui_control_latest.post_camera_status_request) {
                gui_control_latest.post_camera_status_request = false;
                gui_control_latest.update_camera_status_request = true;
            }

            // Do you have a Color image?
            bool is_color_exists = false;
            if (image_state->color_mode == 1) {
                if ((image_state->isc_image_Info.frame_data[fd_inex].color.width != 0) &&
                    (image_state->isc_image_Info.frame_data[fd_inex].color.height != 0)) {

                    is_color_exists = true;
                }
            }

            if (is_color_exists) {
                // color image
                cv::Mat mat_base_image(image_state->isc_image_Info.frame_data[fd_inex].color.height, image_state->isc_image_Info.frame_data[fd_inex].color.width, CV_8UC3, image_state->isc_image_Info.frame_data[fd_inex].color.image);

                double ratio = GetResizeRatio(gui_control_latest.gui_loc_images[0].size.cx, image_state->isc_image_Info.frame_data[fd_inex].color.width);

                image_buffers->buffer_image[0].width = image_state->isc_image_Info.frame_data[fd_inex].color.width;
                image_buffers->buffer_image[0].height = image_state->isc_image_Info.frame_data[fd_inex].color.height;
                image_buffers->buffer_image[0].channel_count = 3;
                cv::Mat mat_base_image_scale(image_buffers->buffer_image[0].height, image_buffers->buffer_image[0].width, CV_8UC3, image_buffers->buffer_image[0].image);
                cv::resize(mat_base_image, mat_base_image_scale, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

                image_buffers->buffer_image[1].width = mat_base_image_scale.cols;
                image_buffers->buffer_image[1].height = mat_base_image_scale.rows;
                image_buffers->buffer_image[1].channel_count = 3;
                cv::Mat mat_base_image_scale_flip_temp(image_buffers->buffer_image[1].height, image_buffers->buffer_image[1].width, CV_8UC3, image_buffers->buffer_image[1].image);
                cv::flip(mat_base_image_scale, mat_base_image_scale_flip_temp, -1);

                image_buffers->draw_image[0].width = mat_base_image_scale_flip_temp.cols;
                image_buffers->draw_image[0].height = mat_base_image_scale_flip_temp.rows;
                image_buffers->draw_image[0].channel_count = 4;
                cv::Mat mat_base_image_scale_flip(image_buffers->draw_image[0].height, image_buffers->draw_image[0].width, CV_8UC4, image_buffers->draw_image[0].image);
                cv::cvtColor(mat_base_image_scale_flip_temp, mat_base_image_scale_flip, cv::COLOR_BGR2RGBA);
            }
            else {
                // base image
                cv::Mat mat_base_image(image_state->isc_image_Info.frame_data[fd_inex].p1.height, image_state->isc_image_Info.frame_data[fd_inex].p1.width, CV_8UC1, image_state->isc_image_Info.frame_data[fd_inex].p1.image);

                double ratio = GetResizeRatio(gui_control_latest.gui_loc_images[0].size.cx, image_state->isc_image_Info.frame_data[fd_inex].p1.width);

                image_buffers->buffer_image[0].width = image_state->isc_image_Info.frame_data[fd_inex].p1.width;
                image_buffers->buffer_image[0].height = image_state->isc_image_Info.frame_data[fd_inex].p1.height;
                image_buffers->buffer_image[0].channel_count = 1;
                cv::Mat mat_base_image_scale(image_buffers->buffer_image[0].height, image_buffers->buffer_image[0].width, CV_8UC1, image_buffers->buffer_image[0].image);
                cv::resize(mat_base_image, mat_base_image_scale, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

                image_buffers->buffer_image[1].width = mat_base_image_scale.cols;
                image_buffers->buffer_image[1].height = mat_base_image_scale.rows;
                image_buffers->buffer_image[1].channel_count = 1;
                cv::Mat mat_base_image_scale_flip_temp(image_buffers->buffer_image[1].height, image_buffers->buffer_image[1].width, CV_8UC1, image_buffers->buffer_image[1].image);
                cv::flip(mat_base_image_scale, mat_base_image_scale_flip_temp, -1);

                image_buffers->draw_image[0].width = mat_base_image_scale_flip_temp.cols;
                image_buffers->draw_image[0].height = mat_base_image_scale_flip_temp.rows;
                image_buffers->draw_image[0].channel_count = 4;
                cv::Mat mat_base_image_scale_flip(image_buffers->draw_image[0].height, image_buffers->draw_image[0].width, CV_8UC4, image_buffers->draw_image[0].image);
                cv::cvtColor(mat_base_image_scale_flip_temp, mat_base_image_scale_flip, cv::COLOR_BGR2RGBA);
            }

            if (image_state->isc_image_Info.grab == IscGrabMode::kParallax) {
                // depth
                const int depth_width = image_state->isc_image_Info.frame_data[fd_inex].depth.width;
                const int depth_height = image_state->isc_image_Info.frame_data[fd_inex].depth.height;
                float* depth = image_state->isc_image_Info.frame_data[fd_inex].depth.image;

                if ((depth_width != 0) && (depth_height != 0)) {
                    {
                        const double min_distance = gui_control_latest.pcl_filter_parameter.pass_through_filter_range.min;
                        const double max_distance = gui_control_latest.pcl_filter_parameter.pass_through_filter_range.max;

                        double min_distance_temp = 0.0f;
                        double max_distance_temp = 0.0f;
                        image_state->dpl_control->GetMinMaxDistance(&min_distance_temp, &max_distance_temp);

                        if ((min_distance != min_distance_temp) || (max_distance != max_distance_temp)) {
                            image_state->dpl_control->RebuildDrawColorMap(min_distance, max_distance);
                        }
                    }

                    image_state->dpl_control->ConvertDisparityToImage(image_state->b, image_state->angle, image_state->bf, image_state->dinf,
                        depth_width, depth_height, depth, image_state->bgra_image);

                    cv::Mat mat_depth_image_temp(depth_height, depth_width, CV_8UC4, image_state->bgra_image);

                    image_buffers->buffer_image[2].width = mat_depth_image_temp.cols;
                    image_buffers->buffer_image[2].height = mat_depth_image_temp.rows;
                    image_buffers->buffer_image[2].channel_count = 4;
                    cv::Mat mat_depth_image(image_buffers->buffer_image[2].height, image_buffers->buffer_image[2].width, CV_8UC4, image_buffers->buffer_image[2].image);
                    cv::cvtColor(mat_depth_image_temp, mat_depth_image, cv::COLOR_BGRA2RGBA);   // cv::COLOR_BGR2RGBA);

                    double ratio = GetResizeRatio(gui_control_latest.gui_loc_images[1].size.cx, mat_depth_image.cols);

                    image_buffers->buffer_image[3].width = mat_depth_image.cols;
                    image_buffers->buffer_image[3].height = mat_depth_image.rows;
                    image_buffers->buffer_image[3].channel_count = 4;
                    cv::Mat mat_depth_image_scale(image_buffers->buffer_image[3].height, image_buffers->buffer_image[3].width, CV_8UC4, image_buffers->buffer_image[3].image);
                    cv::resize(mat_depth_image, mat_depth_image_scale, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

                    image_buffers->draw_image[1].width = mat_depth_image_scale.cols;
                    image_buffers->draw_image[1].height = mat_depth_image_scale.rows;
                    image_buffers->draw_image[1].channel_count = 4;
                    cv::Mat mat_depth_image_scale_flip(image_buffers->draw_image[1].height, image_buffers->draw_image[1].width, CV_8UC4, image_buffers->draw_image[1].image);
                    cv::flip(mat_depth_image_scale, mat_depth_image_scale_flip, -1);
                }
            }
            else if (   (image_state->isc_image_Info.grab == IscGrabMode::kCorrect) ||
                        (image_state->isc_image_Info.grab == IscGrabMode::kBeforeCorrect)) {

                if ((image_state->isc_image_Info.frame_data[fd_inex].p2.width != 0) && (image_state->isc_image_Info.frame_data[fd_inex].p2.height != 0)) {
                    // compare image
                    cv::Mat mat_base_image(image_state->isc_image_Info.frame_data[fd_inex].p2.height, image_state->isc_image_Info.frame_data[fd_inex].p2.width, CV_8UC1, image_state->isc_image_Info.frame_data[fd_inex].p2.image);

                    double ratio = GetResizeRatio(gui_control_latest.gui_loc_images[1].size.cx, image_state->isc_image_Info.frame_data[fd_inex].p2.width);

                    image_buffers->buffer_image[2].width = image_state->isc_image_Info.frame_data[fd_inex].p2.width;
                    image_buffers->buffer_image[2].height = image_state->isc_image_Info.frame_data[fd_inex].p2.height;
                    image_buffers->buffer_image[2].channel_count = 1;
                    cv::Mat mat_base_image_scale(image_buffers->buffer_image[2].height, image_buffers->buffer_image[2].width, CV_8UC1, image_buffers->buffer_image[2].image);
                    cv::resize(mat_base_image, mat_base_image_scale, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

                    image_buffers->buffer_image[3].width = mat_base_image_scale.cols;
                    image_buffers->buffer_image[3].height = mat_base_image_scale.rows;
                    image_buffers->buffer_image[3].channel_count = 1;
                    cv::Mat mat_base_image_scale_flip_temp(image_buffers->buffer_image[3].height, image_buffers->buffer_image[3].width, CV_8UC1, image_buffers->buffer_image[3].image);
                    cv::flip(mat_base_image_scale, mat_base_image_scale_flip_temp, -1);

                    image_buffers->draw_image[1].width = mat_base_image_scale_flip_temp.cols;
                    image_buffers->draw_image[1].height = mat_base_image_scale_flip_temp.rows;
                    image_buffers->draw_image[1].channel_count = 4;
                    cv::Mat mat_base_image_scale_flip(image_buffers->draw_image[1].height, image_buffers->draw_image[1].width, CV_8UC4, image_buffers->draw_image[1].image);
                    cv::cvtColor(mat_base_image_scale_flip_temp, mat_base_image_scale_flip, cv::COLOR_BGR2RGBA);
                }
            }
        }

        if ((image_buffers->draw_image[0].width == 0) || (image_buffers->draw_image[0].height == 0)) {
            Sleep(16);
        }
        else {
            ImGui::SetNextWindowSize(ImVec2((float)image_buffers->draw_image[0].width, (float)image_buffers->draw_image[0].height), ImGuiCond_Once);

            float xp = (float)gui_control_latest.gui_loc_images[0].position.x;
            float yp = (float)gui_control_latest.gui_loc_images[0].position.y;
            ImGui::SetNextWindowPos(ImVec2(xp, yp), ImGuiCond_Once);

            ImGui::Begin("imgui image", &is_show);
            //ImGui::Text("This is base image.");

            glBindTexture(GL_TEXTURE_2D, texture[0]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_buffers->draw_image[0].width, image_buffers->draw_image[0].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_buffers->draw_image[0].image);
            ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(texture[0])), ImVec2((float)image_buffers->draw_image[0].width, (float)image_buffers->draw_image[0].height));
            ImGui::End();
        }

        if ((image_buffers->draw_image[1].width == 0) || (image_buffers->draw_image[1].height == 0)) {
            Sleep(16);
        }
        else {
            ImGui::SetNextWindowSize(ImVec2((float)image_buffers->draw_image[1].width, (float)image_buffers->draw_image[1].height), ImGuiCond_Once);
            float xp = (float)gui_control_latest.gui_loc_images[1].position.x;
            float yp = (float)gui_control_latest.gui_loc_images[1].position.y;
            ImGui::SetNextWindowPos(ImVec2(xp, yp), ImGuiCond_Once);

            ImGui::Begin("imgui depth", &is_show);
            //ImGui::Text("This is depth image.");

            glBindTexture(GL_TEXTURE_2D, texture[1]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_buffers->draw_image[1].width, image_buffers->draw_image[1].height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_buffers->draw_image[1].image);
            ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(texture[1])), ImVec2((float)image_buffers->draw_image[1].width, (float)image_buffers->draw_image[1].height));
            ImGui::End();
        }
    }
    return 0;
}

/**
 * PCLのVisualizerを使った表示へのデータ提供.
 *
 * @param[in] gui_control_latest GUIコンポーネントの最新の状態
 * @param[in] dpl_control_start_mode_latest DPL開始設定
 * @param[in] image_state DPLControlを含むDPL制御用構造体
 * @param[in] image_buffers 作業用Buffer
 * @param[in] input_args 3D表示用設定
 * @param[in] output_args 3D表示GUからのデータ
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int DrawPCLVizImage(GuiControls& gui_control_latest, DplControl::StartMode& dpl_control_start_mode_latest, ImageState* image_state, ImageDataBuffers* image_buffers,
                    PclVizInputArgs* input_args, PclVizOutputArgs* output_args)
{
    int mode = 0;
    bool is_show = true;

    if (gui_control_latest.stereo_matching && gui_control_latest.disparity_filter) {
        mode = 0;
    }
    else if (   !gui_control_latest.stereo_matching &&
                gui_control_latest.disparity_filter &&
                (dpl_control_start_mode_latest.grab_mode == (int)GrabMode::Disparity)) {
        mode = 0;
    }
    else if (   !gui_control_latest.stereo_matching &&
                !gui_control_latest.disparity_filter &&
                (dpl_control_start_mode_latest.grab_mode == (int)GrabMode::Disparity)) {
        mode = 1;
    }
    else {
        return 0;
    }

    if (mode == 0) {
        // images from camera and depth from software matching
        bool camera_status = image_state->dpl_control->GetCameraData(&image_state->isc_image_Info);
        const int fd_inex = kISCIMAGEINFO_FRAMEDATA_LATEST;

        if ((image_state->isc_image_Info.frame_data[fd_inex].p1.width == 0) ||
            (image_state->isc_image_Info.frame_data[fd_inex].p1.height == 0)) {

            camera_status = false;
        }

        // data processing result
        bool data_proc_status = image_state->dpl_control->GetDataProcessingData(&image_state->isc_data_proc_result_data);

        if ((image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].depth.width == 0) ||
            (image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].depth.height == 0)) {

            data_proc_status = false;
        }

        if (data_proc_status){

            // Process post-processing requests
            if (gui_control_latest.post_camera_status_request) {
                gui_control_latest.post_camera_status_request = false;
                gui_control_latest.update_camera_status_request = true;
            }

            // Do you have a Color image?
            bool is_color_exists = false;
            if (image_state->color_mode == 1) {
                if ((image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].color.width != 0) &&
                    (image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].color.height != 0)) {

                    is_color_exists = true;
                }
            }

            if (is_color_exists) {
                // color image
                input_args->width                       = image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].color.width;
                input_args->height                      = image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].color.height;
                input_args->base_image_channel_count    = 3;
                input_args->image                       = image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].color.image;
            }
            else {
                // base image
                input_args->width                       = image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].p1.width;
                input_args->height                      = image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].p1.height;
                input_args->base_image_channel_count    = 1;
                input_args->image                       = image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].p1.image;
            }

            int width       = image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].depth.width;
            int height      = image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].depth.height;
            float* depth    = image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].depth.image;

            if (width != input_args->width) {
                // 4Kカメラの場合は、視差が大きい
                double ratio = (double)input_args->width / (double)width;

                int new_width = (int)((double)width * ratio);
                int new_height = (int)((double)height * ratio);

                cv::Mat mat_depth(height, width, CV_32F, depth);
                cv::Mat mat_depth_scale(new_height, new_width, CV_32F, image_buffers->buffer_depth[0].depth);

                cv::resize(mat_depth, mat_depth_scale, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

                width = mat_depth_scale.cols;
                height = mat_depth_scale.rows;
                depth = image_buffers->buffer_depth[0].depth;
            }

            if (gui_control_latest.viz_mode_3d_im_src_depth_heat) {
                
                const double min_distance = gui_control_latest.pcl_filter_parameter.pass_through_filter_range.min;
                const double max_distance = gui_control_latest.pcl_filter_parameter.pass_through_filter_range.max;
                
                double min_distance_temp = 0.0f;
                double max_distance_temp = 0.0f;
                image_state->dpl_control->GetMinMaxDistance(&min_distance_temp, &max_distance_temp);

                if ((min_distance != min_distance_temp) || (max_distance != max_distance_temp)) {
                    image_state->dpl_control->RebuildDrawColorMap(min_distance, max_distance);
                }
                
                image_state->dpl_control->ConvertDisparityToImage(  image_state->b, image_state->angle, image_state->bf, image_state->dinf,
                                                                    width, height, depth, image_state->bgra_image);

                // color image
                input_args->width   = width;
                input_args->height  = height;
                input_args->base_image_channel_count = 4;
                input_args->image   = image_state->bgra_image;
            }

            input_args->depth_width             = width;
            input_args->depth_height            = height;
            input_args->disparity_data          = depth;
            input_args->disparity_image_bgra    = image_state->bgra_image;

            input_args->pcl_filter_parameter.enabled_remove_nan             = gui_control_latest.pcl_filter_parameter.enabled_remove_nan;

            input_args->pcl_filter_parameter.enabled_pass_through_filter    = gui_control_latest.pcl_filter_parameter.enabled_pass_through_filter;
            input_args->pcl_filter_parameter.pass_through_filter_range.min  = gui_control_latest.pcl_filter_parameter.pass_through_filter_range.min;
            input_args->pcl_filter_parameter.pass_through_filter_range.max  = gui_control_latest.pcl_filter_parameter.pass_through_filter_range.max;

            input_args->pcl_filter_parameter.enabled_down_sampling          = gui_control_latest.pcl_filter_parameter.enabled_down_sampling;
            input_args->pcl_filter_parameter.down_sampling_boxel_size       = gui_control_latest.pcl_filter_parameter.down_sampling_boxel_size;

            input_args->pcl_filter_parameter.enabled_radius_outlier_removal             = gui_control_latest.pcl_filter_parameter.enabled_radius_outlier_removal;
            input_args->pcl_filter_parameter.radius_outlier_removal_param.radius_search = gui_control_latest.pcl_filter_parameter.radius_outlier_removal_param.radius_search;
            input_args->pcl_filter_parameter.radius_outlier_removal_param.min_neighbors = gui_control_latest.pcl_filter_parameter.radius_outlier_removal_param.min_neighbors;

            input_args->pcl_filter_parameter.enabled_plane_detection        = gui_control_latest.pcl_filter_parameter.enabled_plane_detection;
            input_args->pcl_filter_parameter.plane_detection_threshold      = gui_control_latest.pcl_filter_parameter.plane_detection_threshold;

            input_args->base_length                                         = image_state->b;
            input_args->bf                                                  = image_state->bf;
            input_args->d_inf                                               = image_state->dinf;

            // one shot
            input_args->full_screen_request                                 = gui_control_latest.viz_mode_3d_full_screen_req;
            input_args->restore_screen_request                              = gui_control_latest.viz_mode_3d_restore_screen_req;
            gui_control_latest.viz_mode_3d_full_screen_req    = false;
            gui_control_latest.viz_mode_3d_restore_screen_req = false;

            int viz_ret = RunPclViz(input_args, output_args);
        }
    }
    else if (mode == 1) {
        // images and depth from camera
        bool camera_status = image_state->dpl_control->GetCameraData(&image_state->isc_image_Info);
        const int fd_inex = kISCIMAGEINFO_FRAMEDATA_LATEST;

        if ((image_state->isc_image_Info.frame_data[fd_inex].p1.width == 0) ||
            (image_state->isc_image_Info.frame_data[fd_inex].p1.height == 0)) {

            camera_status = false;
        }

        if (image_state->isc_image_Info.grab == IscGrabMode::kParallax) {
        }
        else {
            // No parallax data available.
            camera_status = false;
        }

        if (camera_status) {

            // Process post-processing requests
            if (gui_control_latest.post_camera_status_request) {
                gui_control_latest.post_camera_status_request = false;
                gui_control_latest.update_camera_status_request = true;
            }

            // Do you have a Color image?
            bool is_color_exists = false;
            if (image_state->color_mode == 1) {
                if ((image_state->isc_image_Info.frame_data[fd_inex].color.width != 0) &&
                    (image_state->isc_image_Info.frame_data[fd_inex].color.height != 0)) {

                    is_color_exists = true;
                }
            }

            if (is_color_exists) {
                // color image
                input_args->width                       = image_state->isc_image_Info.frame_data[fd_inex].color.width;
                input_args->height                      = image_state->isc_image_Info.frame_data[fd_inex].color.height;
                input_args->base_image_channel_count    = 3;
                input_args->image                       = image_state->isc_image_Info.frame_data[fd_inex].color.image;
            }
            else {
                // base image
                input_args->width                       = image_state->isc_image_Info.frame_data[fd_inex].p1.width;
                input_args->height                      = image_state->isc_image_Info.frame_data[fd_inex].p1.height;
                input_args->base_image_channel_count    = 1;
                input_args->image                       = image_state->isc_image_Info.frame_data[fd_inex].p1.image;
            }

            int width       = image_state->isc_image_Info.frame_data[fd_inex].depth.width;
            int height      = image_state->isc_image_Info.frame_data[fd_inex].depth.height;
            float* depth    = image_state->isc_image_Info.frame_data[fd_inex].depth.image;

            if (width != input_args->width) {
                // 4Kカメラの場合は、視差が大きい
                double ratio = (double)input_args->width / (double)width;

                int new_width = (int)((double)width * ratio);
                int new_height = (int)((double)height * ratio);

                cv::Mat mat_depth(height, width, CV_32F, depth);
                cv::Mat mat_depth_scale(new_height, new_width, CV_32F, image_buffers->buffer_depth[0].depth);

                cv::resize(mat_depth, mat_depth_scale, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

                width = mat_depth_scale.cols;
                height = mat_depth_scale.rows;
                depth = image_buffers->buffer_depth[0].depth;
            }

            if (gui_control_latest.viz_mode_3d_im_src_depth_heat) {
                const double min_distance = gui_control_latest.pcl_filter_parameter.pass_through_filter_range.min;
                const double max_distance = gui_control_latest.pcl_filter_parameter.pass_through_filter_range.max;

                double min_distance_temp = 0.0f;
                double max_distance_temp = 0.0f;
                image_state->dpl_control->GetMinMaxDistance(&min_distance_temp, &max_distance_temp);

                if ((min_distance != min_distance_temp) || (max_distance != max_distance_temp)) {
                    image_state->dpl_control->RebuildDrawColorMap(min_distance, max_distance);
                }

                image_state->dpl_control->ConvertDisparityToImage(  image_state->b, image_state->angle, image_state->bf, image_state->dinf,
                                                                    width, height, depth, image_state->bgra_image);

                // color image
                input_args->width                    = width;
                input_args->height                   = height;
                input_args->base_image_channel_count = 4;
                input_args->image                    = image_state->bgra_image;
            }

            input_args->depth_width             = width;
            input_args->depth_height            = height;
            input_args->disparity_data          = depth;
            input_args->disparity_image_bgra    = image_state->bgra_image;

            input_args->pcl_filter_parameter.enabled_remove_nan             = gui_control_latest.pcl_filter_parameter.enabled_remove_nan;

            input_args->pcl_filter_parameter.enabled_pass_through_filter    = gui_control_latest.pcl_filter_parameter.enabled_pass_through_filter;
            input_args->pcl_filter_parameter.pass_through_filter_range.min  = gui_control_latest.pcl_filter_parameter.pass_through_filter_range.min;
            input_args->pcl_filter_parameter.pass_through_filter_range.max  = gui_control_latest.pcl_filter_parameter.pass_through_filter_range.max;

            input_args->pcl_filter_parameter.enabled_down_sampling          = gui_control_latest.pcl_filter_parameter.enabled_down_sampling;
            input_args->pcl_filter_parameter.down_sampling_boxel_size       = gui_control_latest.pcl_filter_parameter.down_sampling_boxel_size;

            input_args->pcl_filter_parameter.enabled_radius_outlier_removal             = gui_control_latest.pcl_filter_parameter.enabled_radius_outlier_removal;
            input_args->pcl_filter_parameter.radius_outlier_removal_param.radius_search = gui_control_latest.pcl_filter_parameter.radius_outlier_removal_param.radius_search;
            input_args->pcl_filter_parameter.radius_outlier_removal_param.min_neighbors = gui_control_latest.pcl_filter_parameter.radius_outlier_removal_param.min_neighbors;

            input_args->pcl_filter_parameter.enabled_plane_detection        = gui_control_latest.pcl_filter_parameter.enabled_plane_detection;
            input_args->pcl_filter_parameter.plane_detection_threshold      = gui_control_latest.pcl_filter_parameter.plane_detection_threshold;

            input_args->base_length                                         = image_state->b;
            input_args->bf                                                  = image_state->bf;
            input_args->d_inf                                               = image_state->dinf;

            // one shot
            input_args->full_screen_request                                 = gui_control_latest.viz_mode_3d_full_screen_req;
            input_args->restore_screen_request                              = gui_control_latest.viz_mode_3d_restore_screen_req;
            gui_control_latest.viz_mode_3d_full_screen_req                  = false;
            gui_control_latest.viz_mode_3d_restore_screen_req               = false;

            int viz_ret = RunPclViz(input_args, output_args);
        }
    }

    return 0;
}



