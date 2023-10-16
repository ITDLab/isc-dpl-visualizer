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
 * @file dpl_support.cpp
 * @brief Provide functionality to assist with DPL Control calls
 * @author Takayuki
 * @date 2022.11.21
 * @version 0.1
 *
 * @details Summarizes DPL Control calls and logs them to the console.
 */

#include <windows.h>
#include <cstdlib>
#include <cstdio>
#include <locale.h>
#include <tchar.h>
#include <imagehlp.h>
#include <Shlwapi.h>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>

#include "isc_dpl_error_def.h"
#include "isc_dpl_def.h"
#include "isc_dpl.h"
#include "dpl_controll.h"

#include "dpl_support.h"

#include "opencv2/opencv.hpp"

/**
 * DPLContolを初期化します
 *
 * @param[in] module_path 現在実行中の実行ファイルのフルパス
 * @param[in] image_state DPLControlを含むDPL制御用構造体
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int InitializeDplControl(const wchar_t* module_path, ImageState* image_state)
{

	image_state->dpl_control = new DplControl;
	bool ret = image_state->dpl_control->Initialize(module_path);
	if (!ret) {
		std::cout << "[ERR]initialization failed\n";

		image_state->dpl_control->Terminate();
		delete image_state->dpl_control;

		return -1;
	}

	ret = image_state->dpl_control->InitializeBuffers(&image_state->isc_image_Info, &image_state->isc_data_proc_result_data);
	if (!ret) {
		std::cout << "[ERR]initialization failed\n";

		image_state->dpl_control->Terminate();
		delete image_state->dpl_control;

		return -1;
	}

	ret = image_state->dpl_control->GetCameraParameter(&(image_state->b), &(image_state->bf), &(image_state->dinf), &(image_state->width), &(image_state->height));
	if (!ret) {
		std::cout << "[ERR]initialization failed\n";

		image_state->dpl_control->Terminate();
		delete image_state->dpl_control;

		return -1;
	}

	if (image_state->width != 0 && image_state->height != 0) {
		image_state->bgra_image = new unsigned char[image_state->width * image_state->height * 4];
	}

	return 0;
}

/**
 * リソースの解放.
 *
 * @param[in] image_state DPLControlを含むDPL制御用構造体
 *
 * @retval 0 成功
 * @retval other 失敗
 *
 */
int TerminateDplControl(ImageState* image_state)
{
	delete[] image_state->bgra_image;
	image_state->bgra_image = nullptr;
	int ret = image_state->dpl_control->ReleaseBuffers(&image_state->isc_image_Info, &image_state->isc_data_proc_result_data);

	image_state->dpl_control->Terminate();
	delete image_state->dpl_control;
	image_state->dpl_control = nullptr;

	return 0;
}

/**
 * 現在のカメラモデルを返します.
 *
 * @param[in] image_state DPLControlを含むDPL制御用構造体
 *
 * @retval [0:1:4] カメラモデル
 *
 */
int GetCameraModel(ImageState* image_state)
{
	return image_state->dpl_control->GetCameraModel();
}

/**
 * カメラの有効・無効を返します.
 *
 * @param[in] image_state DPLControlを含むDPL制御用構造体
 *
 * @retval true カメラが有効
 *
 */
bool GetCameraEnabled(ImageState* image_state)
{
	return image_state->dpl_control->GetCameraEnabled();
}

/**
 * カメラデータの保存先を返します.
 *
 * @param[out] path データ保存フォルダー
 * @param[in] max_length pathの最大長さ
 * @param[in] image_state DPLControlを含むDPL制御用構造体
 *
 * @retval true 成功
 *
 */
bool GetDataRecordPath(ImageState* image_state, wchar_t* path, const int max_length)
{
	image_state->dpl_control->GetDataRecordPath(path, max_length);

	return true;
}

/**
 * 描画する最大距離を返します.
 *
 * @param[in] image_state DPLControlを含むDPL制御用構造体
 *
 * @retval 描画最大距離(m)
 *
 */
double GetDrawMinDistance(ImageState* image_state)
{
	return image_state->dpl_control->GetDrawMinDistance();
}

/**
 * 描画する最小距離を返します.
 *
 * @param[in] image_state DPLControlを含むDPL制御用構造体
 *
 * @retval 描画最小距離(m)
 *
 */
double GetDrawMaxDistance(ImageState* image_state)
{
	return image_state->dpl_control->GetDrawMaxDistance();
}

/**
 * 取り込みを開始する.
 *
 * @param[in] start_mode 開始用の設定
 * @param[in] image_state DPLControlを含むDPL制御用構造体
 *
 * @retval 0 成功
 * @retval other 失敗
 *
 */
int DplStart(DplControl::StartMode& start_mode, ImageState* image_state)
{
	bool ret = image_state->dpl_control->Start(start_mode);
	if (!ret) {
		return -1;
	}

	return 0;
}

/**
 * 取り込みを停止する.
 *
 * @param[in] image_state DPLControlを含むDPL制御用構造体
 *
 * @retval 0 成功
 * @retval other 失敗
 *
 */
int DplStop(ImageState* image_state)
{
	bool ret = image_state->dpl_control->Stop();
	if (!ret) {
		return -1;
	}

	return 0;
}

/**
 * 保存されたデータファイルのヘッダー情報を取得します.
 *
 * @param[in] file_name データファイル名
 * @param[out] raw_file_headaer ヘッダーデータ
 *
 * @retval 0 成功
 * @retval other 失敗
 *
 */
int GetPlayFileInformation(ImageState* image_state, wchar_t* file_name, IscRawFileHeader* raw_file_headaer)
{
	bool ret = image_state->dpl_control->GetFileInformation(file_name, raw_file_headaer);
	if (!ret) {
		return -1;
	}

	return 0;
}

/**
 * DEBUG用　OpenCVの機能を使用して取得画像を表示する　通常は、呼び出されない.
 *
 * @param[in] display_scale 表示倍率
 * @param[out] display_mode 表示内容　1固定
 * @param[in] image_state DPLControlを含むDPL制御用構造体
 *
 * @retval 0 成功
 * @retval other 失敗
 *
 */
int ImageHandlerForDplControl(const int display_scale, const int display_mode, ImageState* image_state)
{                 

	// images from camera
	bool camera_status = image_state->dpl_control->GetCameraData(&image_state->isc_image_Info);
	const int fd_inex = kISCIMAGEINFO_FRAMEDATA_LATEST;

	if ((image_state->isc_image_Info.frame_data[fd_inex].p1.width == 0) ||
		(image_state->isc_image_Info.frame_data[fd_inex].p1.height == 0)) {

		camera_status = false;
	}

	cv::Mat mat_base_image_scale_flip;
	if (camera_status) {
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

			double ratio = 1.0 / (double)display_scale;
			cv::Mat mat_base_image_scale;
			cv::resize(mat_base_image, mat_base_image_scale, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

			cv::flip(mat_base_image_scale, mat_base_image_scale_flip, -1);

			cv::imshow("Base Image", mat_base_image_scale_flip);
		}
		else {
			// base image
			cv::Mat mat_base_image(image_state->isc_image_Info.frame_data[fd_inex].p1.height, image_state->isc_image_Info.frame_data[fd_inex].p1.width, CV_8U, image_state->isc_image_Info.frame_data[fd_inex].p1.image);

			double ratio = 1.0 / (double)display_scale;
			cv::Mat mat_base_image_scale;
			cv::resize(mat_base_image, mat_base_image_scale, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

			cv::Mat mat_base_image_scale_flip_temp;
			cv::flip(mat_base_image_scale, mat_base_image_scale_flip_temp, -1);

			cv::Mat mat_base_image_color;
			cv::cvtColor(mat_base_image_scale_flip_temp, mat_base_image_scale_flip, cv::COLOR_GRAY2RGB);

			cv::imshow("Base Image", mat_base_image_scale_flip);
		}
	}

	// data processing result
	bool data_proc_status = image_state->dpl_control->GetDataProcessingData(&image_state->isc_data_proc_result_data);

	if ((image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].depth.width == 0) ||
		(image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].depth.height == 0)) {

		data_proc_status = false;
	}

	cv::Mat mat_depth_image_scale_flip;
	if (data_proc_status) {
		// depth
		const int width = image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].depth.width;
		const int height = image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].depth.height;
		float* depth = image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].depth.image;

		image_state->dpl_control->ConvertDisparityToImage(image_state->b, image_state->angle, image_state->bf, image_state->dinf,
			width, height, depth, image_state->bgra_image);

		cv::Mat mat_depth_image_temp(height, width, CV_8UC4, image_state->bgra_image);

		cv::Mat mat_depth_image;
		cv::cvtColor(mat_depth_image_temp, mat_depth_image, cv::COLOR_BGRA2BGR);

		double ratio = 1.0 / (double)display_scale;
		cv::Mat mat_depth_image_scale;
		cv::resize(mat_depth_image, mat_depth_image_scale, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

		cv::flip(mat_depth_image_scale, mat_depth_image_scale_flip, -1);

		cv::imshow("Depth Image", mat_depth_image_scale_flip);
	}

	// overlap image
	if (display_mode == 1) {
		if (camera_status && data_proc_status) {

			if (mat_base_image_scale_flip.empty()) {
				return 0;
			}

			if (mat_depth_image_scale_flip.empty()) {
				return 0;
			}

			cv::Mat mat_data_proc_image_scale_flip;
			bool is_color_exists = false;
			if (image_state->color_mode == 1) {
				if ((image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].color.width != 0) &&
					(image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].color.height != 0)) {

					is_color_exists = true;
				}
			}

			if (is_color_exists) {
				// color image
				cv::Mat mat_base_image(image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].color.height,
					image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].color.width,
					CV_8UC3,
					image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].color.image);

				double ratio = 1.0 / (double)display_scale;
				cv::Mat mat_base_image_scale;
				cv::resize(mat_base_image, mat_base_image_scale, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

				cv::flip(mat_base_image_scale, mat_data_proc_image_scale_flip, -1);

			}
			else {
				// base image
				cv::Mat mat_base_image(image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].p1.height,
					image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].p1.width,
					CV_8U,
					image_state->isc_data_proc_result_data.isc_image_info.frame_data[fd_inex].p1.image);

				double ratio = 1.0 / (double)display_scale;
				cv::Mat mat_base_image_scale;
				cv::resize(mat_base_image, mat_base_image_scale, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

				cv::Mat mat_base_image_scale_flip_temp;
				cv::flip(mat_base_image_scale, mat_base_image_scale_flip_temp, -1);

				cv::Mat mat_base_image_color;
				cv::cvtColor(mat_base_image_scale_flip_temp, mat_data_proc_image_scale_flip, cv::COLOR_GRAY2RGB);
			}

			double alpha = 0.7;
			double beta = (1.0 - alpha);

			// blend it
			cv::Mat mat_blend_image;
			cv::addWeighted(mat_data_proc_image_scale_flip, alpha, mat_depth_image_scale_flip, beta, 0.0, mat_blend_image);

			cv::imshow("Blend Image", mat_blend_image);
		}
	}

	return 0;
}

