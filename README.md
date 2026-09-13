# SHUFF - A Simple Huffman encoder

## Table of contents

- [What is this?](#what-is-this)
	- [Header structure](#header-structure)
	- [Dependencies](#dependencies)
- [Usage](#usage)
- [Installation](#installation)
	- [Dependencies](#dependencies-1)
	- [Install dependencies](#install-dependencies)
		- [Linux / macOS](#linux--macos)
		- [Windows](#windows)
	- [Install](#install)
	- [Without Make](#without-make)
		- [Linux / macOS](#linux--macos-1)
		- [Windows](#windows-1)
- [Development](#development)
	- [Project structure](#project-structure)
	- [Development builds](#development-builds)
- [Contributing](#contributing)
- [Licensing](#licensing)

# What is this?

```shuff``` is a tool to encode and decode files with the Huffman Coding algorithm.

When encoding a file, it will store a small header with the encoded data following.

## Header structure

| Number |  Byte   | Corresponding data        |
|:------:|:-------:|---------------------------|
|   1    | 01 - 04 | Magic Number              |
|   2    |   05    | Version number            |
|   3    | 06 - 13 | Size of the original data |
|   4    | 13 - 21 | Size of the encoded data  |
|   5    |   22    | Amount of unique bytes    |
|   6    |   23+   | Counts for each byte      |

1. `Magic number` - The exact character sequence `'shuf'`, used to identify that this file is valid encoded data of this
   file type.
2. `Version number` - Currently, only version 1 exists. If the header should change in the future, this number may be
   increased.
3. `Size of the original data` - This is used for both integrity checks and important in decoding to figure out the
   length of the byte. Stored as an unsigned 64-bit Integer.
4. `Size of the encoded data` - This is purely for integrity checks to make sure that the encoded data has not been
   modified. Stored as an unsigned 64-bit Integer.
5. `Amount of unique bytes` - This denotes the amount of unique bytes in the original data and allows to calculate the
   length of the header.
6. `Counts for each byte` - This part of the header describes the amount of occurrences of each byte in the original
   data. In the header only bytes that actually appear in the data are stored to reduce header size. This table always
   denotes the byte, followed by an unsigned 64-bit Integer noting the amounts in the original data. This way, a Huffman
   Tree can be created on decode.

In total, the header size can vary from 22 Bytes to 2,326 Bytes, depending on the amount of unique Bytes.

## Dependencies

This program has **no** runtime dependencies. Build dependencies are listed [below](#dependencies-1)

# Usage

Using shuff is very simple.

```bash
shuff [<action>] [<input>] [<output (optional)>]
```

- `action` - This can be either `encode` or `decode` depending on whether you want to encode or decode a file.
- `input` - This needs to be a valid OS path to an existing file. Permission issues may lead to unexpected errors. Make
  sure to execute the program with higher privileges if the file is read-protected.
- `output` - This needs to be a valid os path to a file. If the file already exists, it may be overwritten without
  confirmation. If left unspecified, the program will append `'.shuf'` to the end of the file. On decode, the program
  will try to remove the `'.shuf'` extension again. If not found, it will default to
  `"<original filename> (1).<original extension>"`.
- Using the `-h` or `--help` flag will print a usage description.


- On Windows, you may have to use `shuff.exe` instead.

# Installation

## Dependencies

- [CMake](https://cmake.org/) - Required to build the program.
- [Clang](https://clang.llvm.org/) - Serves as a compiler and linker. This may be replaced by any other compiler, but
  compilation error may occur.
- [LLVM](https://llvm.org/) - Usually already bundled with clang. On Windows, Clang usually comes with the LLVM install.
- [Make](https://www.gnu.org/software/make/) - Used to simplify the build process. See [below](#without-make) for
  compilation without
  make.
- (On Windows) [PowerShell](https://learn.microsoft.com/powershell/scripting/install/install-powershell-on-windows) -
  Version 7 or newer is required for actual installation. Lower versions have some bugs.

## Install dependencies

### Linux / macOS

Dependencies may be installed with your package manager of choice. A lot of distros have base development packages that
may already include some of these tools.

On Arch Linux for example, you may use this command to install all required packages.

```bash
sudo pacman -S make cmake clang
```

### Windows

On Windows, I would recommend to use [Chocolatey](https://chocolatey.org/) for dependency installation.

(From an elevated shell)

```powershell
choco install cmake llvm make
```

Make sure to refresh your `$PATH` afterward.

## Install

The `Makefile` offers multiple different build options.

- `install` - Will build the target executable and copy it to `/usr/bin` on Linux, `/usr/local/bin` on macOS and to
  `C:\Program Files\shuff` on Windows. On Windows, it will also add `shuff` to path. This requires elevated privileges.
- `release` - Will build the executable with Release optimizations. Recommended for portable use.
- `debug` - Will build the executable with Debug optimizations. Only recommended for development.
- `clean` - Will clean build directories.
- `remove` - Will remove any installed version of `shuff`.

For actual installation please use

```bash
sudo make install
```

If you want to just build the executable, use

```bash
make release
```

instead.

## Without Make

When not using Make, you can build the program for release with these commands:

### Linux / macOS

```bash
cmake -S . -B ./build -DCMAKE_BUILD_TYPE=Release
cmake --build ./build
```

### Windows

```powershell
cmake -S . -B .\build -G "Ninja"
cmake --build .\build --config Release
```

(replace `Ninja` with another generator of your choice if you want.)

You can then find the built executable in the `build` directory.

Installation has to be done manually. For that just copy the executable in a directory on `$PATH` or add its path to
`$PATH` on Windows.

# Development

## Project structure

```
.
├── src
│   ├── huffman.c
│   ├── huffman.h
│   └── main.c
├── .editorconfig
├── .gitattributes
├── .gitignore
├── CMakeLists.txt
├── LICENSE
├── Makefile
└── README.md
```

The code for the actual Huffman Coding algorithm is located in `huffman.c`, with the `huffman.h` header file denoting
all outward functions.
The `main.c` file contains file handling logic and the input parsing.

## Development builds

Using the `Makefile`, you can use

```bash
make debug
```

to build an executable with included debug symbols. This will also copy the executable to the project root.

In order to clear the build files, use

```bash
make clean
```

to remove the `./build` folder and the executable from project root.

# Contributing

Any issues and pull requests are always appreciated. If you have found a bug or just have a feature request, please do
not hesitate to open an issue on GitHub.

# Licensing

This project is licensed under the [GPL-v3 License](https://www.gnu.org/licenses/gpl-3.0.en.html).

Please regard the [LICENSE file](LICENSE) for further details.
