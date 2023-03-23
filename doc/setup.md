# Linux Setup
1. Install **Ninja Build**
    ```
    sudo apt-get install ninja
    ```
2. Install **CMake** version 3.24 or later, and ensure cmake command is in your path.
    ```
    sudo apt-get install cmake
    cmake --version
    ```
    If your version **is not 3.24 or later**, follow these additional instructions:
    ```
    sudo apt purge --auto-remove cmake
    sudo apt-get update
    sudo apt-get install apt-transport-https ca-certificates gnupg software-properties-common wget
    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | sudo apt-key add -
    sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main'
    sudo apt-get update
    sudo apt-get install cmake
    cmake --version

    ```
3. Install **LLVM compiler infrastructure**
    ```
    sudo apt-get install clang lldb lld
    ```
4. Install **Git** if not installed
    ```
    sudo apt-get install cmake


# Windows Setup
1. Install **choco** if not installed. Instructions: https://docs.chocolatey.org/en-us/choco/setup
2. Install **Ninja Build**
    ```
    choco install ninja -y
    ```
3. Install **CMake** version 3.24 or later,
    ```
    choco install cmake -y
    cmake --version
    ```
    Ensure cmake command is in your path.
4. Install **LLVM compiler infrastructure**
    ```
    choco install llvm -y
    ```
5. Install **Git** if not installed
    ```
    choco install cmake -y
    ```
6. Install **Visual Studio 2022 Community Edition** if VS is not installed.  This is needed for Windows SDK and other dependencies.
    ```
    choco install visualstudio2022community -y
    ```
    After initial installation, ensure that C++ development is enabled in **Visual Studio Installer**.
