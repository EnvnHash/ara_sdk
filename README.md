#    Timeart Visualizer OpenGL 3.2+ framework (tav)


written in C++ / OpenGL

Building
========

tav uses CMake (mkdir build, cd build, cmake .., make)

Running
=======

tav needs a data folder in the users home directory called tav_data
copy the example setup.xml into this folder and run ./tav setup.xml

Features
========


Audio
-----
- AudioTextures
- FFTs (general and abstract, though also for images)
- Multichannel FFTs
- Multichannel Input via Portaudio (MIT, free for commercial use)
- Onset and Pitch Detection


Communication
-------------
- SerialPort/USB via Boost
- OpenSoundControl (full SceneNode Parameter-Abstraction for OSC with Median-Filtering)
- TCP/UDP client via Boost (Boost Licenses, commercial use posible)
- TCP/UDP server via Boost


Import
------
- 2D Image Import with Freeimage (GPL v2 and v3)
- 3D Model Import with Assimp (all popular formats, BSD 3-clause license)
- Fonts via FreeType
- Video Import via FFmpeg


Maths
-----
- 2D and 3D Splines
- 2D and 3D Path Interpolator
- Function-based Value Interpolation with Callback Functions
- GLM for all vector and geometrical CPU side calculations
- Matrix Stacks
- Median Filter
- Kalman Filter via OpenCV (3-clause BSD)
- Vector, Matrices and Quaternion via GLM (MIT license)


Shaders/Image Processing
------------------------
- 2D GPU Fluid Simulation (with 2D-FBOs)
- 3D GPU Fluid Simulation (with Multilayer 2D-FBOs)
- BumpMaps
- GPU Bayer Demosaicing
- GPU FastBlur Shader (all Formats)
- GPU Histogram (with Normalization, all GL-Formats)
- GPU Pseudo 2D OpticalFlow
- GPU Pseudo OpticalFlow for RGB-D Images
- GPU ParticleSystems with FBOs
- GPU ParticleSystems with TransformFeedback Buffers
- GPU ParticleSystems with ComputeShaders
- GPU Timeaxis Medianfilter
- GPU LightScattering
- GPU PerlinNoise
- Heightmap Shaders and Manipulators (various)
- LitSphere Shader
- OpenCV-based Image Contour-Extraction and Texture-Upload with object-specific Texture-Coordinates (3-clause BSD License)
- RayCasting
- SphereHarmonics
- Various audio-based Vertex-Shaders



OpenGL C++ Wrappers
-------------------
- FrameBufferObjects (all formats, colors and depths - also PingPongFBOs)
- Material and Light-Definitions (basic)
- ShaderAssembler (String-based GLSL Abstration)
- ShaderBuffers
- Shaders (with a ShaderCollector for memory optimization)
- TextureBuffers
- TransformFeebackObjects
- UniformBlocks
- VAO
- Vertices
- VertexAttributes


OpenGL Utilities
----------------
- Camera Abstraction (Model-View-Projection, Perspective, Ortographic, Frustum)
- CameraSets (GeometryShader-based Camera-Array rendering)
- Geometry Primitives (Circles, Lines, Cubes, segmentated Cubes, Disks, Lines, Quad, segmentated Quads, Spheres, Toruses)
- GeometryShader-based Path Extrusion (with modulation for all segments)
- FrameBufferObjects with shader-based 4 Point-Dedistortion, Cropping and Offset
- FPS Timer
- Meshes
- Multi-GPU contexes (via glfw)
- OpenCV-based Effect Chains (multithreaded)
- Path Rendering via NvPr
- ScreenSpacedAmbient Occlusion (100% on GPU)
- Shared GPU OpenGL contexes (via glfw)
- Shadow Maps (Standard OpenGL, ESM and VSM)
- ShaderPrototypes (runtime intialization)
- SkyBoxes/Environment Mapping
- Tesselation-Shader based Terrain-Rendering
- WindowManager (based on GLFW), runtime creation and destruction of window of any size and position (and though output)  


MotionCapturing/Tracking
------------------------
- 6 DOF Correction and Mapping of PointClouds
- ActivityRange-Tracking for (realtime)-VideoInputs, with ModelMatrix-Generation
- Background Substraction via DepthMap-Thresholds
- FaceDetection and Landmark Estimation via Dlib
- Freenect2 Wrapper
- General 2D and 3D Point Tracking with Kalman-Filtering (OpenCv-based)
- OpenCV-based Person Detection via CUDA
- OpenCv-based Optical-Flow via CUDA
- Repro-Tools
   - Automatic calibration and pose mapping
   - Camera-beamer calibration 2D (tested and prooved), 3D (almost done)
   - Chessboard-based beamer pose-estimation
   - Mapping of 3D models onto volumetrical objects
   - Mapping of 3D models onto volumetrical objects in movement
   - Saving and loading of calibrations via yml files
- Shader-based PointCloud to 2D mapping
- Shader-based Epipolar rectification
- Skeleton and Hand Poseestimation and tracking via NiTE2
- Wrapper for Kinect v1 and v2 (via OpenNI2) - event-based and threaded


SceneGraph
----------
- Full hierarchical Node-Trees with Modelmatrix inheritance
- Full XML-abstraction (Definition of Node-Trees and Parameters)
- SceneNodes (init, draw, update)
   - A SceneNode inherits a Modelmatrix, a Shader, a CameraSet and a TFO from its parent
   - A SceneNode can deactivate and reactivate all its inputs and though use the full OpenGL Functionality
   - A SceneNode can create and destroy windows
   - Each SceneNode can expose variables to OSC (per pointer), an osc server runs in a separate thread, all values get only updated if new values come in, but get median-filtered every update
   - SceneNode Morphing via TFOs


Typography
----------
- Vector-based Typography with NvPr, single-line and multiline texts, utf8 with formating
- Texture-based Typography with Freetype (GPLv2 and v3), single-line texts, utf8, basic formating


Video
-----
- Cuvid Video Textures, GPU based decodification (MPEG1,2,4)
- FFmpeg based MediaRecorder (GPU-CPU Download, threaded, almost all formats)
- Video4Linux Textures (threaded, camera control, etc.)
- VideoTextures via FFMpeg (threaded, all common formats)
