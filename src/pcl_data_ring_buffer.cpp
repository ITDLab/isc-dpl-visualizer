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
 * @file pcl_data_ring_buffer.cpp
 * @brief Ring Buffer to hold data for PCL display.
 * @author Takayuki
 * @date 2023.10.30
 * @version 0.1
 * 
 * @details Ring buffer for transferring data between Threads for 3D display using PCL.
 */

#include <Windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h>

#include "pcl_def.h"

#include "pcl_data_ring_buffer.h"

/**
 * constructor
 *
 */
PclDataRingBuffer::PclDataRingBuffer():
	flag_critical_(), last_mode_(false), allow_overwrite_(false), buffer_count_(0), width_(0), height_(0), channel_count_(0), buffer_data_(nullptr),
	write_inex_(0), read_index_(0), put_index_(0), geted_inedx_(0),
	buff_image_(nullptr), buff_disparity_image_bgra_(nullptr), buff_disparity_data_(nullptr)
{
}

/**
 * destructor
 *
 */
PclDataRingBuffer::~PclDataRingBuffer()
{
}

/**
 * バッファーを初期化します.
 *
 * @param[in] last_mpde true:最新のものを取得します false:FIFOです
 * @param[in] allow_overwrite true:オーバーフロー時に上書きします
 * @param[in] count リングバッファーの深さです
 * @param[in] width_def データ幅
 * @param[in] height_def データ高さ
 *
 * @retval 0 成功
 * @retval -1 失敗
 */
int PclDataRingBuffer::Initialize(const bool last_mpde, const bool allow_overwrite, const int count, const int width_def, const int height_def)
{
	last_mode_ = last_mpde;
	allow_overwrite_ = allow_overwrite;
	buffer_count_ = count;
	width_ = width_def;
	height_ = height_def;
	write_inex_ = 0; read_index_ = 0; put_index_ = 0;  geted_inedx_ = 0;

	InitializeCriticalSection(&flag_critical_);

	buffer_data_ = new BufferData[buffer_count_];

	const size_t one_frame_size = width_ * height_;

	buff_image_					= new unsigned char[buffer_count_ * one_frame_size * 4];
	buff_disparity_data_		= new float[buffer_count_ * one_frame_size];
	buff_disparity_image_bgra_	= new unsigned char[buffer_count_ * one_frame_size * 4];

	memset(buff_image_,					0, buffer_count_ * one_frame_size * 4);
	memset(buff_disparity_data_,		0, buffer_count_ * one_frame_size * sizeof(float));
	memset(buff_disparity_image_bgra_,	0, buffer_count_ * one_frame_size * 4);

	for (int i = 0; i < buffer_count_; i++) {
		buffer_data_[i].inedx = i;
		buffer_data_[i].state = 0;
		buffer_data_[i].time = 0;

		buffer_data_[i].pcl_data.width = width_;
		buffer_data_[i].pcl_data.height = height_;
		buffer_data_[i].pcl_data.base_image_channel_count = 1;
		
		buffer_data_[i].pcl_data.depth_width = width_;
		buffer_data_[i].pcl_data.depth_height = height_;

		memset(&buffer_data_[i].pcl_filter_parameter, 0, sizeof(PclFilterParameter));

		size_t unit = one_frame_size * 4;
		buffer_data_[i].pcl_data.image = buff_image_ + (unit * i);

		unit = one_frame_size;
		buffer_data_[i].pcl_data.disparity_data = buff_disparity_data_ + (unit * i);

		unit = one_frame_size * 4;
		buffer_data_[i].pcl_data.disparity_image_bgra = buff_disparity_image_bgra_ + (unit * i);
	}

	return 0;
}

/**
 * 各変変数を初期化します.
 *
 *
 * @return none.
 */
int PclDataRingBuffer::Clear()
{
	EnterCriticalSection(&flag_critical_);
	write_inex_ = 0; read_index_ = 0; put_index_ = 0;  geted_inedx_ = 0;

	for (int i = 0; i < buffer_count_; i++) {
		buffer_data_[i].state = 0;
		memset(&buffer_data_[i].pcl_filter_parameter, 0, sizeof(PclFilterParameter));
	}
	LeaveCriticalSection(&flag_critical_);

	const size_t one_frame_size = width_ * height_;

	memset(buff_image_,					0, buffer_count_ * one_frame_size * 4);
	memset(buff_disparity_data_,		0, buffer_count_ * one_frame_size * sizeof(float));
	memset(buff_disparity_image_bgra_,	0, buffer_count_ * one_frame_size * 4);

	return 0;
}

/**
 * 動作モードを設定します.
 *
 * @param[in] last_mpde true:最新のものを取得します false:FIFOです
 * @param[in] allow_overwrite true:オーバーフロー時に上書きします
 *
 * @retval 0 成功
 * @retval -1 失敗
 */
int PclDataRingBuffer::SetMode(const bool last_mpde, const bool allow_overwrite)
{
	last_mode_ = last_mpde;
	allow_overwrite_ = allow_overwrite;

	return 0;
}

/**
 * 終了します.
 *
 *
 * @return none.
 */
int PclDataRingBuffer::Terminate()
{
	delete[] buff_image_;
	delete[] buff_disparity_data_;
	delete[] buff_disparity_image_bgra_;

	buff_image_ = nullptr;
	buff_disparity_data_ = nullptr;
	buff_disparity_image_bgra_ = nullptr;

	delete buffer_data_;
	buffer_data_ = nullptr;

	DeleteCriticalSection(&flag_critical_);

	return 0;
}

/**
 * 書き込み対象のバッファーのポインタを取得します.
 *
 * @param[in/out] buffer_data バッファーのポインタを書き込みます
 * @param[in] time 現在時間です
 *
 * @retval >0 バッファーのIndex
 * @retval -1 失敗 空きバッファー無し
 */
int PclDataRingBuffer::GetPutBuffer(BufferData** buffer_data, const ULONGLONG time)
{
	if (buffer_data_ == nullptr) {
		return -1;
	}

	EnterCriticalSection(&flag_critical_);

	int local_write_inex = write_inex_;

	if (buffer_data_[local_write_inex].state == 3) {
		// in use for Get...
		LeaveCriticalSection(&flag_critical_);
		return -1;
	}

	if (!allow_overwrite_) {
		if (buffer_data_[local_write_inex].state != 0) {
			LeaveCriticalSection(&flag_critical_);
			return -1;
		}
	}

	*buffer_data = &buffer_data_[local_write_inex];

	buffer_data_[local_write_inex].time = time;	// GetTickCount();
	buffer_data_[local_write_inex].state = 1;
	put_index_ = local_write_inex;

	LeaveCriticalSection(&flag_critical_);

	return put_index_;
}

/**
 * 取得したバッファーの使用を終了します.
 *
 * @param[in] index バッファーのIndexです。取得時のものです
 * @param[in] status 1:有効です
 *
 * @retval 0 成功
 * @retval -1 失敗
 */
int PclDataRingBuffer::DonePutBuffer(const int index, const int status)
{
	// status 0:This data is invalid 1:This data is valid

	if (buffer_data_ == nullptr) {
		return -1;
	}

	if (index < 0 || index >= buffer_count_) {
		return -1;
	}

	if (index != put_index_) {
		// Get and Done must correspond one-to-one on the same Thread
		return -1;
	}

	EnterCriticalSection(&flag_critical_);

	if (buffer_data_[index].state != 1) {
		// error, this case should not exist
		__debugbreak();
		LeaveCriticalSection(&flag_critical_);
		return -1;
	}

	if (status == 0) {
		// it change 1 -> 0 (not use)
		buffer_data_[index].state = 0;
	}
	else {
		buffer_data_[index].state = 2;

		if (last_mode_) {
			read_index_ = index;
		}

		write_inex_++;
		if (write_inex_ >= buffer_count_) {
			write_inex_ = 0;
		}
	}

	LeaveCriticalSection(&flag_critical_);

	return 0;
}

/**
 * 読み込み対象のバッファーのポインタを取得します.
 *
 * @param[in/out] buffer_data バッファーのポインタを書き込みます
 * @param[in] time 書き込み時間です
 *
 * @retval >0 バッファーのIndex
 * @retval -1 失敗 データ無し
 */
int PclDataRingBuffer::GetGetBuffer(BufferData** buffer_data, ULONGLONG* time_get)
{
	if (buffer_data_ == nullptr) {
		return -1;
	}

	EnterCriticalSection(&flag_critical_);
	int local_read_index = read_index_;

	if (buffer_data_[local_read_index].state != 2) {
		LeaveCriticalSection(&flag_critical_);
		return -1;
	}

	*buffer_data = &buffer_data_[local_read_index];

	*time_get = buffer_data_[local_read_index].time;
	buffer_data_[local_read_index].state = 3;
	geted_inedx_ = local_read_index;

	if (last_mode_) {
		if (!allow_overwrite_) {
			// clear previous 
			int s = local_read_index - 1;
			int e = local_read_index + 1;
			if (s == -1) {
				s = buffer_count_ - 1;
			}
			if (e >= buffer_count_) {
				e = 0;
			}
			for (int i = s; i != e;) {
				if (buffer_data_[i].state != 2) {
					break;
				}
				buffer_data_[i].state = 0;

				i--;
				if (i == 0) {
					i = buffer_count_ - 1;
				}
			}
		}
	}
	else {
		read_index_++;
		if (read_index_ >= buffer_count_) {
			read_index_ = 0;
		}
	}

	LeaveCriticalSection(&flag_critical_);

	return geted_inedx_;
}

/**
 * 取得したバッファーの使用を終了します.
 *
 * @param[in] index バッファーのIndexです。取得時のものです
 *
 * @retval 0 成功
 * @retval -1 失敗
 */
void PclDataRingBuffer::DoneGetBuffer(const int index)
{
	if (buffer_data_ == nullptr) {
		return;
	}

	if (index < 0 || index >= buffer_count_) {
		return;
	}

	if (index != geted_inedx_) {
		// Get and Done must correspond one-to-one on the same Thread
		printf("[ERROR]Get and Done must correspond one-to-one on the same Thread\n");
		return;
	}

	EnterCriticalSection(&flag_critical_);
	buffer_data_[index].state = 0;
	LeaveCriticalSection(&flag_critical_);

	return;
}

