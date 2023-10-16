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
// limitations

/**
 * @file dpl_gui_configuration.h
 * @brief Preserves the contents of the configuration file.
 */

#pragma once

/**
 * @class   DplGuiConfiguration
 * @brief   Preserves the contents of the configuration file
 * 
 */
class DplGuiConfiguration
{

public:

	DplGuiConfiguration();
	~DplGuiConfiguration();

	bool Load(const wchar_t* file_path);
	bool Save();

	bool GetLogFilePath(wchar_t* file_path, const int max_length) const;
	void SetLogFilePath(const wchar_t* file_path);
	int GetLogLevel() const;
	void SetLogLevel(const int level);

	bool IsEnabledCamera() const;
	void SetEnabledCamera(const bool enabled);
	int GetCameraModel() const;
	void SetCameraModel(const int model);
	bool GetDataRecordPath(wchar_t* path, const int max_length) const;
	void SetDataRecordPath(const wchar_t* path);
	
	bool IsEnabledDataProcLib() const;
	void SetEnabledDataProcLib(const bool enabled);

	double GetDrawMinDistance() const;
	void SetDrawMinDistance(const double distance);
	double GetDrawMaxDistance() const;
	void SetDrawMaxDistance(const double distance);
	double GetMaxDisparity() const;
	bool IsDrawOutsideBounds() const;
	void SetDrawOutsideBounds(const bool enabled);

private:

	bool successfully_loaded_;					/**< Configuration successfully loaded. */

	wchar_t configuration_file_path_[_MAX_PATH];	/**< Configuration file path */
	wchar_t configuration_file_name_[_MAX_PATH];	/**< Full path of the configuration file */

	wchar_t log_file_path_[_MAX_PATH];			/**< log save path */
	int log_level_;								/**< log mode */
		
	bool enabled_camera_;						/**< camera-enabled */
	int camera_model_;							/**< Camera type 0:VM 1:XC 2:4K 3:4KA 4:4KJ */
	wchar_t data_record_path_[_MAX_PATH];		/**< Data Storage Destination */

	bool enabled_data_proc_library_;			/**< Data Processing Module Enabled */

	double draw_min_distance_;					/**< Minimum display distance */
	double draw_max_distance_;					/**< Maximum display distance */
	bool draw_outside_bounds_;					/**< Draw outside the minimum to maximum display */

	double max_disparity_;						/**< maximum parallax value */

};

