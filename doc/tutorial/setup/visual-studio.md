---
title: Build in Visual Studio
description: This tutorial shows how to build qiotoolkit.
ms.topic: article
uid: microsoft.azure.quantum.qiotoolkit.tutorial.setup.visual-studio
---

Build in Visual Studio
======================

1.  Install Boost (need to do it once):
    * Download boost source  current .zip https://www.boost.org/users/download/  
    * Create Boost directory and unzip it in some directory: 
        `C:\Boost\boost_1_76_0`
    * Open Developer Command Prompt for VS from Windows Start menu (part of Visual Studio folder).
    * Go to boost directory and build boost with following commands:  
```cmd
> cd C:\Boost\boost_1_76_0
> bootstrap.bat
> b2
```        
2. Install Protobuf (Need to do it once. This will install protobuf version 3.14.0)
    Install vcpkg:
        git clone https://github.com/microsoft/vcpkg
        .\vcpkg\bootstrap-vcpkg.bat  
    Install Protobuf using vcpkg:
        .\vcpkg\vcpkg install protobuf protobuf:x64-windows
```cmd
> git clone https://github.com/microsoft/vcpkg
> .\vcpkg\bootstrap-vcpkg.bat  
> .\vcpkg\vcpkg install protobuf protobuf:x64-windows
```
3.  Generate Visual Studio solution:
    * Open Developer Command Prompt for VS from Windows Start menu (part of Visual Studio folder).
    * Add protobuf directory to CMAKE_PREFIX_PATH environment variable
    * Create `ms_build` directory for Visual studio solution in `qiotoolkit/cpp`. 
    * Run cmake (with directory for cpp's, boost root and protobuf root).

```cmd
> cd path_to\qiotoolkit\cpp
> mkdir vs_build
> cd vs_build
> cmake .. -DBOOST_ROOT="C:\Boost\boost_1_76_0" -DCMAKE_PREFIX_PATH="path_to\vcpkg\packages\protobuf_x64-windows\Debug\bin"
```        
Note: To run in release mode, use: 
cmake .. -DBOOST_ROOT="C:\Boost\boost_1_76_0" -DCMAKE_PREFIX_PATH="path_to\vcpkg\packages\protobuf_x64-windows"

You can find the script in src/Tools of this repo.

4.  Build Visual Studio solution:
    * Open generated qiotoolkit.sln in Visual Studio: `path_to\qiotoolkit\cpp\vs_build\qiotoolkit.sln`
    * (Press OK on dublicate run_tests project warning).
    * Select target build and build. 


