# Soft Trace

OpenGL application displaying a textured heightmap.
An `nPoints` sized triangle strip is generated at runtime. For each vertex, its tangents and UVs are computed. 
The information is fed into a basic graphics pipeline (vert + frag) that displaces the vertices 
and subsequently Blinn-Phong shades the resulting fragments. Three terrain textures are mixed according to height.
A single directional light illuminates the scene.

## Project Structure

```plaintext
heightmap/
├── src/                 # Source code, including shaders
├── external/            # Bundled libraries' source code (GLFW, GLEW, GLM)
├── assets/              # Static assets (.bmp files)
├── _build_/             # Generated build files
├── lib/                 # Generated lib files
├── bin/                 # Generated executable files
├── premake5.lua         # Premake 5 config
├── premake5             # Premake 5 executable (Linux)
├── premake5.exe         # Premake 5 executable (Windows)
└── README.md            # Project README
```

## Environment - Linux

The following packages are required:

```
libxcursor-dev
libx11-dev
libxrandr-dev
libxinerama-dev
libxi-dev
```

## Build

```bash
./premake5 gmake2
make
```

## Run

```bash
bin/main-debug-<arch>-<compiler>
```

Example:

```shell
bin/main-debug-x64-gcc
```

## Technologies

* **Premake**: `5`
* **C++**: `>= C++17`
* **OpenGL**: `>= 4.2`
* **GLFW**: `3.4.0`
* **GLEW**: `1.13.0`
* **GLM**: `0.9.7.1`

## Showcase

![heightmap](https://github.com/user-attachments/assets/893ab962-5d3c-440b-b562-1c01093ce4b6)
