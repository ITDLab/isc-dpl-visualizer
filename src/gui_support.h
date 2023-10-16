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
 * @file gui_support.h
 * @brief Controls Gui components and screen rendering.
 */

#pragma once

/** @struct  InitializeWindowParameter
 *  @brief Setup for initialization
 */
struct InitializeWindowParameter {
	int max_width;							/**< Maximum image width */
	int max_height;							/**< Maximum image height */

	bool enable_camera;						/**< Camera is connected. */

	bool enable_data_processing_library;	/**< Data processing library enabled */
	bool enabled_3d_viewer;					/**< 3D display is enabled */
};

/** @brief Creation and initialization of GLFW Window.
	@return 0, if successful.
 */
GLFWwindow* InitializeWindow(InitializeWindowParameter* initialze_window_parameter);

/** @brief Destroy GLFW Window and release resources.
	@return 0, if successful.
 */
int TerminateWindow();

/** @brief Update the GUI content with the current camera status.
	@return 0, if successful.
 */
int InitializeWindowCameraControl(const bool is_update, ImageState* image_state);

/** @brief Update GUI components and draw images (2D/3D).
	@return 0, if successful.
 */
int DrawWindow(ImageState* image_state);

