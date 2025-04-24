# SceneGraph

- [AssimpImport](#assimpimport)
- [CameraSets](#camerasets)
- [Gizmos](#gizmos)
- [Light](#light)
- [Scene3D](#scene3d)
- [SceneNodes](#scenenodes)
- [ShaderPrototypes](#shaderprototypes)
- [ShadowMap](#shadowmap)

## 3D SceneGraph

### AssimpImport

- Wrapper for libAssimp
- Requires a RootNode as an argument (from wherever you want in your Tree)
- Inserts all meshes from the source file as children of the passed-in RootNode
- Optionally, a lambda function can be passed in that receives a std::list of newly created nodes as an argument
- Can optionally normalize the loaded mesh to size (1|1|1) (normalized on the GPU via Compute-Shader)

### CameraSets

- General container for virtual cameras
- Via Geometry Shader, multiple camera (model-view-projection) matrices can be rendered to one or more target framebuffers, so it makes sense to have a class that manages this process
- A CameraSet can contain one or more target FBOs
- The GLMCamera class is used for managing individual sets of camera matrices (Model, view, projection)
- Contains optional functionality for use as "Trackball" (changing the camera perspective via mouse)
- The "Trackball" essentially consists of a position (x, y, z) and a rotation (Euler angles)
- Position (translation) and rotation are multiplied onto the model matrix of the GLMCamera
- The parameters camPos (0|0|1), lookAt (0|0|0), and upVector (0|1|0) of the view matrix remain unchanged. This means all screen position calculations must take this transformation into account, multiplying it with the inverse camera model matrix to get its position in 3D space within the scene
- TrackBall rotates always relative to the center (0|0|0) of the Camera Model-Matrix
- Each CameraSet must be assigned at least one Shader prototype (method addShaderProto("ShaderProtoName", std::list<renderPasses>)), this uses the ShaderProtoFact class
- Shader prototypes are render-pass specific, with the list passed to addShaderProto defining for which render passes the relevant shader should be used.

### Gizmos

- Entry point is SPObjectSelector. In OBJECT_MAP_PASS, all objects in the scene are rendered into an R32F buffer with their ID as "color". When you click with the mouse, a point is drawn at that location and the color value (ID) at that location is drawn into an FBO. The texture bound to this, which is only 1 pixel large, is then loaded as a float and cast as an int.
- If an object exists with the found ID, one of the three gizmos (translation, scaling, rotation) becomes visible, and its position and rotation are adjusted to match the object's model matrix. The size of the Gizmo is fixed relative to the screen size and is dynamically adjusted in the Gizmo's draw() function.
- Since Gizmos must always be visible, they are rendered in a separate SceneTree (gizmoTree), which is rendered after clearing the depth buffer and rendering the scene tree.
- Two different models are used for display and rendering into the object map. Simple cubes are used for the object map that are slightly larger than the arrow models to have some "error tolerance" when clicking with the mouse.
- Moving an object works as follows:
  - The ObjectSelector determines the ID of the object at the mouse pointer position on MouseDown
  - It saves the current mouse position, and the current model matrices for both the Gizmo and the object
  - Always what is sought after is the position in 3D space projected to NDC at the current mouse position, along the axis (or plane) it will be moved. To find this, the inverse process is taken. Each mouse position in NDC (2D space of the window) corresponds to a line in 3D space where the desired target position is located. This line can be determined by the camera origin point and the normalized x-, y- mouse position combined with the z-position = "near" value of the camera frustum, which intersects with the plane containing the object axis it will move along and that is as parallel to the screen plane as possible.

### Light

- Derived from SceneNodes, defines everything that emits light in the scene
- To create a new Light, an instance must be created, added to the SceneTree using addChild(), and added to the CameraSet's ShaderPrototype using addLight().
- Light objects can load external 3D models that are added to the LightNode using addChild(). There must only be one sub-level of nodes to a LightNode. LightNodes have the NodeType PD3D_SNT_LIGHT. Subnodes for display carry the type PD3D_SNT_LIGHT_SCENE_MESH.
- 3D models for LightNode must always be created centered on (0|0|0).

### Scene3D

- "Root" object for the framework
- Contains scene-wide tools like BoundingBoxer, ShaderCollector, etc.
- Contains the actual render loop:
  - There are various render passes:
    - Object_Map_Pass: objects are rendered into an R32F texture; their ID corresponds to their "color"
    - SHADOW_MAP_PASS: the scene is calculated from the perspective of the light sources. The resulting depth textures will be used later to compare whether there is a pixel from the perspective of each respective light that lies in front of the one being rendered, i.e., it blocks it
    - SCENE_PASS: the actual pass to generate visible results using ShaderPrototypes
    - OBJECT_ID_PASS: Reassignment of SceneNode IDs
  - Render passes have a fixed order. For example, the SHADOW_PASS must be executed before the SCENE_PASS so that lights and shadows can be displayed correctly
  - Order is: CameraSet → ShaderPrototype → SceneNode

### SceneNodes

- Basic element; anything that has meshes
- Can have SubNodes (children)
- Rendered recursively
- Have model matrices (position, scaling, rotation)
  - Matrices are hierarchically multiplied (from root node to SubNode); when rendering upon request (i.e., the "hasNewModelMat" flag is set somewhere in the NodeTree), a matrix stack (std::list<mat4>) is maintained, where the current parent matrix is always the multiplication of all current matrices in the list.
  - To save computation time, this multiplication is only performed if the model matrix of the node or its parent has changed.
  - To prevent duplicate multiplications, this calculation is centrally executed at the beginning of the render loop. When changes occur (translate, rotate, scale), the flag "hasNewModelMat" is set in the node. In the render loop, each node checks for this flag. If found, it is removed from the current node and set for all children.
  - Callbacks can be set that are called when the model matrices change (also centrally in the render loop)
  - SceneNodes then have a modelMat = relative to parent and an absModelMat used for rendering
- In addition to the model matrices, there is the "dimension" attribute which refers to absolute world coordinates
- Have types (Standard, Light, etc.) :
  - SNT_STANDARD: a standard visible mesh
  - SNT_LIGHT: a light emitting object, typically not visible inside the scene, but may contain 
   children with visible meshes; this should have the type SNT_LIGHT_SCENE_MESH
  - SNT_LIGHT_SCENE_MESH: a visible mesh representing the light object (Projector, Spotlight, etc)
- Have unique IDs (integer from 1 - … )
- here is a map<renderPass, bool> to enable/disable a node for a specific render pass 
- A callback can be set that gets called when the position of the node in the tree changes (setParentNode())

### ShaderPrototypes

- General class for instantiating shaders with which a scene can be rendered

### ShadowMap

- As the name says, generators for ShadowMaps, typically textures of type DEPTH_BUFFER_32F (possibly with layers)
- Owns at least one FBO to which the DepthBuffer is bound
- Owns at least one shadowShader (possibly with GeometryShaders for parallel computation of multiple lights, e.g., ShadowMapArray)
- The simplest form of shadow calculation is to render the scene from the light's perspective and store the resulting depth values in a texture. Then, when rendering, for each vertex, its distance from the light source is calculated and compared with the value at the corresponding position in the depth texture, which is obtained by transforming the vertex position into NDC using the light coordinate system. OpenGL provides the special sampler type sampler2DShadow to bind depth textures to it. The command textureProj, which passes the texture unit and the transformed vertex position in the light coordinate system to it, returns a float value between 0-1 directly, corresponding to the resulting brightness value.

### ShadowMapVsmArray

- VSM (Variance Shadow Mapping), instead of creating a depth texture with absolute values, an RG32F texture with the distribution or "density" of these values is set up. This can be smoothed in contrast to simple depth values.
