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
 * @file dpl_control.cpp
 * @brief A class to help facilitate interfacing with the DPL library.
 * @author Takayuki
 * @date 2023.10.30
 * @version 0.1
 * 
 * @details wrapper for using DPL Library (IscDpl).
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <tchar.h>

#include "isc_dpl_error_def.h"
#include "isc_dpl_def.h"
#include "isc_dpl.h"
#include "dpl_gui_configuration.h"

#include "dpl_controll.h"

#include "opencv2\opencv.hpp"

/**
 * constructor
 *
 */
DplControl::DplControl() :
    configuration_file_path_(), log_file_path_(), image_path_(), camera_model_(0), camera_enabled_(false),  draw_min_distance_(0.0), draw_max_distance_(0.0),
    is_draw_outside_bounds_(false), isc_image_info_(), isc_data_proc_result_data_(), camera_parameter_(), isc_dpl_configuration_(), isc_dpl_(nullptr), isc_start_mode_(),
    disp_color_map_distance_(), disp_color_map_disparity_(), max_disparity_(0.0)
{

}

/**
 * destructor
 *
 */
DplControl::~DplControl()
{

}

/**
 * クラスを初期化します.
 *
 * @param[in] module_path 現在実行中の実行ファイルのフルパス
 * @retval true 成功
 * @retval false 失敗
 */
bool DplControl::Initialize(const wchar_t* module_path)
{
    printf("[INFO]Start library open processing\n");    

    // configuration file path
    swprintf_s(configuration_file_path_, L"%s", module_path);

    DplGuiConfiguration dpl_config;
    dpl_config.Load(configuration_file_path_);

    dpl_config.GetLogFilePath(log_file_path_, _MAX_PATH);
    dpl_config.GetDataRecordPath(image_path_, _MAX_PATH);

    // draw parameter
    draw_min_distance_ = dpl_config.GetDrawMinDistance();
    draw_max_distance_ = dpl_config.GetDrawMaxDistance();
    is_draw_outside_bounds_ = dpl_config.IsDrawOutsideBounds();

	// open library
	isc_dpl_ = new ns_isc_dpl::IscDpl;

	swprintf_s(isc_dpl_configuration_.configuration_file_path, L"%s", configuration_file_path_);
	swprintf_s(isc_dpl_configuration_.log_file_path, L"%s", log_file_path_);
	isc_dpl_configuration_.log_level = 0;

	isc_dpl_configuration_.enabled_camera = dpl_config.IsEnabledCamera();

	camera_model_ = dpl_config.GetCameraModel();
    camera_enabled_ = dpl_config.IsEnabledCamera();

    if (camera_model_ == 0 || camera_model_ == 1 || camera_model_ == 3) {
        if (camera_model_ == 0) {
            std::cout << "[INFO]Your specified camera is a VM\n";
        }
        else if (camera_model_ == 1) {
            std::cout << "[INFO]Your specified camera is a XC\n";
        }
        else if (camera_model_ == 3) {
            std::cout << "[INFO]Your specified camera is a 4KA\n";
        }
        std::cout << "\n";
    }
    else {
        std::cout << "[ERROR]Your specified camera model is incorrect\n";
        std::cout << "         camera_model: 0:VM 1:XC 3:4KA\n";

        return false;
    }

	IscCameraModel isc_camera_model = IscCameraModel::kUnknown;
	switch (camera_model_) {
	case 0:isc_camera_model = IscCameraModel::kVM; break;
	case 1:isc_camera_model = IscCameraModel::kXC; break;
	case 2:isc_camera_model = IscCameraModel::k4K; break;
	case 3:isc_camera_model = IscCameraModel::k4KA; break;
	case 4:isc_camera_model = IscCameraModel::k4KJ; break;
	}
	isc_dpl_configuration_.isc_camera_model = isc_camera_model;

	swprintf_s(isc_dpl_configuration_.save_image_path, L"%s", image_path_);
	swprintf_s(isc_dpl_configuration_.load_image_path, L"%s", image_path_);

	isc_dpl_configuration_.enabled_data_proc_module = true;

	// open camera for use it
	DPL_RESULT dpl_result = isc_dpl_->Initialize(&isc_dpl_configuration_);

	if (dpl_result == DPC_E_OK) {
		isc_dpl_->InitializeIscIamgeinfo(&isc_image_info_);
		isc_dpl_->InitializeIscDataProcResultData(&isc_data_proc_result_data_);

		isc_dpl_->DeviceGetOption(IscCameraInfo::kBaseLength, &camera_parameter_.b);
		isc_dpl_->DeviceGetOption(IscCameraInfo::kBF, &camera_parameter_.bf);
		isc_dpl_->DeviceGetOption(IscCameraInfo::kDINF, &camera_parameter_.dinf);
		camera_parameter_.setup_angle = 0;
	
        if (isc_dpl_configuration_.enabled_camera) {
            // information
            printf("[INFO]Library opened successfully\n");
            char camera_str[128] = {};
            isc_dpl_->DeviceGetOption(IscCameraInfo::kSerialNumber, &camera_str[0], (int)sizeof(camera_str));
            printf("[INFO]Camera Serial Number:%s\n", camera_str);

            uint64_t fpga_version = 0;
            isc_dpl_->DeviceGetOption(IscCameraInfo::kFpgaVersion, &fpga_version);
            printf("[INFO]Camera FPGA Version:0x%016I64X\n", fpga_version);

            printf("[INFO]Camera Parameter:b(%.3f) bf(%.3f) dinf(%.3f)\n", camera_parameter_.b, camera_parameter_.bf, camera_parameter_.dinf);
        }
        else {
            // information
            printf("[INFO]Library opened successfully(Camera connection is disabled)\n");

            // set some default value
            camera_parameter_.b = 0.1F;
            camera_parameter_.bf = 60.0F;
            camera_parameter_.dinf = 2.01F;
            camera_parameter_.setup_angle = 0.0F;
        }

    }
	else {
        printf("[ERROR]Failed to open library\n");
        return false;
 	}
    
    // display settings
    double max_disparity = 255.0;
    switch (isc_camera_model) {
    case IscCameraModel::kVM: max_disparity = 128.0; break;
    case IscCameraModel::kXC: max_disparity = 255.0; break;
    }
    disp_color_map_distance_.min_value = draw_min_distance_;
    disp_color_map_distance_.max_value = draw_max_distance_;
    disp_color_map_distance_.color_map_size = 0;
    disp_color_map_distance_.color_map_step = 0.01;
    disp_color_map_distance_.color_map_size = (int)(draw_max_distance_ / disp_color_map_distance_.color_map_step) + 1;
    disp_color_map_distance_.color_map = new int[disp_color_map_distance_.color_map_size + sizeof(int)];
    memset(disp_color_map_distance_.color_map, 0, disp_color_map_distance_.color_map_size + sizeof(int));
    BuildColorHeatMap(&disp_color_map_distance_);

    disp_color_map_disparity_.min_value = 0;
    disp_color_map_disparity_.max_value = max_disparity;
    disp_color_map_disparity_.color_map_size = 0;
    disp_color_map_disparity_.color_map_step = 0.25;
    disp_color_map_disparity_.color_map_size = (int)(max_disparity / disp_color_map_disparity_.color_map_step) + 1;
    disp_color_map_disparity_.color_map = new int[disp_color_map_disparity_.color_map_size + sizeof(int)];
    memset(disp_color_map_disparity_.color_map, 0, disp_color_map_disparity_.color_map_size + sizeof(int));
    BuildColorHeatMapForDisparity(&disp_color_map_disparity_);
    
    printf("[INFO]Finished opening the library\n");    

    return true;
}

/**
 * 終了処理をします.
 *
 */
void DplControl::Terminate()
{
    printf("[INFO]Start library terminate processing\n");

    // ended
    delete[] disp_color_map_distance_.color_map;
    disp_color_map_distance_.color_map = nullptr;

    delete[] disp_color_map_disparity_.color_map;
    disp_color_map_disparity_.color_map = nullptr;

	if (isc_dpl_ != nullptr) {
		isc_dpl_->ReleaeIscDataProcResultData(&isc_data_proc_result_data_);
		isc_dpl_->ReleaeIscIamgeinfo(&isc_image_info_);

		isc_dpl_->Terminate();
		delete isc_dpl_;
		isc_dpl_ = nullptr;
	}

    printf("[INFO]Finished terminate the library\n");

    return;
}

/**
 * バッファーの初期化.
 *
 * @param[inout] isc_image_Info 画像バッファー
 * @param[inout] isc_data_proc_result_data データ処理ライブラリバッファー
 *
 * @retval true 成功
 * @retval false 失敗
 *
 */
bool DplControl::InitializeBuffers(IscImageInfo* isc_image_Info, IscDataProcResultData* isc_data_proc_result_data)
{

    int ret = isc_dpl_->InitializeIscIamgeinfo(isc_image_Info);
    if (ret != DPC_E_OK) {
        return false;
    }

    ret = isc_dpl_->InitializeIscDataProcResultData(isc_data_proc_result_data);
    if (ret != DPC_E_OK) {
        return false;
    }

    return true;
}

/**
 * リソースの解放.
 *
 * @param[inout] isc_image_Info 画像バッファー
 * @param[inout] isc_data_proc_result_data データ処理ライブラリバッファー
 *
 * @retval true 成功
 * @retval false 失敗
 *
 */
bool DplControl::ReleaseBuffers(IscImageInfo* isc_image_Info, IscDataProcResultData* isc_data_proc_result_data)
{

    int ret = isc_dpl_->ReleaeIscIamgeinfo(isc_image_Info);
    if (ret != DPC_E_OK) {
        return false;
    }

    ret = isc_dpl_->ReleaeIscDataProcResultData(isc_data_proc_result_data);
    if (ret != DPC_E_OK) {
        return false;
    }

    return true;
}

/**
 * 現在のカメラモデルを返します.
 *
 * @retval [0:1:4] カメラモデル
 *
 */
int DplControl::GetCameraModel() const
{
    return camera_model_;
}

/**
 * カメラの有効・無効を返します.
 *
 * @retval true カメラが有効
 *
 */
bool DplControl::GetCameraEnabled() const
{
    return camera_enabled_;
}

/**
 * カメラデータの保存先を返します.
 *
 * @param[out] path データ保存フォルダー
 * @param[in] max_length pathの最大長さ
 *
 * @retval true 成功
 *
 */
bool DplControl::GetDataRecordPath(wchar_t* path, const int max_length) const
{
    swprintf_s(path, max_length, L"%s", image_path_);

    return true;
}

/**
 * 描画する最大距離を返します.
 *
 * @retval 描画最大距離(m)
 *
 */
double DplControl::GetDrawMinDistance() const
{
    return draw_min_distance_;
}

/**
 * 描画する最小距離を返します.
 *
 * @retval 描画最小距離(m)
 *
 */
double DplControl::GetDrawMaxDistance() const
{
    return draw_max_distance_;
}

/**
 * ライブラリ isc-dpl　のポインタを返します.
 *
 * @retval IscDpl* isc-dpl Object ポインタ
 *
 */
ns_isc_dpl::IscDpl* DplControl::GetDplObgkect() const 
{
    return isc_dpl_;
}

/**
 * 取り込みを開始する.
 *
 * @param[in] start_mode 開始用の設定
 *
 * @retval true 成功
 * @retval false 失敗
 *
 */
bool DplControl::Start(StartMode& start_mode)
{
    if (isc_dpl_ == nullptr) {
        return false;
    }

    if (start_mode.enabled_stereo_matching) {
        // for (Block Matcing=ON Frame Decoder=ON)
        isc_start_mode_.isc_grab_start_mode.isc_grab_mode = IscGrabMode::kCorrect;

        isc_start_mode_.isc_grab_start_mode.isc_grab_color_mode = IscGrabColorMode::kColorOFF;
        if (start_mode.enabled_color) {
            isc_start_mode_.isc_grab_start_mode.isc_grab_color_mode = IscGrabColorMode::kColorON;
        }
        isc_start_mode_.isc_grab_start_mode.isc_get_mode.wait_time = 100;
        isc_start_mode_.isc_grab_start_mode.isc_get_raw_mode = IscGetModeRaw::kRawOn;
        if (start_mode.enabled_color) {
            isc_start_mode_.isc_grab_start_mode.isc_get_color_mode = IscGetModeColor::kAwb;
        }
        isc_start_mode_.isc_grab_start_mode.isc_record_mode = IscRecordMode::kRecordOff;
        if (start_mode.grab_record_mode) {
            isc_start_mode_.isc_grab_start_mode.isc_record_mode = IscRecordMode::kRecordOn;
        }

        isc_start_mode_.isc_grab_start_mode.isc_play_mode = IscPlayMode::kPlayOff;
        memset(isc_start_mode_.isc_grab_start_mode.isc_play_mode_parameter.play_file_name, 0, sizeof(isc_start_mode_.isc_grab_start_mode.isc_play_mode_parameter.play_file_name));
        if (start_mode.grab_play_mode) {
            isc_start_mode_.isc_grab_start_mode.isc_play_mode = IscPlayMode::kPlayOn;
            swprintf_s(isc_start_mode_.isc_grab_start_mode.isc_play_mode_parameter.play_file_name, L"%s", start_mode.play_file_name);
        }
        isc_start_mode_.isc_grab_start_mode.isc_play_mode_parameter.interval = 30;

        isc_start_mode_.isc_dataproc_start_mode.enabled_stereo_matching = true;
        isc_start_mode_.isc_dataproc_start_mode.enabled_frame_decoder = true;
        isc_start_mode_.isc_dataproc_start_mode.enabled_disparity_filter = start_mode.enabled_disparity_filter;
    }
    else {
        switch (start_mode.grab_mode) {
        case 0:
            isc_start_mode_.isc_grab_start_mode.isc_grab_mode = IscGrabMode::kParallax;
            break;
        case 1:
            isc_start_mode_.isc_grab_start_mode.isc_grab_mode = IscGrabMode::kCorrect;
            break;
        case 2:
            isc_start_mode_.isc_grab_start_mode.isc_grab_mode = IscGrabMode::kBeforeCorrect;
            break;
        default:
            isc_start_mode_.isc_grab_start_mode.isc_grab_mode = IscGrabMode::kParallax;
            break;
        }
    
        isc_start_mode_.isc_grab_start_mode.isc_grab_color_mode = IscGrabColorMode::kColorOFF;
        if (start_mode.enabled_color) {
            isc_start_mode_.isc_grab_start_mode.isc_grab_color_mode = IscGrabColorMode::kColorON;
        }
        isc_start_mode_.isc_grab_start_mode.isc_get_mode.wait_time = 100;

        isc_start_mode_.isc_grab_start_mode.isc_get_raw_mode = IscGetModeRaw::kRawOff;
        if (start_mode.enabled_stereo_matching || start_mode.enabled_disparity_filter) {
            isc_start_mode_.isc_grab_start_mode.isc_get_raw_mode = IscGetModeRaw::kRawOn;
        }

        if (start_mode.enabled_color) {
            isc_start_mode_.isc_grab_start_mode.isc_get_color_mode = IscGetModeColor::kAwb;
        }
        isc_start_mode_.isc_grab_start_mode.isc_record_mode = IscRecordMode::kRecordOff;
        if (start_mode.grab_record_mode) {
            isc_start_mode_.isc_grab_start_mode.isc_record_mode = IscRecordMode::kRecordOn;
        }

        isc_start_mode_.isc_grab_start_mode.isc_play_mode = IscPlayMode::kPlayOff;
        memset(isc_start_mode_.isc_grab_start_mode.isc_play_mode_parameter.play_file_name, 0, sizeof(isc_start_mode_.isc_grab_start_mode.isc_play_mode_parameter.play_file_name));
        if (start_mode.grab_play_mode) {
            isc_start_mode_.isc_grab_start_mode.isc_play_mode = IscPlayMode::kPlayOn;
            swprintf_s(isc_start_mode_.isc_grab_start_mode.isc_play_mode_parameter.play_file_name, L"%s", start_mode.play_file_name);
        }
        isc_start_mode_.isc_grab_start_mode.isc_play_mode_parameter.interval = 30;

        isc_start_mode_.isc_dataproc_start_mode.enabled_stereo_matching = false;
        isc_start_mode_.isc_dataproc_start_mode.enabled_disparity_filter = start_mode.enabled_disparity_filter;

        if (isc_start_mode_.isc_dataproc_start_mode.enabled_stereo_matching || isc_start_mode_.isc_dataproc_start_mode.enabled_disparity_filter) {
            isc_start_mode_.isc_dataproc_start_mode.enabled_frame_decoder = true;
        }
        else {
            isc_start_mode_.isc_dataproc_start_mode.enabled_frame_decoder = false;
        } 
    }

    DPL_RESULT dpl_result = isc_dpl_->Start(&isc_start_mode_);
    if (dpl_result == DPC_E_OK) {
        printf("[INFO]Start successfully\n");    
    }
    else {
        printf("[ERROR]Failed to Start\n");
        return false;
    }

    return true;
}

/**
 * 取り込みを停止する.
 *
 * @retval true 成功
 * @retval false 失敗
 *
 */
bool DplControl::Stop()
{
    if (isc_dpl_ == nullptr) {
        return false;
    }

    DPL_RESULT dpl_result = isc_dpl_->Stop();

    if (dpl_result == DPC_E_OK) {
        printf("[INFO]Stop successfully\n");    
    }
    else {
        printf("[ERROR]Failed to Stop\n");
        return false;
    }

    return true;
}

/**
 * カメラのデータを取得します.
 *
 * @param[out] isc_image_Info カメラデータ構造体
 *
 * @retval true 成功
 * @retval false 失敗
 *
 */
bool DplControl::GetCameraData(IscImageInfo* isc_image_Info)
{
    if (isc_dpl_ == nullptr) {
        return false;
    }
    
    DPL_RESULT dpl_result = isc_dpl_->GetCameraData(isc_image_Info);
	if (dpl_result != DPC_E_OK) {
		return false;
	}

    return true;
}

/**
 * 処理ライブラリのデータを取得します.
 *
 * @param[out] isc_data_proc_result_data データ構造体
 *
 * @retval true 成功
 * @retval false 失敗
 *
 */
bool DplControl::GetDataProcessingData(IscDataProcResultData* isc_data_proc_result_data)
{
    if (isc_dpl_ == nullptr) {
        return false;
    }
    
    DPL_RESULT dpl_result = isc_dpl_->GetDataProcModuleData(isc_data_proc_result_data);
	if (dpl_result != DPC_E_OK) {
		return false;
	}

    return true;
}

/**
 * カメラパラメータを取得します.
 *
 * @param[out] b 基線長
 * @param[out] bf カメラ固有パラメータ
 * @param[out] dinf カメラ固有パラメータ
 * @param[out] width データ幅
 * @param[out] height データ高さ
 *
 * @retval true 成功
 * @retval false 失敗
 *
 */
bool DplControl::GetCameraParameter(float* b, float* bf, float* dinf, int* width, int* height)
{
    if (isc_dpl_ == nullptr) {
        return false;
    }

    *b = 0;
    *bf = 0;
    *dinf = 0;
    *width = 0;
    *height = 0;

    if (isc_dpl_configuration_.enabled_camera) {
        DPL_RESULT ret = isc_dpl_->DeviceGetOption(IscCameraInfo::kBaseLength, b);
        if (ret != DPC_E_OK) {
            return false;
        }

        ret = isc_dpl_->DeviceGetOption(IscCameraInfo::kBF, bf);
        if (ret != DPC_E_OK) {
            return false;
        }

        ret = isc_dpl_->DeviceGetOption(IscCameraInfo::kDINF, dinf);
        if (ret != DPC_E_OK) {
            return false;
        }

        ret = isc_dpl_->DeviceGetOption(IscCameraInfo::kWidthMax, width);
        if (ret != DPC_E_OK) {
            return false;
        }

        ret = isc_dpl_->DeviceGetOption(IscCameraInfo::kHeightMax, height);
        if (ret != DPC_E_OK) {
            return false;
        }
    }
    else {
        // 暫定として指定カメラを確保する

        *b = camera_parameter_.b;
        *bf = camera_parameter_.bf;
        *dinf = camera_parameter_.dinf;
        
        switch (isc_dpl_configuration_.isc_camera_model) {
        case IscCameraModel::kVM:
            *width = 720;
            *height = 480;
            break;
        case IscCameraModel::kXC:
            *width = 1280;
            *height = 720;
            break;
        case IscCameraModel::k4K:
            *width = 3840;
            *height = 1920;
            break;
        case IscCameraModel::k4KA:
            *width = 3840;
            *height = 1920;
            break;
        case IscCameraModel::k4KJ:
            *width = 3840;
            *height = 1920;
            break;
        case IscCameraModel::kUnknown:
            *width = 3840;
            *height = 1920;
            break;
        default:
            *width = 3840;
            *height = 1920;
            break;
        }

    }

    return true;
}

/**
 * 保存されたデータファイルのヘッダー情報を取得します.
 *
 * @param[in] file_name データファイル名
 * @param[out] raw_file_headaer ヘッダーデータ
 *
 * @retval true 成功
 * @retval false 失敗
 *
 */
bool DplControl::GetFileInformation(wchar_t* file_name, IscRawFileHeader* raw_file_headaer)
{
    DPL_RESULT ret = isc_dpl_->GetFileInformation(file_name, raw_file_headaer);
    if (ret != DPC_E_OK) {
        return false;
    }

    return true;
}

/**
 * 設定されている描画の最小最大距離を取得する.
 *
 * @param[out] min_distance 最小距離
 * @param[out] max_distance 最大距離
 *
 * @retval 0 成功
 * @retval other 失敗
 *
 */
void DplControl::GetMinMaxDistance(double* min_distance, double* max_distance) const
{
    *min_distance = disp_color_map_distance_.min_value;
    *max_distance = disp_color_map_distance_.max_value;
}

/**
 * 入力された範囲で、Color LUTを再生成する.
 *
 * @param[out] min_distance 最小距離
 * @param[out] max_distance 最大距離
 *
 * @retval 0 成功
 * @retval other 失敗
 *
 */
void DplControl::RebuildDrawColorMap(const double min_distance, const double max_distance)
{
    delete[] disp_color_map_distance_.color_map;
    disp_color_map_distance_.color_map = nullptr;

    disp_color_map_distance_.min_value = min_distance;
    disp_color_map_distance_.max_value = max_distance;
    disp_color_map_distance_.color_map_size = 0;
    disp_color_map_distance_.color_map_step = 0.01;
    disp_color_map_distance_.color_map_size = (int)(max_distance / disp_color_map_distance_.color_map_step) + 1;
    disp_color_map_distance_.color_map = new int[disp_color_map_distance_.color_map_size + sizeof(int)];
    memset(disp_color_map_distance_.color_map, 0, disp_color_map_distance_.color_map_size + sizeof(int));
    BuildColorHeatMap(&disp_color_map_distance_);

    return;
}

/**
 * 視差データをColor画像へ変換します.
 *
 * @param[in] b 基線長
 * @param[in] angle カメラ設置角度
 * @param[in] bf カメラ固有パラメータ
 * @param[in] dinf カメラ固有パラメータ
 * @param[in] width データ幅
 * @param[in] height データ高さ
 * @param[in] depth 視差データ
 * @param[out] bgra_image Color画像
 *
 * @retval true 成功
 * @retval false 失敗
 *
 */
bool DplControl::ConvertDisparityToImage(double b, const double angle, const double bf, const double dinf, 
                                            const int width, const int height, float* depth, unsigned char* bgra_image)
{
    
    constexpr bool is_color_by_distance = true; // Always use distance data
    const bool is_draw_outside_bounds = is_draw_outside_bounds_;
    const double min_length = disp_color_map_distance_.min_value;
    const double max_length = disp_color_map_distance_.max_value;
    DispColorMap* disp_color_map = &disp_color_map_distance_;

    bool ret = MakeDepthColorImage( is_color_by_distance, is_draw_outside_bounds, min_length, max_length,
                                    disp_color_map, b, angle, bf, dinf,
                                    width, height, depth, bgra_image);

    return ret;
}

/**
 * Color Map用のLUTを作成します.
 *
 * @param[inout] disp_color_map Map用のパラメータ及びLUT
 *
 * @retval 0 成功
 * @retval other 失敗
 *
 */
int DplControl::BuildColorHeatMap(DispColorMap* disp_color_map)
{
    const double min_value = disp_color_map->min_value;
    const double max_value = disp_color_map->max_value;
    const double step = disp_color_map->color_map_step;
    int* color_map = disp_color_map->color_map;

    int start = 0;
    int end = (int)(max_value / step);
    double length = 0;

    int* p_color_map = color_map;

    for (int i = start; i <= end; i++) {

        int ro = 0, go = 0, bo = 0;
        ColorScaleBCGYR(min_value, max_value, length, &bo, &go, &ro);

        *p_color_map = (0xff000000) | (ro << 16) | (go << 8) | (bo);
        p_color_map++;

        length += step;
    }

    return 0;
}

/**
 * 視差範囲でColor LUTを作成する.
 *
 * @param[inout] disp_color_map 設定及びLUT
 *
 * @retval 0 成功
 * @retval other 失敗
 *
 */
int DplControl::BuildColorHeatMapForDisparity(DispColorMap* disp_color_map)
{
    const double min_value = disp_color_map->min_value;
    const double max_value = disp_color_map->max_value;
    const double step = disp_color_map->color_map_step;
    int* color_map = disp_color_map->color_map;

    int start = 0;
    int end = (int)(max_value / step);
    double length = 0;

    int* p_color_map = color_map;

    double* gamma_lut = new double[end + 1];
    memset(gamma_lut, 0, sizeof(double) * (end + 1));

    double gamma = 0.7;	// fix it, good for 4020
    for (int i = 0; i <= end; i++) {
        gamma_lut[i] = (int)(pow((double)i / 255.0, 1.0 / gamma) * 255.0);
    }

    for (int i = start; i <= end; i++) {

        int ro = 0, go = 0, bo = 0;
        double value = (double)(gamma_lut[(int)length]);

        ColorScaleBCGYR(min_value, max_value, value, &bo, &go, &ro);

        *p_color_map = (0xff000000) | (ro << 16) | (go << 8) | (bo);
        p_color_map++;

        length += step;
    }

    delete[] gamma_lut;

    return 0;
}

/**
 * 距離をRGBのグラデーションへ変換します.
 *
 * @param[in] min_value 最小
 * @param[in] max_value 最大
 * @param[in] in_value 入力
 * @param[out] bo 青
 * @param[out] go 緑
 * @param[out] ro 赤
 *
 * @retval true 成功
 * @retval false 失敗
 *
 */
int DplControl::ColorScaleBCGYR(const double min_value, const double max_value, const double in_value, int* bo, int* go, int* ro)
{
    int ret = 0;
    int r = 0, g = 0, b = 0;

    // 0.0～1.0 の範囲の値をサーモグラフィみたいな色にする
    // 0.0                    1.0
    // 青    水    緑    黄    赤
    // 最小値以下 = 青
    // 最大値以上 = 赤

    if (in_value <= min_value) {
        // red
        r = 255;
        g = 0;
        b = 0;
    }
    else if (in_value >= max_value) {
        // blue
        r = 0;
        g = 0;
        b = 255;
    }
    else {
        double temp_in_value = in_value - min_value;
        double range = max_value - min_value;

        double  value = 1.0 - (temp_in_value / range);
        double  tmp_val = cos(4 * 3.141592653589793 * value);
        int     col_val = (int)((-tmp_val / 2 + 0.5) * 255);

        if (value >= (4.0 / 4.0)) { r = 255;     g = 0;       b = 0; }		        // 赤
        else if (value >= (3.0 / 4.0)) { r = 255;     g = col_val; b = 0; }		    // 黄～赤
        else if (value >= (2.0 / 4.0)) { r = col_val; g = 255;     b = 0; }		    // 緑～黄
        else if (value >= (1.0 / 4.0)) { r = 0;       g = 255;     b = col_val; }	// 水～緑
        else if (value >= (0.0 / 4.0)) { r = 0;       g = col_val; b = 255; }		// 青～水
        else { r = 0;       g = 0;       b = 255; }		// 青
    }

    *bo = b;
    *go = g;
    *ro = r;

    return ret;
}

/**
 * 視差よりColor画像を作成します　色は、Color LUTに従います.
 *
 * @param[in] is_color_by_distance 距離のColor Mapを使用する
 * @param[in] is_draw_outside_bounds 距離範囲外を描画する
 * @param[in] min_length_i 描画最小距離
 * @param[in] max_length_i 描画最大距離
 * @param[in] disp_color_map Map用のColor LUT
 * @param[in] b_i 基線長
 * @param[in] angle_i カメラ設置角度
 * @param[in] bf_i カメラ固有パラメータ
 * @param[in] dinf_i カメラ固有パラメータ
 * @param[in] width 画像幅
 * @param[in] height 画像高さ
 * @param[in] depth 視差
 * @param[out] bgra_image Color画像
 *
 * @retval trye 成功
 * @retval false 失敗
 *
 */
bool DplControl::MakeDepthColorImage(   const bool is_color_by_distance, const bool is_draw_outside_bounds, const double min_length_i, const double max_length_i,
                                        DispColorMap* disp_color_map, double b_i, const double angle_i, const double bf_i, const double dinf_i,
                                        const int width, const int height, float* depth, unsigned char* bgra_image)
{
    if (disp_color_map == nullptr) {
        return false;
    }

    if (depth == nullptr) {
        return false;
    }

    if (bgra_image == nullptr) {
        return false;
    }

    constexpr double pi = 3.1415926535;
    const double bf = bf_i;
    const double b = b_i;
    const double dinf = dinf_i;
    const double rad = angle_i * pi / 180.0;
    const double color_map_step_mag = 1.0 / disp_color_map->color_map_step;

    if (is_color_by_distance) {
        // 距離変換

        for (int i = 0; i < height; i++) {
            float* src = depth + (i * width);
            unsigned char* dst = bgra_image + (i * width * 4);

            for (int j = 0; j < width; j++) {
                int r = 0, g = 0, b = 0;
                if (*src <= dinf) {
                    r = 0;
                    g = 0;
                    b = 0;
                }
                else {
                    double d = (*src - dinf);
                    double za = max_length_i;
                    if (d > 0) {
#if 0
                        double yh = (b * (i - (height / 2))) / d;
                        double z = bf / d;
                        za = -1 * yh * sin(rad) + z * cos(rad);
#else
                        za = bf / d;
#endif
                    }

                    if (is_draw_outside_bounds) {
                        int map_index = (int)(za * color_map_step_mag);
                        if (map_index >= 0 && map_index < disp_color_map->color_map_size) {
                            int map_value = disp_color_map->color_map[map_index];

                            r = (unsigned char)(map_value >> 16);
                            g = (unsigned char)(map_value >> 8);
                            b = (unsigned char)(map_value);
                        }
                        else {
                            // it's blue
                            r = 0;
                            g = 0;
                            b = 255;
                        }
                    }
                    else {
                        if (za > max_length_i) {
                            r = 0;
                            g = 0;
                            b = 0;
                        }
                        else if (za < min_length_i) {
                            r = 0;
                            g = 0;
                            b = 0;
                        }
                        else {
                            int map_index = (int)(za * color_map_step_mag);
                            if (map_index >= 0 && map_index < disp_color_map->color_map_size) {
                                int map_value = disp_color_map->color_map[map_index];

                                r = (unsigned char)(map_value >> 16);
                                g = (unsigned char)(map_value >> 8);
                                b = (unsigned char)(map_value);
                            }
                            else {
                                // it's black
                                r = 0;
                                g = 0;
                                b = 0;
                            }
                        }
                    }
                }

                *dst++ = b;
                *dst++ = g;
                *dst++ = r;
                *dst++ = 255;

                src++;
            }
        }
    }
    else {
        // 視差
        const double max_value = max_disparity_;
        const double dinf = dinf_i;

        for (int i = 0; i < height; i++) {
            float* src = depth + (i * width);
            unsigned char* dst = bgra_image + (i * width * 4);

            for (int j = 0; j < width; j++) {
                int r = 0, g = 0, b = 0;
                if (*src <= dinf) {
                    r = 0;
                    g = 0;
                    b = 0;
                }
                else {
                    double d = MAX(0, (max_value - *src - dinf));

                    int map_index = (int)(d * color_map_step_mag);
                    if (map_index >= 0 && map_index < disp_color_map->color_map_size) {
                        int map_value = disp_color_map->color_map[map_index];

                        r = (unsigned char)(map_value >> 16);
                        g = (unsigned char)(map_value >> 8);
                        b = (unsigned char)(map_value);
                    }
                    else {
                        // it's black
                        r = 0;
                        g = 0;
                        b = 0;
                    }
                }

                *dst++ = b;
                *dst++ = g;
                *dst++ = r;
                *dst++ = 255;

                src++;
            }
        }
    }

    return true;
}
