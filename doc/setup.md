# Linux Setup
1. Install **Ninja Build**
    ```bash
    sudo apt-get install ninja
    ```
2. Install **CMake** version 3.24 or later, and ensure cmake command is in your path.
    ```bash
    sudo apt-get install cmake
    cmake --version
    ```
    If your version **is not 3.24 or later**, follow these additional instructions:
    ```bash
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
    ```bash
    sudo apt-get install clang lldb lld
    ```

# Windows Setup
## Option 1: Using winget
1. Install **Ninja Build**
    ```cmd
    winget install ninja
    ```
2. Install **CMake** version 3.24 or later,
    ```cmd
    winget install cmake
    cmake --version
    ```
    Ensure cmake command is in your path.
3. Install **LLVM compiler infrastructure**
    ```cmd
    winget install llvm
    ```
4. Install **Visual Studio 2022 Community Edition** if VS is not installed.  This is needed for Windows SDK and other dependencies.
    ```cmd
    winget install Microsoft.VisualStudio.2022.Community
    ```
    After initial installation, ensure that C++ development is enabled in **Visual Studio Installer**.

## Option 2: Using choco
1. Install **choco** if not installed. Instructions: https://docs.chocolatey.org/en-us/choco/setup
2. Install **Ninja Build**
    ```cmd
    choco install ninja -y
    ```
3. Install **CMake** version 3.24 or later,
    ```cmd
    choco install cmake -y
    cmake --version
    ```
    Ensure cmake command is in your path.
4. Install **LLVM compiler infrastructure**
    ```cmd
    choco install llvm -y
    ```
5. Install **Visual Studio 2022 Community Edition** if VS is not installed.  This is needed for Windows SDK and other dependencies.
    ```cmd
    choco install visualstudio2022community -y
    ```
    After initial installation, ensure that C++ development is enabled in **Visual Studio Installer**.
