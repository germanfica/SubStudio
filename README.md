# SubStudio

## Build mingw_64

```bash
cmake --preset default
cmake --build build
```

## Build msvc2022

```bash
cmake --preset msvc2022
cmake --build build_msvc
cmake --build build_msvc --config Debug
```
