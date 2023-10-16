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
 * @file dpl_support.h
 * @brief Provide functionality to assist with DPL Control calls.
 */

#pragma once

struct ImageState {
	int grab_mode;		// *not used* 0:parallax  1: after correct 1:before correct
	int color_mode;		// 0:off 1:on
	int width, height;

	float b, bf, dinf;
	double angle;

	DplControl* dpl_control;
	IscImageInfo isc_image_Info;
	IscDataProcResultData isc_data_proc_result_data;

	unsigned char* bgra_image;
};

/** @brief Initializes the DPL Control. Must be called at least once before streaming is started.
	@return 0, if successful.
 */
int InitializeDplControl(const wchar_t* module_path, ImageState* image_state);

/** @brief ... Shut down the runtime system. Don't call any method after calling Terminate().
@return 0, if successful.
*/
int TerminateDplControl(ImageState* image_state);

/** @brief Returns the current camera model.
	@return camera models.
 */
int GetCameraModel(ImageState* image_state);

/** @brief Returns whether the camera is enabled or disabled.
	@return true, if enabled.
 */
bool GetCameraEnabled(ImageState* image_state);

/** @brief Returns the location where the camera data is stored.
	@return true, if successful.
 */
bool GetDataRecordPath(ImageState* image_state, wchar_t* path, const int max_length);

/** @brief Returns the minimum distance to be drawn.
	@return minimum distance.
 */
double GetDrawMinDistance(ImageState* image_state);

/** @brief Returns the maximum distance to draw.
	@return maximum distance.
 */
double GetDrawMaxDistance(ImageState* image_state);

/** @brief Start capturing.
	@return 0, if successful.
 */
int DplStart(DplControl::StartMode& start_mode, ImageState* image_state);

/** @brief Stop capturing.
	@return 0, if successful.
 */
int DplStop(ImageState* image_state);

/** @brief Retrieves header information for saved data files.
	@return 0, if successful.
 */
int GetPlayFileInformation(ImageState* image_state, wchar_t* file_name, IscRawFileHeader* raw_file_headaer);

/** @brief Displays the acquired image using OpenCV functionality Normally not called.
	@return 0, if successful.
 */
int ImageHandlerForDplControl(const int display_scale, const int display_mode, ImageState* image_state);
