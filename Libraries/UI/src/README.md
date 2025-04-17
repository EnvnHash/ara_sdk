# GLSceneGraph

- [3D SceneGraph](#3D-scenegraph)
  - [AssimpImport](#assimpimport)
  - [CameraSets](#camerasets)
  - [Gizmos](#gizmos)
  - [Light](#light)
  - [Scene3D](#scene3d)
  - [SceneNodes](#scenenodes)
  - [ShaderPrototypes](#shaderprototypes)
  - [ShadowMap](#ShadowMap)
- [2D UI System](#ui-system)
  - [User Interaction](#user-interaction)
  - [Drawing](#drawing)
  - [Basic Application Component Design](#basic-application-component-design)
  - [UIApplication in multithreaded mode](#uiapplication-in-multithreaded-mode)
  - [UIApplication in single thread mode](#uiapplication-in-single-thread-mode)
  - [Coordinate System](#coordinate-system)
  - [UI Graph](#ui-graph)
  - [Positioning](#positioning)
  - [UINoode Base class and derivatives](#uinode-base-class-and-derivatives)
  - [UINoode Tree processing](#uinode-tree-processing)
  - [HID Processing](#hid-processing)
  - [Styles](#styles)

## 3D SceneGraph
### AssimpImport
* Wrapper for libAssimp
* Requires a RootNode as an argument (from wherever you want in your Tree)
* Inserts all meshes from the source file as children of the passed-in RootNode
* Optionally, a lambda function can be passed in that receives a std::list of newly created nodes as an argument
* Can optionally normalize the loaded mesh to size (1|1|1) (normalized on the GPU via Compute-Shader)

### CameraSets
* General container for virtual cameras
* Via Geometry Shader, multiple camera (model-view-projection) matrices can be rendered to one or more target framebuffers, so it makes sense to have a class that manages this process
* A CameraSet can contain one or more target FBOs
* The GLMCamera class is used for managing individual sets of camera matrices (Model, view, projection)
* Contains optional functionality for use as "Trackball" (changing the camera perspective via mouse)
* The "Trackball" essentially consists of a position (x, y, z) and a rotation (Euler angles)
* Position (translation) and rotation are multiplied onto the model matrix of the GLMCamera
* The parameters camPos (0|0|1), lookAt (0|0|0), and upVector (0|1|0) of the view matrix remain unchanged. This means all screen position calculations must take this transformation into account, multiplying it with the inverse camera model matrix to get its position in 3D space within the scene
* TrackBall rotates always relative to the center (0|0|0) of the Camera Model-Matrix
* Each CameraSet must be assigned at least one Shader prototype (method addShaderProto("ShaderProtoName", std::list<renderPasses>)), this uses the ShaderProtoFact class
* Shader prototypes are render-pass specific, with the list passed to addShaderProto defining for which render passes the relevant shader should be used.

### Gizmos
* Entry point is SPObjectSelector. In OBJECT_MAP_PASS, all objects in the scene are rendered into an R32F buffer with their ID as "color". When you click with the mouse, a point is drawn at that location and the color value (ID) at that location is drawn into an FBO. The texture bound to this, which is only 1 pixel large, is then loaded as a float and cast as an int.
* If an object exists with the found ID, one of the three gizmos (translation, scaling, rotation) becomes visible, and its position and rotation are adjusted to match the object's model matrix. The size of the Gizmo is fixed relative to the screen size and is dynamically adjusted in the Gizmo's draw() function.
* Since Gizmos must always be visible, they are rendered in a separate SceneTree (gizmoTree), which is rendered after clearing the depth buffer and rendering the scene tree.
* Two different models are used for display and rendering into the object map. Simple cubes are used for the object map that are slightly larger than the arrow models to have some "error tolerance" when clicking with the mouse.
* Moving an object works as follows:
  * The ObjectSelector determines the ID of the object at the mouse pointer position on MouseDown
  * It saves the current mouse position, and the current model matrices for both the Gizmo and the object
  * Always what is sought after is the position in 3D space projected to NDC at the current mouse position, along the axis (or plane) it will be moved. To find this, the inverse process is taken. Each mouse position in NDC (2D space of the window) corresponds to a line in 3D space where the desired target position is located. This line can be determined by the camera origin point and the normalized x-, y- mouse position combined with the z-position = "near" value of the camera frustum, which intersects with the plane containing the object axis it will move along and that is as parallel to the screen plane as possible.

### Light
* Derived from SceneNodes, defines everything that emits light in the scene
* To create a new Light, an instance must be created, added to the SceneTree using addChild(), and added to the CameraSet's ShaderPrototype using addLight().
* Light objects can load external 3D models that are added to the LightNode using addChild(). There must only be one sub-level of nodes to a LightNode. LightNodes have the NodeType PD3D_SNT_LIGHT. Subnodes for display carry the type PD3D_SNT_LIGHT_SCENE_MESH.
* 3D models for LightNode must always be created centered on (0|0|0).

### Scene3D
* "Root" object for the framework
* Contains scene-wide tools like BoundingBoxer, ShaderCollector, etc.
* Contains the actual render loop:
  * There are various render passes:
    * Object_Map_Pass: objects are rendered into an R32F texture; their ID corresponds to their "color"
    * SHADOW_MAP_PASS: the scene is calculated from the perspective of the light sources. The resulting depth textures will be used later to compare whether there is a pixel from the perspective of each respective light that lies in front of the one being rendered, i.e., it blocks it
    * SCENE_PASS: the actual pass to generate visible results using ShaderPrototypes
    * OBJECT_ID_PASS: Reassignment of SceneNode IDs
  * Render passes have a fixed order. For example, the SHADOW_PASS must be executed before the SCENE_PASS so that lights and shadows can be displayed correctly
  * Order is: CameraSet → ShaderPrototype → SceneNode

### SceneNodes
* Basic element; anything that has meshes
* Can have SubNodes (children)
* Rendered recursively
* Have model matrices (position, scaling, rotation)
  * Matrices are hierarchically multiplied (from root node to SubNode); when rendering upon request (i.e., the "hasNewModelMat" flag is set somewhere in the NodeTree), a matrix stack (std::list<mat4>) is maintained, where the current parent matrix is always the multiplication of all current matrices in the list.
  * To save computation time, this multiplication is only performed if the model matrix of the node or its parent has changed.
  * To prevent duplicate multiplications, this calculation is centrally executed at the beginning of the render loop. When changes occur (translate, rotate, scale), the flag "hasNewModelMat" is set in the node. In the render loop, each node checks for this flag. If found, it is removed from the current node and set for all children.
  * Callbacks can be set that are called when the model matrices change (also centrally in the render loop)
  * SceneNodes then have a modelMat = relative to parent and an absModelMat used for rendering
* In addition to the model matrices, there is the "dimension" attribute which refers to absolute world coordinates
* Have types (Standard, Light, etc.) :
  * SNT_STANDARD: a standard visible mesh
  * SNT_LIGHT: a light emitting object, typically not visible inside the scene, but may contain children with visible meshes; this should have the type SNT_LIGHT_SCENE_MESH
  * SNT_LIGHT_SCENE_MESH: a visible mesh representing the light object (Projector, Spotlight, etc)
* Have unique IDs (integer from 1 - … )
* here is a map<renderPass, bool> to enable/disable a node for a specific render pass * A callback can be set that gets called when the position of the node in the tree changes (setParentNode())

### ShaderPrototypes
* General class for instantiating shaders with which a scene can be rendered

### ShadowMap
* As the name says, generators for ShadowMaps, typically textures of type DEPTH_BUFFER_32F (possibly with layers)
* Owns at least one FBO to which the DepthBuffer is bound
* Owns at least one shadowShader (possibly with GeometryShaders for parallel computation of multiple lights, e.g., ShadowMapArray)
* The simplest form of shadow calculation is to render the scene from the light's perspective and store the resulting depth values in a texture. Then, when rendering, for each vertex, its distance from the light source is calculated and compared with the value at the corresponding position in the depth texture, which is obtained by transforming the vertex position into NDC using the light coordinate system. OpenGL provides the special sampler type sampler2DShadow to bind depth textures to it. The command textureProj, which passes the texture unit and the transformed vertex position in the light coordinate system to it, returns a float value between 0 -1 directly, corresponding to the resulting brightness value.

### ShadowMapVsmArray
* VSM (Variance Shadow Mapping), instead of creating a depth texture with absolute values, an RG32F texture with the distribution or "density" of these values is set up. This can be smoothed in contrast to simple depth values.


## UI System

### User Interaction
- The GLSceneGraph UIWindow class, provides the following virtual methods which need to be bound to the underlying OS-specific HID managing library (like GLFW, QT or native OS calls):
  - `onKeyDown(int keyNum, bool shiftPressed, bool _ctrlPressed, bool _altPressed)`
  - `onLeftMouseButtonDown(float _xPos, float _yPos)`
  - `onLeftMouseButtonDownNoDrag(float _xPos, float _yPos)`
  - `onLeftMouseButtonUp()`
  - `onRightMouseButtonDown(float _xPos, float _yPos)`
  - `onRightMouseButtonUp()`
  - `onMouseMove(float _xPos, float _yPos, ushort _mode)`
  - `onWheel(int _deg)`
  - `onResizeDone()`
- all these methods acquire the s_procHidMtx mutex in form of a unique_lock and push a lambda function with the call to the respective glOnKeyDown(), glOnLeftMouseButtonDown(), etc method to the s_hidEvents list, which is processed on calling Scene::procHid()
- normally user interaction on UINode should either have an effect on the parameters/data of the UINode itself, or on other data which is part of the Scene. To resolve the first case UINode has the virtual methods `mouseIn()`, `mouseCursor()`, `mouseDown()`, `mouseOut()` which simply can be overloaded. To solve the latter case, there mouseClick, mouseIn, mouseOut and mouseCursor callback functions in form of std::functions member variables of UINode which can be set from outside and thus manipulate the data, which the std::functions capture.

### Drawing
- Processing Steps: A UI System basically should be event-based and render only when there has been user interaction which causes changes in the visible elements. For this reason there is the concept of ProcSteps or processing steps, which are nothing more than a std::function which an active/inactive flag, arranged in form of a std::map<procStepType, ProcStep>. Typically a Scene setup for UI cases consists of a continuous event-loop which iterates through the ProcSteps and executes them in case they are active and sets them inactive afterwards. So a procSteps map would at least contain something like:
  procSteps[Draw] = ProcStep{true, std::bind(&UIApp::drawNodeTree, this) };
- There may be animated elements or other elements that may need to be updated apart from user changes. Typically these are organized either as a separate list of std::functions which are called on every loop iteration or as a ProcStep which is always set to active.
- A UINodes matrices are only updated when the “m_geoChanged” flag is set (implicitly done by all methods such as setWidth(), setHeight(), setX() etc, which causes the UIMat’s updateMatrix() method to be called. Inside this method, the following calculations happen:
  - the actual parent view’s offset, size and aspect is calculated (in pixels) which makes use of the m_viewport member variable, which is set when the UINode is added as a child object. Basically only the root UINode will hold an vec4 viewport in form of an object, all other child UINodes will just hold a pointer to this root viewport - thus it changes on all children, when the root is updated
  - then the UINodes position and size values (either Pixel or relative) are transformed to absolute window-relative pixels. In this step the Align and Pivot settings are applied.
  - the result is a transformation matrix, composed of scaling and translation, which is multiplied by the parent matrix (the down-merged matrix-stack).
  - In case there are border settings, which are always in pixels, the relative line width in relation to the local matrix is calculated
  - since a UINode may be a container for other UINodes, there is the need to have zooming and positioning within UINode. This is done by the so called contentTransMat. First the position and size of the actual UINode is calculated, then the user defined rotation, scaling, zooming are transformed to matrices and multiplied. The get the final viewMat for “in-set” children elements this matrix is multiplied with the relative UINodes transformation matrix
  - all standard UINodes (Div, Label, Image) are rendered indirectly which is way faster then individual rendering, since it implies less state changes, value-passing and thus synchronization points between GPU and CPU. There is the DrawManager class which basically takes care of collecting, sorting of the UINodes vertex data and later shader setup for rendering. This process is quite complex since UINodes are assembled into one single VAO and all relevant standard textures (e.g. font glyphs) are merged into 3d-layer textures which then are referenced by indexes.
  - One big issue in indirect rendering is transparency, which basically can be solved by using z-positioning or z-sorting. Free z-position for any Node at any time, raises the necessity for several drawing passes (typically first drawing all solid elements and the all transparent element). Keeping a fixed order of Nodes which translates into z-order doesn't require multiple draw passes and results in better performance which is the reason why here it was implemented in this way.
  - There is also the possibility for inmediate mode rendering by pushing a lambda function to the DrawManager. This won't be evaluated immediately, but the order of rendering is kept and the function executed as-is.

### Basic Application Component Design

![App Design](../../../Documentation/images/uisdk_design_General.jpg)

### UIApplication in multithreaded mode

![Threading](../../../Documentation/images/uisdk_design-Threading.png)

- In the main thread a loop is running which is blocked, until an HID event is entering
- the WindowManager calls all global callback corresponding to this event and afterwards the corresponding UIWindow HID callback
- each UIWindow HID callback pushes the specific event to the gl-event queue and decide wheter to the UIWindows gl-loop has to be iterated or not
- In the UIWindows gl loop
  - the hid queue is processed (and has size 0 afterwards)
  - then all gl callbacks are processed
  - then when the draw flag is set, the UINode tree is processed (and drawn)

> [!NOTE]
> running an UIApplication in multithreaded mode is meant for cases where high draw performance is needed. No gl context switching is needed and data synchronisation is done via queues, BUT extreme care has to be taken in which context commands are executed. E.g. opening a window outside the main thread is prohibited and may cause a crash. Calling gl commands without a bound gl context will have no effect and may lead to undefined behaviour.
 
### UIApplication in single thread mode
- glfwWaitEvents()
- on event, global event callbacks are executed
- window specific gl context is bound
- window specific event callback is called
- UINode tree is processed / drawn if necessary
 
Event and gl loop are identical, no queues are needed, gl commands can be used at any time, windows can be opened at any time. This approach is much simpler than the multithreaded one and contain way less risks, since no synchronization of threads are needed. The downside is that gl context switching is need which is a bottleneck in situations where the highest performance possible is needed.

### Coordinate System

![Coordinate System](../../../Documentation/images/uisdk_design-Matrices.jpg)

orthogonal matrix, origin left-top, unidades = pixels (float) 

### UI Graph

![UI Graph](../../../Documentation/images/uisdk_design-Matrices_2.jpg)

The UI graph is simple standard node structure with parent-child relations. 

### Positioning

- Alignment: align::left, align::right, align::center, valign::top, valign::bottom, valign::center
- Pivot: default is same as alignment, but can be set explicitly
- Padding: setPadding() -> equal on all sides or setPadding(left, top, right, bottom)

![Positioning](../../../Documentation/images/uisdk_design-Matrices_3.jpg)

![Positioning2](../../../Documentation/images/uisdk_design-Matrices_4.jpg)

![Positioning3](../../../Documentation/images/uisdk_design-Matrices_5.jpg)

![Positioning4](../../../Documentation/images/uisdk_design-Matrices_6.jpg)

### UINode base class and derivatives

![UINode base class](../../../Documentation/images/uisdk_design-Inheritance.jpg)

### UINode Tree processing

- recursive matrix update iteration
  - init the Node if necessary and build a styleSheet from it's default values
  - update the Node's styles if necessary
  - update it's matrix
    - get the parent's absolute content matrix and create a copy of it (sum of all parent nodes matrices)
    - calculate the parent's content viewport in pixels
    - calculate the nodes position (concerning int/float conversion, pivot point, alignment)
    - calculate the nodes size
    - if a fixed aspect is requested, adapt the size
    - calculate the content matrix (padding, border, content Transformation 2D/3D)
  - iterate through the nodes children and calculate a bounding box around them
- clear scissor stack
- recursive draw iteration
  - set gl scissoring if requested, depending on the parents viewport
  - call the UINode's draw function
  - call the UINode's objectmap draw function
  - push the actual viewport to the scissor stack
  - recursively iterate through all children

**Optimizations**
- recursive tree iteration is slower then parsing a flat list. During the first matrix iteration a flat list with all active nodes in the correct drawing can be created and used during the drawing step
- drawing of visible parts and drawing of object map can be done in one step, since they use the same matrices. Rendering is already done into an FBO with 2 attachments. The only thing missing is to integrate the objmap drawing part into all drawing shaders

### HID Processing

![HID processing](../../../Documentation/images/uisdk_design-HID Processing.png)

#### Brief description

- OS HID Events are received from glfw
- glfw calls global HID callbacks in GLFWWindowManger
- glfw calls window specific HID callbacks in GLFWWindowManager
- the UIWindows specific HID function is called
- inside the HID function, _UIWindow::getObjAtPos()_ is called using the actual mouse position. Here, the nodetree is recursively iterated from top to bottom. In case the mouse position is inside the bounds of the actual node, it is marked as found. in case it has more children, those are iterated and the process is repeated until the end of the tree is reached
- when a Node is found, a flat will be generated containing the node and all its parent elements. This list will then be iterated from bottom (lowest level) to top. Any node in this list can "consume" the hid event and prevent further propagation up the tree
- UINodes can be excluded from HID processing using the _excludeFromObjectMap(bool)_ method. Excluding a UINode implicitly excludes all its children
- each UINode is identified by a single or range of object IDs (depending on its content). in case the UINode is excluded from the object map, the ID will be -1
- UINode Ids are assigned once after changes in the tree during the drawing step
- An UINode must be visible, not in any _disabled*_ state and not excluded from object map in order to receive HID events
- it is also possible to set global HID callbacks not related to any specific Node using _UIWindow::addGlobalMouseDownLeftCb(...)_


### Styles

![Styles](../../../Documentation/images/uisdk_design-Styles.jpg)

To take into account:
- UINode may have different positions, size, etc when selected, highlighted, etc
- when styles are edited via the styles.txt, default values are needed, in case a parameter is not set or deleted
- style parsing and updating of UINodes must be done in a well-defined order
- a C++ programmer may want to just setPosition, or setSize and will expect that after this command the corresponding values are updated
- style definitions must be inheritable in order to provide better control and keep text files small
- at the moment, default styles are managed by lambda functions which are overwritten, when a setPosition, or setSize command is issued

#### Style parsing

![Styles Parsing](../../../Documentation/images/uisdk_design-Styles2.jpg)

#### Style management

![Styles Parsing](../../../Documentation/images/uisdk_design-Styles3.jpg)