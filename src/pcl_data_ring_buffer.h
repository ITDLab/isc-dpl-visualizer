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
 * @file pcl_data_ring_buffer.h
 * @brief Ring Buffer to hold data for PCL display.
 */

#pragma once

/**
 * @class   PclDataRingBuffer
 * @brief   Buffer class
 * this class is an inplementation for PCL support
 */
class PclDataRingBuffer {
public:

	struct PclData {
		int width;
		int height;
		int base_image_channel_count;
		unsigned char* image;

		int depth_width;
		int depth_height;
		float* disparity_data;

		unsigned char* disparity_image_bgra;
	};
	
	struct BufferData {
		int inedx;									/**< buffer number */
		int state;									/**< 0:nothing 1:under write 2: write done 3:read/using */
		ULONGLONG time;								/**< put time */

		PclFilterParameter pcl_filter_parameter;	/**< filter parameter for build Point cloud data */

		PclData pcl_data;							/**< images */
	};

	PclDataRingBuffer();
	~PclDataRingBuffer();

	int Initialize(const bool last_mpde, const bool allow_overwrite, const int count, const int width_def, const int height_def);

	int Clear();

	int SetMode(const bool last_mpde, const bool allow_overwrite);

	int Terminate();

	int GetPutBuffer(BufferData** buffer_data, const ULONGLONG time);

	int DonePutBuffer(const int index, const int status);

	int GetGetBuffer(BufferData** buffer_data, ULONGLONG* time_get);

	void DoneGetBuffer(const int index);

private:
	CRITICAL_SECTION flag_critical_;
	bool last_mode_;
	bool allow_overwrite_;

	int buffer_count_;
	int width_, height_;
	int channel_count_;

	BufferData* buffer_data_;

	int write_inex_, read_index_, put_index_, geted_inedx_;

	unsigned char* buff_image_;
	unsigned char* buff_disparity_image_bgra_;
	float* buff_disparity_data_;

};
