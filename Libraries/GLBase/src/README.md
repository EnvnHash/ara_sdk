# GLBase

OpenGL Utilities and wrapper classes / Window Management

## Assimp

Wrapper class for <https://github.com/assimp/assimp> swiss army knife library for 3d model import / export

## GeoPrimitives

GeoPrimitive base class and derivatives for drawing basic geometries. Wraps around a VAO, used for drawing. Meant to be used for one-time setup on CPU, upload to GPU and static drawing onwards. 

## Meshes

Mesh definition class for general purposes, mean to reside on CPU, to be manipulated on CPU and used for dynamic drawing.

## Res (will be replaced in future releases)

Resource management. Loads images and fonts from the CMRC packed Assets folder (memory).

Contains a Parser for the ara sdk custom json-like Style format (UI Elements)

## Shaders

- Wrapper class for OpenGL Shaders
- ShaderCollector
  -Container class for shaders for global management (resource optimization)
  - Contains standard shaders (simple color, texture shaders, or shaders for clearing the screen - possibly faster than glClear).
  - Management in the form of a map<string, Shaders>
  - A name must be assigned to new shaders; if it already exists, it will be aborted.
- ShaderProperties and MaterialProperties

## Utils

- Wrappers for OpenGL Vertexes, FBOs, VAOs, TFOs, PBOs
- Font Management
- Texture Management
- TrackBall
 
## WindowManagement

- Base GLWindow class with derivatives for GLFW, WGL, GLX, EGL and X11