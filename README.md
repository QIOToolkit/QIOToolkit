# Introduction 
This project provides the components for building and running Quantum Inspired Optimization solvers. These methods can be used to solve different types of optimization problems that can be expressed as unbounded binary optimization problems.

# Getting Started
1.	Installation process
## Linux / WSL
If running wsl, you may need to convert the setup bash file to run with linux file endings. To do that:
    1. Install dos2unix: sudo apt-get install dos2unix
    2. Assuming you are in the project directory, run: dos2unix ./setup/linuxsetup.h 

You will need admin permission for setup.
Run
1. ./setup/linuxsetup.sh 
    Note: Please install and run dos2unix ./linuxsetup.h in case running from wsl, to avoid windows line endings related errors. 

This process will take few minutes to complete.

## Windows

### Requirements:

* [Visual Studio 2022/2019](https://visualstudio.microsoft.com/)
* [Git for Windows (if not already installed via VS)](https://gitforwindows.org/)
* [7zip](https://www.7-zip.org/)
> Why 7zip? From experience, the Windows zip tool tends to crash while trying to unzip the downloaded Boost folder
Recommended Azure VM image: Visual Studio 2019 or 2022 on Windows 10/11 or Windows Server

### Installation

1.  Install Boost (you only need to do this once):
    * Download Boost source current .zip: https://www.boost.org/users/download/  
    * Create Boost directory in a location of your choice and use 7zip to unzip the downloaded file there, for example:

        `C:\Boost\boost_1_81_0`

    * Open Developer Command Prompt for VS from Windows Start menu (part of Visual Studio folder).
    * Navigate to the Boost directory and build Boost with following commands (note: the `b2` step will probably take a long time):  

        ```cmd
        > cd C:\Boost\boost_1_81_0
        > bootstrap.bat
        > b2 
        
        ```
2. In Powershell, navigate to the QIOToolkit repo directory and then run `./setup/windowssetup.ps1` (this will also take a long while)


2.	Software dependencies
3.	Latest releases
4.	API references

# Build and Test
## Linux / WSL
1. After installation is complete, the make and make test commands will be run from the linuxsetup.sh file

## Windows

# Contribute
TODO: Explain how other users and developers can contribute to make your code better. 
