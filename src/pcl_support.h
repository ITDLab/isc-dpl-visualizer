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
 * @file pcl_support.h
 * @brief Provides display functionality using Point Cloud Library.
 */

#pragma once

/** @brief Initializes the PCL support function. Must be called at least once before streaming is started.
    @return 0, if successful.
 */
int InitializePclViz(const VizParameters* init_viz_parameters);

/** @brief ... Shut down the runtime system. Don't call any method after calling Terminate().
    @return 0, if successful.
 */
int TerminatePclViz();

/** @brief Display Thread is started and data can be accepted.
    @return 0, if successful.
 */
int StartPclViz();

/** @brief Stop display Thread.
    @return 0, if successful.
 */
int StopPclViz();

/** @brief Enter data for display.
    @return 0, if successful.
 */
int RunPclViz(PclVizInputArgs* input_args, PclVizOutputArgs* output_args);
