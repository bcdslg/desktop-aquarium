# Desktop Aquarium

Desktop Aquarium is a tiny Windows desktop companion that renders animated fish on top of your desktop. It uses a transparent layered window, Direct2D drawing, schooling behavior, solo fish, food particles, and a tray menu for quick controls.

![Desktop Aquarium icon](resources/app.ico)

## Features

- Transparent always-on-top desktop aquarium.
- Sticker-style fish with different sizes, colors, and swimming personalities.
- Mixed behavior: fish schools plus independent solo fish.
- Feeding mode: click to drop food; nearby interested fish speed up and swim toward it.
- Tray menu controls for fish count, school count, speed, size, feeding mode, and quit.
- Native Win32/C++ implementation with Direct2D.

## Requirements

- Windows 10 or Windows 11.
- Visual Studio 2022/2026 with C++ desktop development tools, or compatible MSVC toolchain.
- CMake 3.20 or newer.

## Build

From a Visual Studio Developer Command Prompt:

```bat
cmake -S . -B build
cmake --build build --config Release
```

Or run:

```bat
build.bat
```

The executable will be generated at:

```text
build\Release\DesktopAquarium.exe
```

## Usage

Run `DesktopAquarium.exe`. Use the tray icon menu to:

- Toggle feeding mode.
- Change fish count.
- Change school count.
- Change swimming speed.
- Change fish size.
- Quit the app.

When feeding mode is enabled, click on the desktop to drop food particles.

## Project Structure

```text
src/          Win32, Direct2D, fish, food, tray, and mouse hook code
resources/    App icon, manifest, and Windows resource file
CMakeLists.txt
build.bat
```

## Notes

This is a Windows-only desktop app. It creates a transparent click-through overlay window and uses a global mouse hook for feeding mode.

## License

MIT License. See [LICENSE](LICENSE).
