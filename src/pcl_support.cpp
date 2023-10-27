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
 * @file pcl_support.cpp
 * @brief Provides display functionality using Point Cloud Library.
 * @author Takayuki
 * @date 2023.10.30
 * @version 0.1
 * 
 * @details Create a point cloud from the parallax and filter and display it using the Point cloud Library functionality.
 */

#include <mutex>
#include <iostream>
#include <thread>
#include <boost/thread.hpp>
#include <pcl/common/angles.h> // for pcl::deg2rad
#include <pcl/features/normal_3d.h>
#include <pcl/io/pcd_io.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <pcl/console/parse.h>
#include <pcl/visualization/cloud_viewer.h>
#include <boost/make_shared.hpp>
#include <pcl/filters/passthrough.h>
#include <pcl/filters/radius_outlier_removal.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/surface/mls.h>
#include <pcl/segmentation/sac_segmentation.h>

#include "opencv2/opencv.hpp"

#include "isc_dpl_error_def.h"
#include "isc_dpl_def.h"
#include "isc_dpl.h"
#include "dpl_controll.h"
#include "dpl_support.h"

#include "pcl_def.h"
#include "pcl_data_ring_buffer.h"

#include "pcl_support.h"

/** @enum  OperationStatus
 *  @brief 表示動作の状態
 */
enum class OperationStatus {
	idle,
	active
};

/** @struct  PickData
 *  @brief Mouseによる選択データ
 */
struct PickData {
	bool valid;
	float x, y, z;
};

/** @struct  PickInforamtion
 *  @brief Mouseによる選択データ
 */
struct PickInforamtion {
	int max_count;
	int count;
	PickData pick_data[4];
};

/** @struct  PclVizControl
 *  @brief 表示動作の制御用構造体
 */
struct PclVizControl {
	// parameter
	VizParameters viz_parameters;

	// data ring buffer
	PclDataRingBuffer* pcl_data_ring_buffer;

	// point cloud for draw
	pcl::PointCloud<pcl::PointXYZRGBA>::Ptr cloud;

	// Operation status
	OperationStatus operation_status;

	// Thread Control
	HANDLE handle_semaphore_pcl_build;
	HANDLE handle_semaphore_pcl_draw;
	CRITICAL_SECTION	threads_critical;

	struct ThreadControl {
		HANDLE thread_handle;
		;
		int terminate_request;
		int terminate_done;
		int end_code;
		bool stop_request;
	};
	ThreadControl thread_control_build_pcl;
	ThreadControl thread_control_draw;

	// pick control/information
	CRITICAL_SECTION pick_callback_critical;
	PickInforamtion pick_information;

	// ketbord call back 
	CRITICAL_SECTION kbd_callback_critical;

};
PclVizControl pcl_viz_control_ = {};	/**< 表示Threadへ渡すデータ */

/** @struct  CallbackArgs
 *  @brief マウスイベントのCallback用データ構造体
 */
struct CallbackArgs {
	// structure used to pass arguments to the callback function
	pcl::visualization::PCLVisualizer::Ptr viewer_ptr;
	PclVizControl* pcl_viz_control;
};

// 
// functions
//
unsigned __stdcall BuildPCLThread(void* context);

unsigned __stdcall VisualizerThread(void* context);

void BuildPointCloud(	const int width, const int height, const double d_inf, const double base_length, const double bf, const double min_distance, const double max_distance,
						cv::Mat& base_image, cv::Mat& depth_data,
						pcl::PointCloud<pcl::PointXYZRGBA>::Ptr cloud);

int PathThroughFilter(const std::string field_name, const double min_length, const double max_length, pcl::PointCloud<pcl::PointXYZRGBA>::Ptr cloud, pcl::PointCloud<pcl::PointXYZRGBA>::Ptr filtered_cloud);

int RadiusOutlierRemoval(const double radius_search, const int min_neighbors_in_radius, pcl::PointCloud<pcl::PointXYZRGBA>::Ptr cloud, pcl::PointCloud<pcl::PointXYZRGBA>::Ptr filtered_cloud);

int DownSampling(const double boxel_size, pcl::PointCloud<pcl::PointXYZRGBA>::Ptr cloud, pcl::PointCloud<pcl::PointXYZRGBA>::Ptr filtered_cloud);

int PlaneDetection(double threshold, pcl::PointCloud<pcl::PointXYZRGBA>::Ptr cloud);

int WritePclToFile(char* write_file_name, pcl::PointCloud<pcl::PointXYZRGBA>::Ptr cloud);

/**
 * 初期化します.
 *
 * @param[in] init_viz_parameters 表示用パラメータ
 * @retval 0 成功
 * @retval other 失敗
 */
int InitializePclViz(const VizParameters* init_viz_parameters)
{
	// setup parameters
	PclVizControl* pcl_viz_control = &pcl_viz_control_;

	pcl_viz_control->viz_parameters.viz_position_x			= init_viz_parameters->viz_position_x;
	pcl_viz_control->viz_parameters.viz_position_y			= init_viz_parameters->viz_position_y;
	pcl_viz_control->viz_parameters.viz_width				= init_viz_parameters->viz_width;
	pcl_viz_control->viz_parameters.viz_height				= init_viz_parameters->viz_height;
	pcl_viz_control->viz_parameters.width					= init_viz_parameters->width;
	pcl_viz_control->viz_parameters.height					= init_viz_parameters->height;
	pcl_viz_control->viz_parameters.d_inf					= init_viz_parameters->d_inf;
	pcl_viz_control->viz_parameters.base_length				= init_viz_parameters->base_length;
	pcl_viz_control->viz_parameters.bf						= init_viz_parameters->bf;
	pcl_viz_control->viz_parameters.min_distance			= init_viz_parameters->min_distance;
	pcl_viz_control->viz_parameters.max_distance			= init_viz_parameters->max_distance;
	pcl_viz_control->viz_parameters.coordinate_system		= init_viz_parameters->coordinate_system;
	pcl_viz_control->viz_parameters.full_screen_request		= init_viz_parameters->full_screen_request;
	pcl_viz_control->viz_parameters.restore_screen_request	= init_viz_parameters->restore_screen_request;

	sprintf(pcl_viz_control->viz_parameters.pcd_file_write_folder, "%s", init_viz_parameters->pcd_file_write_folder);

	pcl_viz_control->operation_status = OperationStatus::idle;

	pcl_viz_control->thread_control_build_pcl.thread_handle = NULL;
	pcl_viz_control->thread_control_build_pcl.terminate_request = 0;
	pcl_viz_control->thread_control_build_pcl.terminate_done = 0;
	pcl_viz_control->thread_control_build_pcl.stop_request = false;
	pcl_viz_control->thread_control_build_pcl.end_code = 0;

	pcl_viz_control->thread_control_draw.thread_handle = NULL;
	pcl_viz_control->thread_control_draw.terminate_request = 0;
	pcl_viz_control->thread_control_draw.terminate_done = 0;
	pcl_viz_control->thread_control_draw.stop_request = false;
	pcl_viz_control->thread_control_draw.end_code = 0;

	pcl_viz_control->pick_information.max_count = 4;
	pcl_viz_control->pick_information.count = 0;
	for (int i= 0; i < pcl_viz_control->pick_information.max_count; i++) {
		pcl_viz_control->pick_information.pick_data[i].valid = false;
		pcl_viz_control->pick_information.pick_data[i].x = 0.0F;
		pcl_viz_control->pick_information.pick_data[i].y = 0.0F;
		pcl_viz_control->pick_information.pick_data[i].z = 0.0F;
	}

	// buffers
	pcl_viz_control->pcl_data_ring_buffer = new PclDataRingBuffer;
	pcl_viz_control->pcl_data_ring_buffer->Initialize(true, true, 4, pcl_viz_control->viz_parameters.width, pcl_viz_control->viz_parameters.height);

	// flags
	char semaphoreName[64] = {};

	sprintf_s(semaphoreName, "THREAD_SEMAPHORENAME_pb_%d", 0);
	pcl_viz_control->handle_semaphore_pcl_build = CreateSemaphoreA(NULL, 0, 1, semaphoreName);
	if (pcl_viz_control->handle_semaphore_pcl_build == NULL) {
		// Fail
		return 0;
	}

	sprintf_s(semaphoreName, "THREAD_SEMAPHORENAME_bd_U_%d", 0);
	pcl_viz_control->handle_semaphore_pcl_draw = CreateSemaphoreA(NULL, 0, 1, semaphoreName);
	if (pcl_viz_control->handle_semaphore_pcl_draw == NULL) {
		// Fail
		return 0;
	}

	InitializeCriticalSection(&pcl_viz_control->threads_critical);
	InitializeCriticalSection(&pcl_viz_control->pick_callback_critical);
	InitializeCriticalSection(&pcl_viz_control->kbd_callback_critical);

	return 0;
}

/**
 * 終了処理をします.
 *
 */
int TerminatePclViz()
{
	// ended
	PclVizControl* pcl_viz_control = &pcl_viz_control_;

	// stop draw
	if (pcl_viz_control->thread_control_draw.thread_handle != NULL) {
		pcl_viz_control->thread_control_draw.stop_request = true;
		pcl_viz_control->thread_control_draw.terminate_done = 0;
		pcl_viz_control->thread_control_draw.end_code = 0;
		pcl_viz_control->thread_control_draw.terminate_request = 1;

		int count = 0;
		while (pcl_viz_control->thread_control_draw.terminate_done == 0) {
			if (count > 200) {
				break;
			}
			count++;
			Sleep(10L);
		}

		if (pcl_viz_control->thread_control_draw.thread_handle != NULL) {
			CloseHandle(pcl_viz_control->thread_control_draw.thread_handle);
			pcl_viz_control->thread_control_draw.thread_handle = NULL;
		}
	}

	// stop build
	if (pcl_viz_control->thread_control_build_pcl.thread_handle != NULL) {
		pcl_viz_control->thread_control_build_pcl.stop_request = true;
		pcl_viz_control->thread_control_build_pcl.terminate_done = 0;
		pcl_viz_control->thread_control_build_pcl.end_code = 0;
		pcl_viz_control->thread_control_build_pcl.terminate_request = 1;

		int count = 0;
		while (pcl_viz_control->thread_control_build_pcl.terminate_done == 0) {
			if (count > 200) {
				break;
			}
			count++;
			Sleep(10L);
		}

		if (pcl_viz_control->thread_control_build_pcl.thread_handle != NULL) {
			CloseHandle(pcl_viz_control->thread_control_build_pcl.thread_handle);
			pcl_viz_control->thread_control_build_pcl.thread_handle = NULL;
		}
	}

	// delete flags
	DeleteCriticalSection(&pcl_viz_control->threads_critical);
	DeleteCriticalSection(&pcl_viz_control->pick_callback_critical);
	DeleteCriticalSection(&pcl_viz_control->kbd_callback_critical);

	if (pcl_viz_control->handle_semaphore_pcl_draw != NULL) {
		CloseHandle(pcl_viz_control->handle_semaphore_pcl_draw);
		pcl_viz_control->handle_semaphore_pcl_draw = NULL;
	}
		
	if (pcl_viz_control->handle_semaphore_pcl_build != NULL) {
		CloseHandle(pcl_viz_control->handle_semaphore_pcl_build);
		pcl_viz_control->handle_semaphore_pcl_build = NULL;
	}

	// deletebuffer
	pcl_viz_control->pcl_data_ring_buffer->Terminate();
	delete pcl_viz_control->pcl_data_ring_buffer;
	pcl_viz_control->pcl_data_ring_buffer = nullptr;

	return 0;
}

/**
 * 表示Threadを開始し、データ受付可能とします.
 *
 * @retval 0 成功
 * @retval other 失敗
 *
 */
int StartPclViz()
{
	PclVizControl* pcl_viz_control = &pcl_viz_control_;

	// start buildl thread
	if (pcl_viz_control->thread_control_build_pcl.thread_handle != NULL) {
		return -1;
	}
	pcl_viz_control->thread_control_build_pcl.thread_handle = NULL;
	pcl_viz_control->thread_control_build_pcl.terminate_request = 0;
	pcl_viz_control->thread_control_build_pcl.terminate_done = 0;
	pcl_viz_control->thread_control_build_pcl.stop_request = false;
	pcl_viz_control->thread_control_build_pcl.end_code = 0;

	if ((pcl_viz_control->thread_control_build_pcl.thread_handle = (HANDLE)_beginthreadex(0, 0, BuildPCLThread, (void*)pcl_viz_control, 0, 0)) == 0) {
		// Fail
		return 0;
	}
	SetThreadPriority(pcl_viz_control->thread_control_build_pcl.thread_handle, THREAD_PRIORITY_NORMAL);

	// start visual thread
	if (pcl_viz_control->thread_control_draw.thread_handle != NULL) {
		return -1;
	}
	pcl_viz_control->thread_control_draw.thread_handle = NULL;
	pcl_viz_control->thread_control_draw.terminate_request = 0;
	pcl_viz_control->thread_control_draw.terminate_done = 0;
	pcl_viz_control->thread_control_draw.stop_request = false;
	pcl_viz_control->thread_control_draw.end_code = 0;

	if ((pcl_viz_control->thread_control_draw.thread_handle = (HANDLE)_beginthreadex(0, 0, VisualizerThread, (void*)pcl_viz_control, 0, 0)) == 0) {
		// Fail
		return 0;
	}
	SetThreadPriority(pcl_viz_control->thread_control_draw.thread_handle, THREAD_PRIORITY_NORMAL);

	// clear buufer
	pcl_viz_control->pick_information.count = 0;
	for (int i = 0; i < 4; i++) {
		pcl_viz_control->pick_information.pick_data[i].valid = false;
		pcl_viz_control->pick_information.pick_data[i].x = 0.0F;
		pcl_viz_control->pick_information.pick_data[i].y = 0.0F;
		pcl_viz_control->pick_information.pick_data[i].z = 0.0F;
	}

	pcl_viz_control->pcl_data_ring_buffer->Clear();

	pcl_viz_control->operation_status = OperationStatus::active;

	return 0;
}

/**
 * 表示Threadを停止します.
 *
 * @retval 0 成功
 * @retval other 失敗
 *
 */
int StopPclViz() 
{
	PclVizControl* pcl_viz_control = &pcl_viz_control_;

	pcl_viz_control->operation_status = OperationStatus::idle;

	// stop draw
	if (pcl_viz_control->thread_control_draw.thread_handle != NULL) {
		pcl_viz_control->thread_control_draw.stop_request = true;
		pcl_viz_control->thread_control_draw.terminate_done = 0;
		pcl_viz_control->thread_control_draw.end_code = 0;
		pcl_viz_control->thread_control_draw.terminate_request = 1;

		int count = 0;
		while (pcl_viz_control->thread_control_draw.terminate_done == 0) {
			if (count > 200) {
				printf("[ERROR]Stop VisualizerThread! count > 200\n");
				break;
			}
			count++;
			Sleep(10L);
		}

		if (pcl_viz_control->thread_control_draw.thread_handle != NULL) {
			CloseHandle(pcl_viz_control->thread_control_draw.thread_handle);
			pcl_viz_control->thread_control_draw.thread_handle = NULL;
		}
	}

	// stop build
	if (pcl_viz_control->thread_control_build_pcl.thread_handle != NULL) {
		pcl_viz_control->thread_control_build_pcl.stop_request = true;
		pcl_viz_control->thread_control_build_pcl.terminate_done = 0;
		pcl_viz_control->thread_control_build_pcl.end_code = 0;
		pcl_viz_control->thread_control_build_pcl.terminate_request = 1;

		int count = 0;
		while (pcl_viz_control->thread_control_build_pcl.terminate_done == 0) {
			if (count > 200) {
				printf("[ERROR]Stop BuildPCLThread! count > 200\n");
				break;
			}
			count++;
			Sleep(10L);
		}

		if (pcl_viz_control->thread_control_build_pcl.thread_handle != NULL) {
			CloseHandle(pcl_viz_control->thread_control_build_pcl.thread_handle);
			pcl_viz_control->thread_control_build_pcl.thread_handle = NULL;
		}
	}

	pcl_viz_control->pick_information.count = 0;
	for (int i = 0; i < 4; i++) {
		pcl_viz_control->pick_information.pick_data[i].valid = false;
		pcl_viz_control->pick_information.pick_data[i].x = 0.0F;
		pcl_viz_control->pick_information.pick_data[i].y = 0.0F;
		pcl_viz_control->pick_information.pick_data[i].z = 0.0F;
	}

	return 0;
}

/**
 * 表示用データを入力します.
 *
 * @param[in] input_args 表示用データ
 * @param[out] output_args 表示処理からの出力データ
 *
 * @retval 0 成功
 * @retval other 失敗
 *
 */
int RunPclViz(PclVizInputArgs* input_args, PclVizOutputArgs* output_args)
{
	PclDataRingBuffer::BufferData* buffer_data = nullptr;
	const ULONGLONG time = GetTickCount64();

	PclVizControl* pcl_viz_control = &pcl_viz_control_;

	if (pcl_viz_control->operation_status != OperationStatus::active) {
		return 0;
	}

	int put_index = pcl_viz_control->pcl_data_ring_buffer->GetPutBuffer(&buffer_data, time);
	int image_status = 0;

	if (put_index >= 0 && buffer_data != nullptr) {
		pcl_viz_control->viz_parameters.base_length		= input_args->base_length;
		pcl_viz_control->viz_parameters.d_inf			= input_args->d_inf;
		pcl_viz_control->viz_parameters.bf				= input_args->bf;

		buffer_data->pcl_data.width						= input_args->width;
		buffer_data->pcl_data.height					= input_args->height;
		buffer_data->pcl_data.base_image_channel_count	= input_args->base_image_channel_count;

		size_t cp_size = input_args->width * input_args->height * input_args->base_image_channel_count;
		memcpy(buffer_data->pcl_data.image, input_args->image, cp_size);

		buffer_data->pcl_data.depth_width	= input_args->width;
		buffer_data->pcl_data.depth_height	= input_args->height;

		cp_size = input_args->width * input_args->height * sizeof(float);
		memcpy(buffer_data->pcl_data.disparity_data, input_args->disparity_data, cp_size);

		cp_size = input_args->width * input_args->height * 4;
		memcpy(buffer_data->pcl_data.disparity_image_bgra, input_args->disparity_image_bgra, cp_size);

		// parameter
		buffer_data->pcl_filter_parameter.enabled_remove_nan							= input_args->pcl_filter_parameter.enabled_remove_nan;
		buffer_data->pcl_filter_parameter.enabled_pass_through_filter					= input_args->pcl_filter_parameter.enabled_pass_through_filter;
		buffer_data->pcl_filter_parameter.pass_through_filter_range.min					= input_args->pcl_filter_parameter.pass_through_filter_range.min;
		buffer_data->pcl_filter_parameter.pass_through_filter_range.max					= input_args->pcl_filter_parameter.pass_through_filter_range.max;
		buffer_data->pcl_filter_parameter.enabled_down_sampling							= input_args->pcl_filter_parameter.enabled_down_sampling;
		buffer_data->pcl_filter_parameter.down_sampling_boxel_size						= input_args->pcl_filter_parameter.down_sampling_boxel_size;
		buffer_data->pcl_filter_parameter.enabled_radius_outlier_removal				= input_args->pcl_filter_parameter.enabled_radius_outlier_removal;
		buffer_data->pcl_filter_parameter.radius_outlier_removal_param.radius_search	= input_args->pcl_filter_parameter.radius_outlier_removal_param.radius_search;
		buffer_data->pcl_filter_parameter.radius_outlier_removal_param.min_neighbors	= input_args->pcl_filter_parameter.radius_outlier_removal_param.min_neighbors;
		buffer_data->pcl_filter_parameter.enabled_plane_detection						= input_args->pcl_filter_parameter.enabled_plane_detection;
		buffer_data->pcl_filter_parameter.plane_detection_threshold						= input_args->pcl_filter_parameter.plane_detection_threshold;

		// OK
		image_status = 1;
	}
	pcl_viz_control->pcl_data_ring_buffer->DonePutBuffer(put_index, image_status);

	// screen control
	// Immediately Execute
	EnterCriticalSection(&pcl_viz_control->threads_critical);

	if (input_args->full_screen_request) {
		pcl_viz_control->viz_parameters.full_screen_request = true;
	}

	if (input_args->restore_screen_request) {
		pcl_viz_control->viz_parameters.full_screen_request = false;
		pcl_viz_control->viz_parameters.restore_screen_request = true;
	}

	LeaveCriticalSection(&pcl_viz_control->threads_critical);

	// mouse pick information
	EnterCriticalSection(&pcl_viz_control->pick_callback_critical);

	if (pcl_viz_control->pick_information.count > 0) {
		// it copy last one
		int index = MAX(0, pcl_viz_control->pick_information.count - 1);
		if (pcl_viz_control->pick_information.pick_data[index].valid) {
			output_args->pick_information.count = 1;
			output_args->pick_information.pick_data[0].x = pcl_viz_control->pick_information.pick_data[index].x;
			output_args->pick_information.pick_data[0].y = pcl_viz_control->pick_information.pick_data[index].y;
			output_args->pick_information.pick_data[0].z = pcl_viz_control->pick_information.pick_data[index].z;
			output_args->pick_information.pick_data[0].valid = true;
		}

		pcl_viz_control->pick_information.count = 0;
		pcl_viz_control->pick_information.pick_data[index].valid = false;
	}

	LeaveCriticalSection(&pcl_viz_control->pick_callback_critical);

	return 0;
}

/**
 * 視差データより点群(Point Cloud)を作成し、Filter処理を行います.
 *
 * @param[inout] context (pcl_viz_control)表示データ授受用の構造体
 *
 * @retval 0 成功
 * @retval other 失敗
 *
 */
unsigned __stdcall BuildPCLThread(void* context)
{
	// build point cloud thread
	PclVizControl* pcl_viz_control = (PclVizControl*)context;

	if (pcl_viz_control == nullptr) {
		return -1;
	}

	while (pcl_viz_control->thread_control_build_pcl.terminate_request < 1) {

		for (;;) {
			if (pcl_viz_control->thread_control_build_pcl.stop_request) {
				pcl_viz_control->thread_control_build_pcl.stop_request = false;
				break;
			}

			// get data
			PclDataRingBuffer::BufferData* buffer_data = nullptr;
			ULONGLONG time = 0;
			int get_index = pcl_viz_control->pcl_data_ring_buffer->GetGetBuffer(&buffer_data, &time);

			if (get_index >= 0) {
				// build pcl data
				// build point cloud
				double display_scale = 1.0;

				// Base Image
				cv::Mat mat_data_proc_image_scale_flip;
				{
					const int width						= buffer_data->pcl_data.width;
					const int height					= buffer_data->pcl_data.height;
					const int base_image_channel_count	= buffer_data->pcl_data.base_image_channel_count;
					unsigned char* image				= buffer_data->pcl_data.image;

					if (base_image_channel_count == 3) {
						// color image
						cv::Mat mat_base_image(height, width, CV_8UC3, image);

						double ratio = 1.0 / (double)display_scale;
						cv::Mat mat_base_image_scale;
						cv::resize(mat_base_image, mat_base_image_scale, cv::Size(), ratio, ratio, cv::INTER_NEAREST);
						cv::flip(mat_base_image_scale, mat_data_proc_image_scale_flip, -1);
					}
					else if (base_image_channel_count == 4) {
						// color image
						cv::Mat mat_base_image(height, width, CV_8UC4, image);

						double ratio = 1.0 / (double)display_scale;
						cv::Mat mat_base_image_scale;
						cv::resize(mat_base_image, mat_base_image_scale, cv::Size(), ratio, ratio, cv::INTER_NEAREST);
						cv::flip(mat_base_image_scale, mat_data_proc_image_scale_flip, -1);
					}
					else {
						// base image
						cv::Mat mat_base_image(height, width, CV_8U, image);

						double ratio = 1.0 / (double)display_scale;
						cv::Mat mat_base_image_scale;
						cv::resize(mat_base_image, mat_base_image_scale, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

						cv::Mat mat_base_image_scale_flip_temp;
						cv::flip(mat_base_image_scale, mat_base_image_scale_flip_temp, -1);

						cv::Mat mat_base_image_color;
						cv::cvtColor(mat_base_image_scale_flip_temp, mat_data_proc_image_scale_flip, cv::COLOR_GRAY2RGB);
					}

					//cv::imshow("Base Image", mat_data_proc_image_scale_flip);
				}

				// depth
				cv::Mat mat_depth_scale_flip;
				{
					const int depth_width	= buffer_data->pcl_data.depth_width;
					const int depth_height	= buffer_data->pcl_data.depth_height;
					float* depth			= buffer_data->pcl_data.disparity_data;

					cv::Mat mat_depth(depth_height, depth_width, CV_32F, depth);

					double ratio = 1.0 / (double)display_scale;
					cv::Mat mat_depth_scale;
					cv::resize(mat_depth, mat_depth_scale, cv::Size(), ratio, ratio, cv::INTER_NEAREST);

					cv::flip(mat_depth_scale, mat_depth_scale_flip, -1);
				}

				// CloudViewer に与える PointCloud 
				//pcl::PointCloud<pcl::PointXYZRGBA>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZRGBA>);

				// parameters
				VizParameters* viz_parameters = &pcl_viz_control->viz_parameters;

				// draw PCL
				{
					if (mat_data_proc_image_scale_flip.empty()) {
						return 0;
					}

					if (mat_depth_scale_flip.empty()) {
						return 0;
					}

					// CloudViewer に与える PointCloud 
					pcl::PointCloud<pcl::PointXYZRGBA>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZRGBA>);
					//NaNを含む可能性があるならfalseにする。基本はfalseでよい
					cloud->is_dense = false;

					const int width = mat_data_proc_image_scale_flip.cols;
					const int height = mat_data_proc_image_scale_flip.rows;

					BuildPointCloud(
						width,
						height,
						viz_parameters->d_inf,
						viz_parameters->base_length,
						viz_parameters->bf,
						viz_parameters->min_distance,
						viz_parameters->max_distance,
						mat_data_proc_image_scale_flip,
						mat_depth_scale_flip,
						cloud);

					// filter
					PclFilterParameter* pcl_filter_parameter = &buffer_data->pcl_filter_parameter;

					bool remove_nan				= pcl_filter_parameter->enabled_remove_nan;
					bool path_through_filter	= pcl_filter_parameter->enabled_pass_through_filter;
					bool down_sampling			= pcl_filter_parameter->enabled_down_sampling;
					bool radius_outlier_removal	= pcl_filter_parameter->enabled_radius_outlier_removal;
					bool plane_detection		= pcl_filter_parameter->enabled_plane_detection;

					if (remove_nan) {
						// The mapping tells you to what points of the old cloud the new ones correspond,
						// but we will not use it.
						// このmappingにより元のクラウドのどのポイントが新しいクラウドのどこに写像されたかわかるが、
						// これは使わない

						pcl::PointCloud<pcl::PointXYZRGBA>::Ptr temp_filtered_cloud(new pcl::PointCloud<pcl::PointXYZRGBA>);

						std::vector<int> mapping;
						pcl::removeNaNFromPointCloud(*cloud, *temp_filtered_cloud, mapping);

						cloud = std::move(temp_filtered_cloud);
					}

					if (path_through_filter) {
						const double min_length = pcl_filter_parameter->pass_through_filter_range.min;	
						const double max_length = pcl_filter_parameter->pass_through_filter_range.max;	

						pcl::PointCloud<pcl::PointXYZRGBA>::Ptr temp_filtered_cloud(new pcl::PointCloud<pcl::PointXYZRGBA>);

						int ret = PathThroughFilter("z", min_length, max_length, cloud, temp_filtered_cloud);
						//int ret = PathThroughFilter("y", min_length, max_length, cloud, temp_filtered_cloud);
						//int ret = PathThroughFilter("x", min_length, max_length, cloud, temp_filtered_cloud);

						cloud = std::move(temp_filtered_cloud);
					}

					if (down_sampling) {
						const double boxel_size = pcl_filter_parameter->down_sampling_boxel_size;	//0.1;// 0.01f;

						pcl::PointCloud<pcl::PointXYZRGBA>::Ptr temp_filtered_cloud(new pcl::PointCloud<pcl::PointXYZRGBA>);

						int ret = DownSampling(boxel_size, cloud, temp_filtered_cloud);

						cloud = std::move(temp_filtered_cloud);
					}

					if (radius_outlier_removal) {
						const double radius_search			= pcl_filter_parameter->radius_outlier_removal_param.radius_search;			// 0.01;// 0.15;
						const int min_neighbors_in_radius	= pcl_filter_parameter->radius_outlier_removal_param.min_neighbors;	// 100;

						pcl::PointCloud<pcl::PointXYZRGBA>::Ptr temp_filtered_cloud(new pcl::PointCloud<pcl::PointXYZRGBA>);

						int ret = RadiusOutlierRemoval(radius_search, min_neighbors_in_radius, cloud, temp_filtered_cloud);

						cloud = std::move(temp_filtered_cloud);
					}

					if (plane_detection) {
						double threshold = pcl_filter_parameter->plane_detection_threshold;	//  0.2;
						int ret = PlaneDetection(threshold, cloud);
					}

					// set draw data
					PclVizControl* pcl_viz_control = &pcl_viz_control_;

					EnterCriticalSection(&pcl_viz_control->threads_critical);
					pcl_viz_control->cloud = cloud;
					ReleaseSemaphore(pcl_viz_control->handle_semaphore_pcl_draw, 1, NULL);

					LeaveCriticalSection(&pcl_viz_control->threads_critical);
				}

				// done
				pcl_viz_control->pcl_data_ring_buffer->DoneGetBuffer(get_index);
			}
			else {
				Sleep(16);
			}
		}// for (;;) {

	}// while (thread_control_camera_.terminate_request < 1) {

	pcl_viz_control->thread_control_build_pcl.terminate_done = 1;

	return 0;
}

/**
 * Vizulizer表示上でのMouse ClickのCallback.
 *
 * @param[in] event マウスのPickイベント情報
 * @param[out] args cloudデータ
 *
 * @retval 0 成功
 * @retval other 失敗
 *
 */
static void PointPickCallback(const pcl::visualization::PointPickingEvent& event, void* args)
{
	/*
		正しくPickできないため、暫定的にこの機能を未使用とします

	*/

	return;

	// get 3D information
	int idx = event.getPointIndex();
	if (idx == -1)
		return;

	float x = 0.0F, y = 0.0F, z = 0.0F;
	event.getPoint(x, y, z);

	struct CallbackArgs* cb_args = nullptr;

	if (args != nullptr) {
		cb_args = (struct CallbackArgs*)args;

		EnterCriticalSection(&cb_args->pcl_viz_control->pick_callback_critical);

		if (0) {
			pcl::search::KdTree<pcl::PointXYZRGBA> search;
			search.setInputCloud(cb_args->pcl_viz_control->cloud);
			// Return the correct index in the cloud instead of the index on the screen
			pcl::Indices indices(1);
			std::vector<float> distances(1);

			// Because VTK/OpenGL stores data without NaN, we lose the 1-1 correspondence, so we must search for the real point
			pcl::PointXYZRGBA picked_pt;
			event.getPoint(picked_pt.x, picked_pt.y, picked_pt.z);
			search.nearestKSearch(picked_pt, 1, indices, distances);

			PCL_INFO("Point index picked: %d (real: %d) - [%f, %f, %f]\n", idx, indices[0], picked_pt.x, picked_pt.y, picked_pt.z);
		}

		int index = 0;
		cb_args->pcl_viz_control->pick_information.pick_data[index].x = x;
		cb_args->pcl_viz_control->pick_information.pick_data[index].y = y;
		cb_args->pcl_viz_control->pick_information.pick_data[index].z = z;
		cb_args->pcl_viz_control->pick_information.pick_data[index].valid = true;

		cb_args->pcl_viz_control->pick_information.count = 1;

		LeaveCriticalSection(&cb_args->pcl_viz_control->pick_callback_critical);
	}

	// debug
	if (false) {
		printf("[INFO][DEBUG]Clicked 3D point=x:%f, y:%f, z:%f\n", x, y, z);
		printf("\n");

		if (cb_args != nullptr) {
			pcl::visualization::Camera camera_info;
			cb_args->viewer_ptr->getCameraParameters(camera_info);

			printf("[INFO][DEBUG]camera pos:(%f,%f,%f)\n", camera_info.pos[0], camera_info.pos[1], camera_info.pos[2]);
			printf("[INFO][DEBUG]camera view:(%f,%f,%f)\n", camera_info.view[0], camera_info.view[1], camera_info.view[2]);
			printf("[INFO][DEBUG]camera focal:(%f,%f,%f)\n", camera_info.focal[0], camera_info.focal[1], camera_info.focal[2]);
			printf("\n");
		}
	}

	return;
}

/**
 * Vizulizer表示上でのkeyboard ClickのCallback.
 *
 * @param[in] event マウスのPickイベント情報
 * @param[out] args cloudデータ
 *
 * @retval 0 成功
 * @retval other 失敗
 *
 */
static void KeyboardEventCallback(const pcl::visualization::KeyboardEvent& event, void* args)
{

	if (event.getKeySym() == "n" && event.keyDown()) {
		struct CallbackArgs* cb_args = nullptr;

		if (args != nullptr) {
			cb_args = (struct CallbackArgs*)args;

			EnterCriticalSection(&cb_args->pcl_viz_control->threads_critical);
			pcl::PointCloud<pcl::PointXYZRGBA>::Ptr deep_copy(new pcl::PointCloud<pcl::PointXYZRGBA>(*cb_args->pcl_viz_control->cloud));
			LeaveCriticalSection(&cb_args->pcl_viz_control->threads_critical);

			int ret = WritePclToFile(cb_args->pcl_viz_control->viz_parameters.pcd_file_write_folder, deep_copy);
			
			// debug
			if (false) {
				printf("[INFO][DEBUG]KeyboardEventCallback\n");
				printf("\n");
			}
		}
	}

	return;
}

/**
 * Vizulizer表示Thread
 *
 * @param[in] context (pcl_viz_control)表示データ
 *
 * @retval 0 成功
 * @retval other 失敗
 *
 */
unsigned __stdcall VisualizerThread(void* context)
{

	/*
		RPY Angles Rotate the reference frame by the angle roll about axis x Rotate the reference frame by the angle pitch about axis y Rotate the reference frame by the angle yaw about axis z
		Description: Sets the orientation of the Prop3D. Orientation is specified as X,Y and Z rotations in that order, but they are performed as RotateZ, RotateX, and finally RotateY.
		All axies use right hand rule. x=red axis, y=green axis, z=blue axis z direction is point into the screen.

		z
		 \
		  \
		   \
			-----------> x
			|
			|
			|
			|
			|
			|
			y
	
	*/

	const std::string visualizer_window_name("DPL 3D Viewer");
	
	struct CallbackArgs cb_args;

	// PCL Visualizer thread
	PclVizControl* pcl_viz_control = (PclVizControl*)context;

	pcl::visualization::PCLVisualizer::Ptr viewer(new pcl::visualization::PCLVisualizer(visualizer_window_name));
	cb_args.viewer_ptr = pcl::visualization::PCLVisualizer::Ptr(viewer);
	cb_args.pcl_viz_control = pcl_viz_control;

	// Background should be black
	viewer->setBackgroundColor(0, 0, 0);
	// FPS is not displayed
	viewer->setShowFPS(false);

	const int xp = MAX(0, pcl_viz_control->viz_parameters.viz_position_x);
	const int yp = MAX(0, pcl_viz_control->viz_parameters.viz_position_y);
	const int v_width = pcl_viz_control->viz_parameters.viz_width;
	const int v_height = pcl_viz_control->viz_parameters.viz_height;
	viewer->setPosition(xp, yp);
	//viewer->setPosition(0, 0);

	// setSize is not valid
	// That will change when you view it, even if you set it here.
	viewer->setSize(v_width, v_height);

	// mouse pick event
	viewer->registerPointPickingCallback(PointPickCallback, (void*)&cb_args);

	// keyboard callback
	viewer->registerKeyboardCallback(KeyboardEventCallback, (void*)&cb_args);

	// window order
	HWND hwnd = FindWindowA(NULL, visualizer_window_name.c_str());
	{
		//SetForegroundWindow(hwnd);
		SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW));
		SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW));	
	
		/*
		// 最前面表示の設定
		SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW));

		// 最前面表示の解除
		SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW));	
		*/
	}
	// vtkSmartPointer<vtkRenderWindow> renderWindow = viewer->getRenderWindow();

	// default camera position
	/*
		カメラの初期位置を指定する
		setCameraParameters(camera_info)
	*/
	pcl::visualization::Camera camera_info;
	camera_info.pos[0]		= -9.806882;
	camera_info.pos[1]		= -0.293040;
	camera_info.pos[2]		= 1.869724;
	camera_info.view[0]		= 0.087897;
	camera_info.view[1]		= -0.014561;
	camera_info.view[2]		= 0.9960232;
	camera_info.focal[0]	= 0.000000;
	camera_info.focal[1]	= 0.000000;
	camera_info.focal[2]	= 1.000000;

	// wait start
	while (!viewer->wasStopped()) {

		if (pcl_viz_control->thread_control_draw.terminate_request == 1) {
			break;
		}

		viewer->spinOnce();

		DWORD wait_result = WaitForSingleObject(pcl_viz_control->handle_semaphore_pcl_draw, 16);

		if (wait_result == WAIT_OBJECT_0) {
			EnterCriticalSection(&pcl_viz_control->threads_critical);

			if (pcl_viz_control->cloud != NULL) {
				if ((pcl_viz_control->cloud->size() != 0)) {

					if (pcl_viz_control->viz_parameters.full_screen_request) {
						viewer->setPosition(0, 0);
						viewer->setSize(1920, 1080);
					}
					else if (pcl_viz_control->viz_parameters.restore_screen_request) {
						pcl_viz_control->viz_parameters.restore_screen_request = false;
						viewer->setPosition(xp, yp);
						viewer->setSize(v_width, v_height);
					}
					else {
						viewer->setSize(v_width, v_height);
					}

					auto ret = viewer->updatePointCloud(pcl_viz_control->cloud, "cloud");

					if (!ret) {
						viewer->addPointCloud<pcl::PointXYZRGBA>(pcl_viz_control->cloud, "cloud");

						viewer->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 1, "cloud");
						if (pcl_viz_control->viz_parameters.coordinate_system) {
							viewer->addCoordinateSystem(1.0);
						}
						
						// camera 
						viewer->initCameraParameters();
						//viewer->setCameraParameters(camera_info);
					}
				}
			}

			LeaveCriticalSection(&pcl_viz_control->threads_critical);
		}
	}

	// ended
	pcl_viz_control->thread_control_draw.end_code = 0;
	pcl_viz_control->thread_control_draw.terminate_done = 1;

	return 0;
}

/**
 * 視差データより点群(Point Cloud:XYZRGBA)を作成します.
 *
 * @param[in] width データ幅
 * @param[in] height データ高さ
 * @param[in] d_inf カメラ固有パラメータ
 * @param[in] base_length カメラ基線長
 * @param[in] bf カメラ固有パラメータ
 * @param[in] min_distance 描画する最短距離
 * @param[in] max_distance 描画する最大距離
 * @param[in] base_image 画像
 * @param[in] depth_data 視差
 * @param[in] cloud 点群データ
 *
 */
void BuildPointCloud(	const int width, const int height, const double d_inf, const double base_length, const double bf, const double min_distance, const double max_distance,
						cv::Mat& base_image, cv::Mat& depth_data,
						pcl::PointCloud<pcl::PointXYZRGBA>::Ptr cloud)
{

	/*
		座標系について
		 ROS	: 右手系
		 Unity	: 左手系
 
		 ROSではロボットの進行方向がx軸、左方向がy軸、上方向がz軸の正方向

		変換方法
		 Unity -> ROS
		  Position: Unity(x,y,z) -> ROS(z,-x,y)
		  Quaternion: Unity(x,y,z,w) -> ROS(z,-x,y,-w)

		 ROS -> Unity
		  Position: ROS(x,y,z) -> Unity(-y,z,x)
		  Quaternion: ROS(x,y,z,w) -> Unity(-y,z,x,-w)
	*/

	// ポイントクラウドの大きさをセット
	cloud->width = width;
	cloud->height = height;
	cloud->is_dense = false;
	cloud->points.resize(cloud->height * cloud->width);

	// nan
	pcl::PointXYZRGBA point_nan;
	point_nan.x = std::numeric_limits<float>::quiet_NaN();
	point_nan.y = std::numeric_limits<float>::quiet_NaN();
	point_nan.z = std::numeric_limits<float>::quiet_NaN();
	point_nan.r = 0;
	point_nan.g = 0;
	point_nan.b = 0;

	int yc = height / 2;
	int xc = width / 2;

	int type = base_image.type();

	if (type == CV_8UC3) {
		for (int i = 0; i < height; i++) {

			float* src_depth = depth_data.ptr<float>(i);
			cv::Vec3b* src_image = base_image.ptr<cv::Vec3b>(i);

			for (int j = 0; j < width; j++) {
				float value = src_depth[j] - (float)d_inf;
				float x = 0;
				float y = 0;
				float z = 0;
				unsigned int col = 0;
				unsigned char b = 0;
				unsigned char g = 0;
				unsigned char r = 0;

				if (value > 0) {
					x = (base_length * (j - xc)) / value;	// m
					y = (base_length * (yc - i)) / value;	// m
					z = (float)bf / value;					// m

					col = 0;

					if (z >= min_distance && z < max_distance) {
						b = src_image[j][0];
						g = src_image[j][1];
						r = src_image[j][2];

						pcl::PointXYZRGBA point;
						point.x = -1 * x;	// z;		// x;
						point.y = y;		// x * -1;	// y;
						point.z = z;		// y;		// z;

						point.r = r;
						point.g = g;
						point.b = b;

						cloud->push_back(point);
					}
					else {
						cloud->push_back(point_nan);
					}
				}
				else {
					cloud->push_back(point_nan);
				}
			}
		}
	}
	else if (type == CV_8UC4) {
		for (int i = 0; i < height; i++) {

			float* src_depth = depth_data.ptr<float>(i);
			cv::Vec4b* src_image = base_image.ptr<cv::Vec4b>(i);

			for (int j = 0; j < width; j++) {
				float value = src_depth[j] - (float)d_inf;
				float x = 0;
				float y = 0;
				float z = 0;
				unsigned int col = 0;
				unsigned char b = 0;
				unsigned char g = 0;
				unsigned char r = 0;

				if (value > 0) {
					x = (base_length * (j - xc)) / value;
					y = (base_length * (yc - i)) / value;
					z = (float)bf / value;

					col = 0;

					if (z >= min_distance && z < max_distance) {
						b = src_image[j][0];
						g = src_image[j][1];
						r = src_image[j][2];

						pcl::PointXYZRGBA point;
						point.x = -1 * x;	// z;		// x;
						point.y = y;		// x * -1;	// y;
						point.z = z;		// y;		// z;

						point.r = r;
						point.g = g;
						point.b = b;

						cloud->push_back(point);
					}
					else {
						cloud->push_back(point_nan);
					}
				}
				else {
					cloud->push_back(point_nan);
				}

			}
		}
	}

	return;
}

/**
 * 値がユーザーが指定した特定の範囲にないポイントがクラウドから削除される.
 *
 * @param[in] field_name 対象とするフィールド
 * @param[in] min_length 最短距離
 * @param[in] max_length 最大距離
 * @param[in] cloud 入力点群データ
 * @param[out] filtered_cloud フィルター後の点群データ
 *
 * @retval 0 成功
 * @retval other 失敗
 *
 */
int PathThroughFilter(const std::string field_name, const double min_length, const double max_length, pcl::PointCloud<pcl::PointXYZRGBA>::Ptr cloud, pcl::PointCloud<pcl::PointXYZRGBA>::Ptr filtered_cloud)
{
	// パススルー　フィルター
	// pass through filter
	pcl::PassThrough<pcl::PointXYZRGBA> filter;
	filter.setInputCloud(cloud);

	// Filter out all points with Z values not in the [1-10] range.
	// Z値が[1-10]の範囲に「ない」すべてのポイントを見つけ出す
	filter.setFilterFieldName(field_name);
	filter.setFilterLimits(min_length, max_length);

	filter.filter(*filtered_cloud);

	return 0;
}

/**
 * 半径に基づく外れ値除去.
 *
 * @param[in] radius_search 検索半径
 * @param[in] min_neighbors_in_radius ポイントが外れ値としてラベル付けされるのを避けるべき最小の近傍点数
 * @param[in] cloud 入力点群データ
 * @param[out] filtered_cloud フィルター後の点群データ
 *
 * @retval 0 成功
 * @retval other 失敗
 *
 */
int RadiusOutlierRemoval(const double radius_search, const int min_neighbors_in_radius, pcl::PointCloud<pcl::PointXYZRGBA>::Ptr cloud, pcl::PointCloud<pcl::PointXYZRGBA>::Ptr filtered_cloud)
{
	// 半径に基づく外れ値除去
	/*
		このアルゴリズムはすべてのポイントを繰り返し実行し（そのためクラウドが大きい場合は遅くなる可能性がある）、チェックを実行する。
		指定された半径内に指定した近傍数より少ないポイントが見つかった場合は、それらを削除する。
	*/

	pcl::RadiusOutlierRemoval<pcl::PointXYZRGBA> filter;
	filter.setInputCloud(cloud);
	// Every point must have 5neighbors within 15cm, or it will be removed.
	// どのポイントも15cm以内に5個以上の近傍点を持たなければならない、そうでなければ除去される
	filter.setRadiusSearch(radius_search);						// unit:m
	filter.setMinNeighborsInRadius(min_neighbors_in_radius);	// unit:count

	filter.filter(*filtered_cloud);

	return 0;
}

/**
 * クラウドのポイント数を減らす（ダウンサンプリングする）.
 *
 * @param[in] boxel_size ボクセルのサイズ
 * @param[in] cloud 入力点群データ
 * @param[out] filtered_cloud フィルター後の点群データ
 *
 * @retval 0 成功
 * @retval other 失敗
 *
 */
int DownSampling(const double boxel_size, pcl::PointCloud<pcl::PointXYZRGBA>::Ptr cloud, pcl::PointCloud<pcl::PointXYZRGBA>::Ptr filtered_cloud)
{
	// ダウンサンプリング

	pcl::VoxelGrid<pcl::PointXYZRGBA> filter;
	filter.setInputCloud(cloud);

	// We set the size of every voxel to be 1x1x1cm
	// (only one point per every cubic centimeter will survive).
	// どのボクセルのサイズも 1 x 1 x 1 cmとする(1 立方センチメートルの立方体あたり1個だけ残す)

	float lx = (float)boxel_size;
	float ly = (float)boxel_size;
	float lz = (float)boxel_size;
	filter.setLeafSize(lx, ly, lz);

	filter.filter(*filtered_cloud);

	return 0;
}

/**
 * 平面検出を行います.
 *
 * @param[in] threshold 平面とするThreshold
 * @param[inout] cloud 入力点群データ(インプレース処理)
 *
 * @retval 0 成功
 * @retval other 失敗
 *
 */
int PlaneDetection(double threshold, pcl::PointCloud<pcl::PointXYZRGBA>::Ptr cloud)
{
	//平面方程式と平面と検出された点のインデックス
	pcl::ModelCoefficients::Ptr coefficients(new pcl::ModelCoefficients);
	pcl::PointIndices::Ptr inliers(new pcl::PointIndices);

	//RANSACによる検出．
	pcl::SACSegmentation<pcl::PointXYZRGBA> seg;
	seg.setOptimizeCoefficients(true);				//外れ値の存在を前提とし最適化を行う
	seg.setModelType(pcl::SACMODEL_PLANE);			//モードを平面検出に設定
	seg.setMethodType(pcl::SAC_RANSAC);				//検出方法をRANSACに設定
	seg.setDistanceThreshold(threshold);			//しきい値を設定
	seg.setInputCloud(cloud->makeShared());			//入力点群をセット
	seg.segment(*inliers, *coefficients);			//検出を行う

	if (inliers->indices.size() == 0)
	{
		std::cout << "Could not estimate a planar model for the given dataset." << std::endl;
		return -1;
	}

	for (size_t i = 0; i < inliers->indices.size(); ++i) {
		cloud->points[inliers->indices[i]].r = 255;
		cloud->points[inliers->indices[i]].g = 0;
		cloud->points[inliers->indices[i]].b = 0;
	}

	return 0;
}

/**
 * Point Cloudをファイルへ保存します.　保存形式は、PCD COMPRESSED　です
 *
 * @param[in] write_file_name 保存ファイル名
 * @param[inout] cloud 入力点群データ
 *
 * @retval 0 成功
 * @retval other 失敗
 *
 */
int WritePclToFile(char* write_folder_name, pcl::PointCloud<pcl::PointXYZRGBA>::Ptr cloud)
{

	SYSTEMTIME st = {};
	GetLocalTime(&st);

	char date_time_name[_MAX_PATH] = {};
	// YYYYMMDD_HHMMSS
	sprintf(date_time_name, "%04d%02d%02d_%02d%02d%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

	char write_file_name[_MAX_PATH] = {};
	sprintf(write_file_name, "%s\\dpl-pcd-dada_%s.pcd", write_folder_name, date_time_name);

	std::string file_name(write_file_name);

	int ret = pcl::io::savePCDFileBinaryCompressed(file_name, *cloud);
	if (ret != 0) {
		// failed
		return ret;
	}

	return 0;
}
