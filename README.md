# ProteusLab SDL

ProteusLab SDL is a C++20 desktop circuit editor and mixed-signal simulator. Its complete frontend is rendered with SDL2; Qt and other GUI frameworks are not used.

## Windows

Run PowerShell as Administrator once:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\setup_windows.ps1
```

Then build, test and run:

```bat
scripts\run_windows.bat
```

The executable is written to `build-windows\ProteusLabSDL.exe`.
The Windows build script pins CMake, Ninja, GCC/G++ and SDL2 to the same
`C:\msys64\ucrt64` toolchain, refreshes stale CMake caches, and copies the
matching runtime DLLs beside the executable.

## Highlights

- SDL2 schematic canvas, menus, toolbars, panels and vector text
- component library with filtering, preview and drag/drop
- grid snapping, zoom, pan, multi-select, rotate and mirror
- orthogonal wiring, dynamic endpoints and electrical junctions
- analog MNA solver and event-driven digital simulation
- RLC, sources, interactive devices, gates, DFF, ADC/DAC and MCU
- external memory, LCD, keypad, voltmeter, ammeter and oscilloscope
- Run/Pause/Stop/Step, live net colors, DRC and simulation log
- JSON Save/Load, recent projects, runtime-state persistence
- snapshot Undo/Redo and dependency-free PNG export

See [README_FA.md](README_FA.md) and the `docs` directory for the complete Persian documentation.
