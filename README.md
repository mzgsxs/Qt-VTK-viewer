# Simple VTK 3d viewer
A simple VTK 3D viewer with Qt framework.


## Installation
Only provide macOS setup, but it should compile in windows easily.
1. Install vtk, now comes with default Qt integration
```sh
brew install vtk
```

2. Qt installation via brew or from official [website](https://www.qt.io/download-dev) 
```sh
brew install qt
```

3. clone this project
```
git clone ...
```

4. Build project.
```
cmake -S . -B build
cmake --build build
./build/MM804A1
```


## License
Apache License 2.0

## Acknowledgments
Based on this [project](https://github.com/martijnkoopman/Qt-VTK-viewer).
