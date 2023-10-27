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
 * @file pcl_def.h
 * @brief Data structure for pcl_support.
 */

#pragma once

/** @struct  PclFilterParameter
 *  @brief PCL Operation Mode Setting Parameters
 */
struct PclFilterParameter {
	struct Range {
		float min, max;
	};

	struct RadiusOuterParam {
		double radius_search;
		int min_neighbors;
	};

	// it remove NAN
	bool enabled_remove_nan;							/**< Removes points with x, y, or z equal to NaN */

	// pass through filter
	bool enabled_pass_through_filter;					/**< passes points in a cloud based on constraints for one particular field of the point type */
	Range pass_through_filter_range;					/**< range(m) */

	// down sampling
	bool enabled_down_sampling;							/**< set the size of every voxel to be w/h/z */
	float down_sampling_boxel_size;						/**< range(m) */

	// radius outlier removal
	bool enabled_radius_outlier_removal;				/**< filters points in a cloud based on the number of neighbors they have */
	RadiusOuterParam radius_outlier_removal_param;		/**< radius(m) */

	// plane detection
	bool enabled_plane_detection;						/**< Palne detection by RANZAC */
	double plane_detection_threshold;					/**< plane threshold */

};

/** @struct  VizParameters
 *  @brief Display Settings
 */
struct VizParameters {
	// viz
	int viz_position_x, viz_position_y;		/**< Display position */
	int viz_width, viz_height;				/**< Display size */

	// camera
	int width;								/**< Display width */
	int height;								/**< Display height */
	double d_inf;							/**< Camera Specific Parameters */
	double base_length;						/**< Camera baseline length */
	double bf;								/**< Camera Specific Parameters */
	double min_distance;					/**< Minimum display distance */
	double max_distance;					/**< Indicates the maximum distance */

	// system
	bool coordinate_system;					/**< Display axis */
	bool full_screen_request;				/**< Request full screen display */
	bool restore_screen_request;			/**< Exit full-screen display */
	char pcd_file_write_folder[_MAX_PATH];	/**< Folder name for save pcd file */
};

/** @struct  PclVizInputArgs
 *  @brief Display Data
 */
struct PclVizInputArgs {
	double d_inf;								/**< Camera Specific Parameters */
	double base_length;							/**< Camera baseline length */
	double bf;									/**< Camera Specific Parameters */

	int width;									/**< Image Width */
	int height;									/**< Image Height */
	int base_image_channel_count;				/**< Channkel number of images */
	unsigned char* image;						/**< display image */

	int depth_width;							/**< Width of disparity data */
	int depth_height;							/**< Height of disparity data */
	float* disparity_data;						/**< disparity data */

	unsigned char* disparity_image_bgra;		/**< Color image */

	bool full_screen_request;					/**< Request full screen display */
	bool restore_screen_request;				/**< Exit full-screen display */

	PclFilterParameter pcl_filter_parameter;	/**< Display PCL data and filter settings */

};

/** @struct  PclVizOutputArgs
 *  @brief Output from display module
 */
struct PclVizOutputArgs {

	// mouse pick informamtion
	struct PickData {
		bool valid;
		float x, y, z;
	};

	struct PickInforamtion {
		int max_count;
		int count;
		PickData pick_data[4];
	};
	PickInforamtion pick_information;	/**< Information about the location selected with the mouse */
};
