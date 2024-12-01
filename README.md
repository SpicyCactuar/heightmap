# Heightmap

OpenGL application displaying a textured heightmap.
An `nPoints` sized triangle strip is generated at runtime. For each vertex, its tangents and UVs are computed. 
The information is fed into a basic graphics pipeline (`.vert` + `.frag`) that displaces the vertices 
and subsequently Blinn-Phong shades the resulting fragments. Three terrain textures are mixed according to height.
A single directional light illuminates the scene.

![heightmap](https://github.com/user-attachments/assets/893ab962-5d3c-440b-b562-1c01093ce4b6)

## Project Structure

```plaintext
heightmap/
├── src/                 # Source code, including shaders
├── external/            # Bundled libraries' source code (GLFW, GLEW, GLM)
├── assets/              # Static assets (.bmp files)
├── premake5.lua         # Premake 5 config
├── premake5             # Premake 5 executable (Unix)
├── premake5.exe         # Premake 5 executable (Windows)
└── README.md            # Project README
```

## Build - Make

```shell
./premake5 gmake2
make [config={debug_x64|release_x64}]
```

The `config` parameter defaults to `debug_x64`.

## Build - Visual Studio

```shell
./premake5.exe vs2022
```

Open generated `.sln` project file.

## Build - Xcode

```shell
./premake5.apple xcode4
```

Open generated Xcode project.

## Run

```shell
bin/heightmap-{target}.exe
```

Executables have `.exe` extension for all platforms, but binaries are platform-specific.

## Controls

| Key(s)                  | Action                                |
|-------------------------|---------------------------------------|
| `↑` / `↓` / `←` / `→`   | Move forward, back, left, and right   |
| `Space`                 | Toggle wireframe visualisation mode   |
| `N`                     | Toggle normal visualisation mode      |
| `T` / `S`               | Scale height up and down              |
| `W` / `A` / `S` / `D`   | Rotate directional light              |
| `Esc`                   | Close the application                 |

## Technologies

* **Premake**: `5`
* **C++**: `>= C++17`
* **OpenGL**: `>= 4.2`
* **GLFW**: `3.4.0`
* **GLEW**: `1.13.0`
* **GLM**: `0.9.7.1`
