# isc-dpl-visualizer 
Application for Data processing library project.

****
## Outline 
****
isc-dpl-visualizerは、Point Cloud Libraryを使用し、isc-dplライブラリの出力を点群として処理するサンプルコードです  
本サンプルコードでは、isc-dplライブラリの出力を点群に変換し、簡単なフィルター処理を行った後、それを表示します  
フィルター処理及び表示（3D）に、Point Cloud Libraryを使用しています  

isc-dplライブラリは、ISCシリーズステレオカメラに対応したカメラ制御及び視差データ処理を行うライブラリです    
isc-dplについては、 [isc-dpl](https://github.com/ITDLab/isc-dpl)　を参照してください  

Point Cloud Library (PCL) は，3D点群 (Point Cloud)を入力とする「ロボットビジョン」や「3D幾何処理」のアルゴリズム群を集めたライブラリです  
Point Cloud Libraryについては、[公式ホームページ](https://pointclouds.org/)　を参照してください

isc-dpl-visualizerでは、GUIの構築にDear ImGuiを使用しています
Dear ImGuiについては、[公式ホームページ](https://github.com/ocornut/imgui)　を参照してください

****
## Requirements for Windows  
****
- Windows 10(x64)/11  
- Visual Studio 2019 (or later)  
- OpenCV 4.8.0 (これ以外のバージョンも動作可能ですが、その場合はbuildの設定を調整してください)  
- ISC Stereo Camrea  
    - ISC100VM: FPGA(0x75)  
    - ISC100XC: FPGA(0x22)  
- ISC Stereo Camera SDK
    - ISC100VM: 2.3.2
    - ISC100XC: 2.2.2
- isc-dpl library

****
# How to build and Run  
****
## buildの流れ  
- 必要なアプリケーションのインストール
    - Visual Studioのインストール
    - CMAKEのインストール
    - Gitのインストール
    - VCPKGのインストール

- isc_dpl　のBuild
    - isc_dplをBuildします (OpenCVのインストールを含みます)

- PCLのBuild
    - PCLのBuild
    - VisualizationのBuild

- GLEW、GLFWのインストール

- サンプルアプリケーションのBuild

## 必要なアプリケーションのインストール
- Visual Studioのインストール
    - Visual Studio をインストールします（2019 or later）  
    - 英語の言語パッケージを追加インストールします  
      Visual Studio Installer　よりインストールできます  

- CMAKEのインストール
    - CMAKEの[公式ホームページ](https://cmake.org/)よりWindows用のインストーラをダウンロードして実行します
    - インストール方法については、公式ホームページの内容を参照してください

- Gitのインストール
    - [Git for Windows](https://gitforwindows.org/)よりダウンロードしてインストールします
    - インストール方法については、ホームページの内容を参照してください

- VCPKGのインストール
    - githubよりvcpkgをクローンしVCPKGをビルドします  
    - ビルド作業はPowerShell上で行います  
    - VCPKGをインストールしたいフォルダにVCPKGをクローンしビルドを実行します  
    例）Dドライブで作業します  
> PS C:\> d:  
> PS D:\> mkdir pcl  
> PS D:\> cd pcl  
> PS D:\pcl> git clone https://github.com/microsoft/vcpkg  
> PS D:\pcl> .\vcpkg\bootstrap-vcpkg.bat  

## isc-dpl　のBuild
ISCシリーズのステレオカメラに対応したデータ処理ライブラリを含むアプリケーションです  
詳細は、Githubにある[isc-dpl](https://github.com/ITDLab/isc-dpl)を参照してください  

> PS C:\> d:  
> PS D:\> mkdir isc  
> PS D:\> cd isc  
> PS D:\isc> git clone https://github.com/ITDLab/isc-dpl.git  

Githubに記載の手順に従いアプリケーションをBuildしてください  
dpl-visualizer　で使用するライブラリも同時にBuildされます  

## PCLのBuild  
- VCPKGのインストールで作成したフォルダーで作業します  
- PCLをビルドします(https://pointclouds.org/downloads/)
> PS D:\pcl> .\vcpkg\vcpkg.exe install pcl:x64-windows  

必要な全てのライブラリのダウンロードとインストールも実行されるので時間がかかります
- システムの環境変数に以下の値を設定します  
PCL_DIR=d:\pcl\vcpkg\installed\x64-windows (実際の環境に合わせます)  

- VisualizationのBuild(同じ作業ォルダーです)  
> PS D:\pcl> .\vcpkg\vcpkg.exe install pcl[visualization] --triplet x64-windows --recurse  

必要な全てのライブラリのダウンロードとインストールも実行されるので時間がかかります  

## GLEW、GLFWのインストール  
- GLEWのインストール  
    - GLEWの[公式ホームページ](https://glew.sourceforge.net/)より、Windowsのバイナリをダウンロードして解凍します  
      例）glew-2.1.0-win32.zip  
    - 環境変数に解凍したフォルダを登録します  
      GLEW_DIR=D:\OpenGLs\glew-2.1.0（実際の環境に合わせます）  

- GLFWのインストール  
    - GLFWの公式ホームページのDownloadページ内”Windows pre-compiled binaries”よりバイナリをダウンロードして解凍します(https://www.glfw.org/download.html)  
      例）glfw-3.3.8.bin.WIN64.zip  
    - 環境変数に解凍したフォルダを登録します  
      GLFW_DIR=D:\OpenGLs\glfw-3.3.8.bin.WIN64（実際の環境に合わせます）  

## サンプルアプリケーションのBuild  
- サンプルアプリケーションをbuildするフォルダーに移動します(例：D:\isc)  
- GitHubよりisc-dpl-visualizerソースコードをcloneします  
> PS D:\isc> git clone https://github.com/ITDLab/isc-dpl-visualizer.git  
- isc-dplのBuild　で作成した　IscDpl.lib　を以下のフォルダーにコピーしておきます  
　isc-dpl-visualizer\third-party\itdlab\lib\debug  
　isc-dpl-visualizer\third-party\itdlab\lib\release  
- third-party\itdlab\include　を更新します  
  以下よりHeader File　を　third-party\itdlab\include　へコピーします  
  isc-dpl\source\include  
  isc-dpl\source\modules\IscDpl\include  
  isc-dpl\source\modules\IscDplC\include  
- cloneしたフォルダにあるCMakelists.txtを使用して、CMakeを実行することでプロジェクトを生成します  
    - PowerShellでd:\isc\isc-dpl-visualizerフォルダに進み以下のコマンドを入力します  
      パスは、実際の環境に合わせます  
> PS D:\isc\isc-dpl-visualizer> cmake -BD:\isc\isc-dpl-visualizer\build -DCMAKE_TOOLCHAIN_FILE=D:\pcl\vcpkg\scripts\buildsystems\vcpkg.cmake  
- D:\isc\isc-dpl-visualizer\build 以下にソリューションファイルが作成されます   
- ソリューションファイルをVisual Studioで起動し、buildします  

- 実行に必要なファイルを実行フォルダへコピーします  
    - isc-dpl:  
      IscDpl.dll/IscDplMainControl.dll/IscCameraControl.dll/IscDataProcessingControl.dll/  
      IscDisparityFilter.dll/IscFrameDecoder.dll/IscStereoMatching.dll/K4aSdkWrapper.dll/  
      VmSdkWrapper.dll/XcSdkWrapper.dll/IscUtility.dll  

    - 使用するカメラのSDK:  
      [VM]ISCLibvm.dll/ISCSDKLibvm200.dll/  
      [XC]FTD3XX.dll/ISCLibxc.dll/ISCSDKLibxc.dll/  

    - OpenCV:  
      isc-dpl　と共通で使用するOpenCVのDLL

    - isc-dpl用設定ファイル  
      （共通）  
      DPLGuiConfig.ini  
      （使用するカメラ）  
      DisparityFilterParameter_[VM/XC].ini  
      FrameDecoderParameter_[VM/XC].ini  

## サンプルアプリケーションの実行  
- DPLGuiConfig.ini の使用カメラ設定を確認します
    - [CAMERA]  
      ENABLED=1  
      CAMERA_MODEL=1 (VM:0 XC:1)  

- dpl_visualizer.exe を実行します  

## サンプルアプリケーションの操作
- 2D表示  
  Grabを選択すると、取り込みと表示を開始します  
- 3D表示  
  3Dを選択し、Grabを選択すると、取り込みと3D表示を開始します  
  Based on Heat Mapを選択すると、距離を色のグラデーションとして表示します  
  Full Screenを選択すると、最大（1920x1080)　で表示します  
- Select Function  
  - Stereo Matching: Software stereo matching　を行います  
  - Disparity Filter: Disparity Filterを有効とします  
  - Color Image: （可能であれば）Color画像を表示します  
  - Camera Control: カメラの露光モードなどを設定します  
  - PCL Filter: Point cloud Library　の機能を使ったフィルタを適用します  
    - Pass Through Filter: 表示する距離の範囲を設定します  
    - Down Sampling: ボクセル内の点は1つを除いて処理されます  
    - Radius Outlier Removal: 指定された半径内に指定した近傍数より少ないポイントが見つかった場合は、それらを削除します  
    - Plane Detection: 平面上にあるポイントを検出します  

****
## 使用上の注意
****
- PCLによる3D表示時に、Windowのサイズを変更しないでください 表示が停止します  
  最大化は、操作パネルより可能です  

****
# Project structure
****

****
# Manuals
****

****
# License  
****
This software is licensed under the Apache 2.0 LICENSE.

> Copyright 2023 ITD Lab Corp. All Rights Reserved.  
>    
> Licensed under the Apache License, Version 2.0 (the "License");  
> you may not use this file except in compliance with the License.  
> You may obtain a copy of the License at  
>    
> http://www.apache.org/licenses/LICENSE-2.0  
>    
> Unless required by applicable law or agreed to in writing, software  
> distributed under the License is distributed on an "AS IS" BASIS,  
> WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  
> See the License for the specific language governing permissions and  
> limitations under the License.  
    
****  

# Other Libraries  
- Dear ImGui (*Included in project*)  
Dear ImGui is licensed under the MIT License, see LICENSE.txt for more information.  

- Point Cloud Library  
PCL is released under the terms of the BSD license.  

- GLFW  
GLFW is licensed under the zlib/libpng license.  

- GLEW  
GLEW is originally derived from the EXTGL project by Lev Povalahev. The source code is licensed under the Modified BSD License, the Mesa 3-D License (MIT) and the Khronos License (MIT).  
The automatic code generation scripts are released under the GNU GPL.

- OpenCV  
OpenCV 4.5.0 and higher versions are licensed under the Apache 2 License.

- FTDI  
[Future Technology Devices International Limited](https://ftdichip.com/)

*end of document.*  
