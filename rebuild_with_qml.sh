#!/bin/bash
# Clean and rebuild script for QML resource integration

echo "=== Cleaning previous build ==="
cd /home/user/El-7aress-Project

# Remove old build artifacts
rm -rf build/
mkdir build
cd build

echo "=== Running qmake ==="
qmake ../root\ .pro

echo "=== Building project ==="
make clean
make -j$(nproc)

echo "=== Build complete ==="
echo "Check for 'qrc_qml.cpp' in the build directory to verify resources were compiled"
find . -name "qrc_*.cpp" | head -5
