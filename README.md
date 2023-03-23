# Priml Programming Language
## Introduction


## Installing dependencies

[Dependencies Setup](doc/setup.md)

## Building the compiler
After installing the dependencies, you can build the compiler from the source code. Following scripts are provided to make it easy to build and install the compiler.

| Scripts | |
| - | - |
| cm.sh cm.cmd | Configures the CMake files to build using Ninja and CLang. Only need to run once |
| bld.sh bld.cmd | Builds the compiler and tests. Can specify 'Debug' (default), 'Release', 'RelWithDebInfo' or 'MinSizeRel'  |
| install.sh install.cmd | Installs the compiler.  Must be run from "Administrative" prompt |

### On Windows:
```cmd
git clone https://github.com/primllang/pc.git
cd pc
script\cm.cmd
script\bld.cmd
script\install.cmd
```
### On Linux:
```bash
git clone https://github.com/primllang/pc.git
cd pc
chmod +x script/*.sh
script/cm.sh
script/bld.sh
script/install.sh
```
## Running the compiler
See usage:
```
pc -?
Usage:
    new <unit> [--exe or --lib] [--nogit]
    build [<unit>] [<build type>]
    run [<unit>] [<build type>]
    clean <units> ...

Build types:
    --debug
    --release
    --relwithdeb

Other options:
    --verbose
    --cleanfirst
    --hard
    --checkdeps
    --diagfiles
    --savetemps
    --config <configfile>
    -?, --help
```
Create a new executable **unit** and initialize the git repo (default):
```
pc new --exe myexe
```
Create a new library **unit** with without git:
```
pc new --lib mylib --nogit
```
Build a **unit** by specifying the name of the directory where the unit is stored. If the directory is ommited, current directory is used.  By default this builds the debug build type:
```
pc build myexe
```
Build a release build:
```
pc build myexe --release
```
Run the release build type of an executable **unit** using pc tool.
```
pc run myexe --release
```
## The **unit.yaml** file
Each unit has a yaml file that describes the unit, source files and  dependencies, and extended libraries written in cpp.
```yaml
unit:
  name: helloworld
  type: exe
  version: 0.1.1
  edition: 2023
src:
  - main.pc
deps:
  - base:
      namespace: global
ext:
```

![](doc/primllogo.jpg)
