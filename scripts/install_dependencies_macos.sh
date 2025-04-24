#!/bin/bash

# Script to install dependencies using Homebrew on macOS 14 (Sonoma)

set -e  # Exit immediately if a command exits with a non-zero status.

echo "Updating Homebrew..."
brew update

echo "Installing libfreeimage..."
brew install freeimage

echo "Installing GLEW..."
brew install glew

echo "Installing GLFW3..."
brew install glfw

echo "Installing Assimp..."
brew install assimp

echo "Installing GLM..."
brew install glm

echo "Installing libcurl4..."
brew install curl  # Homebrew's 'curl' provides libcurl4 functionality. No separate libcurl4 package needed.

echo "Installing OpenSSL..."
brew install openssl

brew install pugixml

echo "Dependencies installed successfully!"
