#!/bin/bash

# Build script for Relay PC
echo "Building Relay PC with Qt6..."

# Clean previous build
if [ -f "Makefile" ]; then
    echo "Cleaning previous build..."
    make clean > /dev/null 2>&1
fi

# Generate Makefile with Qt6
echo "Generating Makefile..."
qmake6 relay-pc.pro

if [ $? -ne 0 ]; then
    echo "Error: Failed to generate Makefile. Make sure Qt6 is installed."
    exit 1
fi

# Build the project
echo "Compiling..."
make -j$(nproc)

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ Build successful!"
    echo "Run './relay-pc' to start the application."
else
    echo ""
    echo "❌ Build failed!"
    exit 1
fi
