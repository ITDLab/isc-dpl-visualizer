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
 * @file dpl_control.h
 * @brief A class to help facilitate interfacing with the DPL library.
 */

#pragma once

/**
 * @class   DplControl
 * @brief   dpl support class
 * this class is an inplementation for isc DPL
 */
class DplControl
{
public:
	/** @struct  StartMode
	 *  @brief Start Parameters
	 */
	struct StartMode {
		int grab_mode;						/**< grab mode 0:disparity 1:correct image 2:before correct image */

		bool enabled_stereo_matching;		/**< enable stereo matching function */
		bool enabled_disparity_filter;		/**< enable disparity filter function */
		bool enabled_color;					/**< enable color caputure mode */

		bool grab_record_mode;				/**< start grab width recode */

		bool grab_play_mode;				/**< read data from file */
		wchar_t play_file_name[_MAX_PATH];	/**< file name for reda data */
	};

	DplControl();
	~DplControl();

	/** @brief Initializes the CaptureSession and prepares it to start streaming data. Must be called at least once before streaming is started.
		@return true, if successful.
	*/
	bool Initialize(const wchar_t* module_path);

	/** @brief ... Shut down the runtime system. Don't call any method after calling Terminate().
		@return 0, if successful.
	 */
	void Terminate();

	/** @brief Buffer initialization.
		@return 0, if successful.
	 */
	bool InitializeBuffers(IscImageInfo* isc_image_Info, IscDataProcResultData* isc_data_proc_result_data);

	/** @brief Resource Release.
		@return 0, if successful.
	 */
	bool ReleaseBuffers(IscImageInfo* isc_image_Info, IscDataProcResultData* isc_data_proc_result_data);

	/** @brief Returns the current camera model.
		@return camera models.
	 */
	int GetCameraModel() const;

	/** @brief Returns whether the camera is enabled or disabled.
		@return true, if enabled.
	 */
	bool GetCameraEnabled() const;

	/** @brief Returns the location where the camera data is stored.
		@return true, if successful.
	 */
	bool GetDataRecordPath(wchar_t* path, const int max_length) const;

	/** @brief Returns the minimum distance to be drawn.
		@return minimum distance.
	 */
	double GetDrawMinDistance() const;

	/** @brief Returns the maximum distance to draw.
		@return maximum distance.
	 */
	double GetDrawMaxDistance() const;

	/** @brief Returns a pointer to the library isc-dpl.
		@return iscDpl object pointer.
	 */
	ns_isc_dpl::IscDpl* GetDplObgkect() const;

	/** @brief Start capturing.
		@return 0, if successful.
	 */
	bool Start(StartMode& start_mode);

	/** @brief Stop capturing.
		@return true, if successful.
	 */
	bool Stop();

	/** @brief Get camera data.
		@return true, if successful.
	 */
	bool GetCameraData(IscImageInfo* isc_image_Info);

	/** @brief Retrieves data from the processing library.
		@return true, if successful.
	 */
	bool GetDataProcessingData(IscDataProcResultData* isc_data_proc_result_data);

	/** @brief Get camera parameters.
		@return true, if successful.
	 */
	bool GetCameraParameter(float* b, float* bf, float* dinf, int* width, int* height);

	/** @brief Retrieves header information for saved data files.
		@return true, if successful.
	 */
	bool GetFileInformation(wchar_t* file_name, IscRawFileHeader* raw_file_headaer);

	/** @brief Obtain the minimum and maximum distance of the drawing that has been set.
		@return 0, if successful.
	 */
	void GetMinMaxDistance(double* min_distance, double* max_distance) const;

	/** @brief Regenerate Color LUTs in the input range.
		@return 0, if successful.
	 */
	void RebuildDrawColorMap(const double min_distance, const double max_distance);

	/** @brief Converts disparity data to a Color image..
		@return 0, if successful.
	 */
	bool ConvertDisparityToImage(double b, const double angle, const double bf, const double dinf,
									const int width, const int height, float* depth, unsigned char* bgra_image);

private:

    wchar_t configuration_file_path_[_MAX_PATH];	/**< Full path of the configuration file */
    wchar_t log_file_path_[_MAX_PATH];				/**< Full path of log file */
    wchar_t image_path_[_MAX_PATH];					/**< Where to save images */

	int camera_model_;								/**< The model of the camera to be used. */
	bool camera_enabled_;							/**< Camera enabled. */
	double draw_min_distance_, draw_max_distance_;	/**< Minimum and maximum distances to draw */
	bool is_draw_outside_bounds_;					/**< Draws outside the specified area */

	IscImageInfo isc_image_info_;						/**< image buffer */
	IscDataProcResultData isc_data_proc_result_data_;	/**< Data processing results */

	struct CameraParameter {
		float b;
		float bf;
		float dinf;
		float setup_angle;
	};
	CameraParameter camera_parameter_;				/**< Camera Specific Parameters */

	IscDplConfiguration isc_dpl_configuration_;		/**< Configure data processing libraries */
	ns_isc_dpl::IscDpl* isc_dpl_;					/**< Data Processing Library Objects */

	IscStartMode isc_start_mode_;					/**< Image capturing start parameters */

	// Color Map
	struct DispColorMap {
		double min_value;						
		double max_value;						

		int color_map_size;						
		int* color_map;							
		double color_map_step;					
	};
	DispColorMap disp_color_map_distance_;			/**< Color LUT(distance) */
	DispColorMap disp_color_map_disparity_;			/**< Color LUt(Disparity) */
	double max_disparity_;							/**< Max Disparity */			

	/** @brief Create LUT for Color Map.
		@return 0, if successful.
	 */
	int BuildColorHeatMap(DispColorMap* disp_color_map);

	/** @brief Create Color LUT with parallax range.
		@return 0, if successful.
	 */
	int BuildColorHeatMapForDisparity(DispColorMap* disp_color_map);

	/** @brief Converts distance to RGB gradient.
		@return 0, if successful.
	 */
	int ColorScaleBCGYR(const double min_value, const double max_value, const double in_value, int* bo, int* go, int* ro);

	/** @brief Creates a Color image from parallax.　Color follows the Color LUT.
		@return 0, if successful.
	 */
	bool MakeDepthColorImage(const bool is_color_by_distance, const bool is_draw_outside_bounds, const double min_length_i, const double max_length_i,
		DispColorMap* disp_color_map, double b_i, const double angle_i, const double bf_i, const double dinf_i,
		const int width, const int height, float* depth, unsigned char* bgra_image);

};

