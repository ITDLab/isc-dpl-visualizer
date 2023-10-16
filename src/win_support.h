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
 * @file win_support.h
 * @brief Provides Windows-specific functions.
 */

#pragma once

/** @brief Initialize code page.
    @return none
 */
void InitForWinConsole();

/** @brief Get the executable folder from the full path of the executable.
    @return none
 */
void GetModulePath(wchar_t* module_path, int max_length);

/** @brief Get the executable folder from the full path of the executable.
    @return none
 */
void GetAbsModulePath(const char* mdule_file_name, wchar_t* module_path, const int max_length);

/** @brief Runs the file open dialog.
    @return 0, if successful.
 */
int WsOpenFileDialog(wchar_t* initial_folder, wchar_t* open_file_name);

/** @brief Runs the Open Folder dialog.
    @return 0, if successful.
 */
int WsOpenFolderDialog(wchar_t* open_folder_name);
