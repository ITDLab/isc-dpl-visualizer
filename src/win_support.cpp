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
 * @file win_support.cpp
 * @brief Provides Windows-specific functions.
 * @author Takayuki
 * @date 2023.10.30
 * @version 0.1
 * 
 * @details Provides Wrapper functionality for calling Windows APIs.
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
#include <shobjidl.h> 

#include "win_support.h"

/**
 * コードページを初期化します.　Vusual StudioのConsole 出力のための設定です
 *
 */
void InitForWinConsole()
{
    // for Visual Studio debug console
    SetConsoleOutputCP(932);
    
    return;
}

/**
 * 実行ファイルのフルパスより実行フォルダを取得します
 *
 * @param[out] module_path 実行フォルダ
 * @param[in] max_length 最大長
 *
 */
void GetModulePath(wchar_t* module_path, int max_length)
{
    char mdule_file_name[_MAX_PATH] = {};
    DWORD len = GetModuleFileNameA(NULL, mdule_file_name, sizeof(mdule_file_name));

    //wchar_t module_path[_MAX_PATH] = {};
    GetAbsModulePath(mdule_file_name, module_path, max_length);

    return;
}

/**
 * 実行ファイルのフルパスより実行フォルダを取得します
 *
 * @param[in] mdule_file_name 実行ファイルのフルパス
 * @param[in] module_path 実行フォルダ
 * @param[in] max_length 最大長
 *
 * @retval 0 成功
 * @retval other 失敗
 */
void GetAbsModulePath(const char* mdule_file_name, wchar_t* module_path, const int max_length)
{

    char* loc_ret = setlocale(LC_ALL, "ja-JP");
    wchar_t result_path[_MAX_PATH] = {};

    size_t return_value = 0;
    mbstate_t mb_state = { 0 };
    const char* src = mdule_file_name;
    size_t size_in_words = _MAX_PATH;
    errno_t err = mbsrtowcs_s(&return_value, &result_path[0], size_in_words, &src, size_in_words - 1, &mb_state);
    if (err != 0) {
        return;
    }

    PathRemoveFileSpecW(result_path);

    swprintf_s(module_path, max_length, L"%s", result_path);

    return;
}

const COMDLG_FILTERSPEC dlg_show_types[] =
{
    {L"dat files(*.dat)",   L"*.dat"},
    {L"all files (*.*)",    L"*.*"}
};

// Indexes of file types
#define kINDEX_FILE_DAT 1
#define kINDEX_FILE_ALL 2

/**
 * ファイルのオープンダイアログを実行します
 *
 * @param[in] initial_folder 初期フォルダー
 * @param[out] open_file_name 選択されたファイル名
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int WsOpenFileDialog(wchar_t* initial_folder, wchar_t* open_file_name)
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr)) {
        IFileOpenDialog* ifile_open_dialog;

        // Create the FileOpenDialog object.
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&ifile_open_dialog));

        if (SUCCEEDED(hr)) {
            ifile_open_dialog->SetFileTypes(ARRAYSIZE(dlg_show_types), dlg_show_types);     // 表示するファイルタイプを指定
            ifile_open_dialog->SetFileTypeIndex(kINDEX_FILE_DAT);                           // ダイアログオープン時のデフォルト表示の指定

            IShellItem* ishell_item = NULL;
            wchar_t current_folder[_MAX_PATH] = {};
            swprintf_s(current_folder, L"%s", initial_folder);

            hr = SHCreateItemFromParsingName(current_folder, NULL, IID_PPV_ARGS(&ishell_item));
            if (SUCCEEDED(hr)) {
                ifile_open_dialog->SetFolder(ishell_item);
                ishell_item->Release();
            }

            // Show the Open dialog box.
            hr = ifile_open_dialog->Show(NULL);

            // Get the file name from the dialog box.
            if (SUCCEEDED(hr)) {
                IShellItem* ishell_item_get;
                hr = ifile_open_dialog->GetResult(&ishell_item_get);
                if (SUCCEEDED(hr)) {
                    PWSTR file_path;
                    hr = ishell_item_get->GetDisplayName(SIGDN_FILESYSPATH, &file_path);

                    // Display the file name to the user.
                    if (SUCCEEDED(hr)) {
                        //MessageBoxW(NULL, file_path, L"File Path", MB_OK);
                        swprintf_s(open_file_name, _MAX_PATH, L"%s", file_path);
                        CoTaskMemFree(file_path);
                    }
                    ishell_item_get->Release();
                }
            }
            ifile_open_dialog->Release();
        }
        CoUninitialize();
    }

    return 0;
}

/**
 * フォルダのオープンダイアログを実行します
 *
 * @param[out] open_folder_name 選択されたフォルダ
 *
 * @retval 0 成功
 * @retval other 失敗
 */
int WsOpenFolderDialog(wchar_t* open_folder_name)
{
    int ret = -1;
    
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    if (SUCCEEDED(hr)) {
        IFileOpenDialog* ifile_open_dialog;

        // Create the FileOpenDialog object.
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&ifile_open_dialog));

        if (SUCCEEDED(hr)) {
            // 設定の初期化
            DWORD options = 0;;
            ifile_open_dialog->GetOptions(&options);
            ifile_open_dialog->SetOptions(options | FOS_PICKFOLDERS);

            IShellItem* ishell_item = NULL;
            wchar_t current_folder[_MAX_PATH] = {};
            swprintf_s(current_folder, L"%s", L"c:\\temp");

            hr = SHCreateItemFromParsingName(current_folder, NULL, IID_PPV_ARGS(&ishell_item));
            if (SUCCEEDED(hr)) {
                ifile_open_dialog->SetFolder(ishell_item);
                ishell_item->Release();
            }

            // Show the Open dialog box.
            hr = ifile_open_dialog->Show(NULL);

            // Get the file name from the dialog box.
            if (SUCCEEDED(hr)) {
                IShellItem* ishell_item_get;
                hr = ifile_open_dialog->GetResult(&ishell_item_get);
                if (SUCCEEDED(hr)) {
                    PWSTR file_path;
                    hr = ishell_item_get->GetDisplayName(SIGDN_FILESYSPATH, &file_path);

                    // Display the file name to the user.
                    if (SUCCEEDED(hr)) {
                        //MessageBoxW(NULL, file_path, L"File Path", MB_OK);
                        swprintf_s(open_folder_name, _MAX_PATH, L"%s", file_path);
                        ret = 0;
                        CoTaskMemFree(file_path);
                    }
                    ishell_item_get->Release();
                }
            }
            ifile_open_dialog->Release();
        }
        CoUninitialize();
    }

    return ret;
}
