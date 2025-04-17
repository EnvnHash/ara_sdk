//
// Created by sven on 18-10-22.
//

#ifdef __ANDROID__

#pragma once

namespace ara::cardboard {

/// @defgroup types Cardboard SDK types
/// @brief Various types used in the Cardboard SDK.
/// @{

/// Struct to hold UV coordinates.
typedef struct CardboardUv {
    /// u coordinate.
    float u;
    /// v coordinate.
    float v;
} CardboardUv;

/// Enum to distinguish left and right eyes.
typedef enum CardboardEye {
    /// Left eye.
    kLeft = 0,
    /// Right eye.
    kRight = 1,
} CardboardEye;

/// Enum to describe the possible orientations of the viewport.
typedef enum CardboardViewportOrientation {
    /// Landscape left orientation, which maps to:
    /// - Android: landscape.
    /// - IOS: UIDeviceOrientationLandscapeLeft.
    /// - Unity: ScreenOrientation.LandscapeLeft.
    kLandscapeLeft = 0,
    /// Landscape right orientation, which maps to:
    /// - Android: reverseLandscape.
    /// - IOS: UIDeviceOrientationLandscapeRight.
    /// - Unity: ScreenOrientation.LandscapeRight.
    kLandscapeRight = 1,
    /// Portrait orientation, which maps to:
    /// - Android: portrait.
    /// - IOS: UIDeviceOrientationPortrait.
    /// - Unity: ScreenOrientation.Portrait.
    kPortrait = 2,
    /// Portrait upside down orientation, which maps to:
    /// - Android: reversePortrait.
    /// - IOS: UIDeviceOrientationPortraitUpsideDown.
    /// - Unity: ScreenOrientation.PortraitUpsideDown.
    kPortraitUpsideDown = 3,
} CardboardViewportOrientation;

/// Struct representing a 3D mesh with 3D vertices and corresponding UV
/// coordinates.
typedef struct CardboardMesh {
    /// Indices buffer.
    int* indices;
    /// Number of indices.
    int n_indices;
    /// Vertices buffer. 2 floats per vertex: x, y.
    float* vertices;
    /// UV coordinates buffer. 2 floats per uv: u, v.
    float* uvs;
    /// Number of vertices.
    int n_vertices;
} CardboardMesh;

/// Struct to hold information about an eye texture.
typedef struct CardboardEyeTextureDescription {
    /// The texture with eye pixels.
    ///
    /// When using OpenGL ES 2.x and OpenGL ES 3.x, this field corresponds to a
    /// GLuint variable.
    ///
    /// When using Vulkan, this field corresponds to an uint64_t address
    /// pointing to a @c VkImage variable.The SDK client is expected to manage
    /// the object ownership and to guarantee the pointer validity during the
    /// @c ::CardboardDistortionRenderer_renderEyeToDisplay function execution
    /// to ensure it is properly retained. Usage example:
    ///
    /// @code{.cc}
    /// VkImage image;
    /// // Initialize and set up the image...
    /// CardboardEyeTextureDescription leftEye;
    /// leftEye.texture = reinterpret_cast<uint64_t>(image)
    /// // Fill remaining fields in leftEye...
    /// CardboardDistortionRenderer_renderEyeToDisplay(..., &leftEye, ...);
    /// // Clear previous image if it is needed.
    /// @endcode
    ///
    /// When using Metal, this field corresponds to a @c CFTypeRef
    /// variable pointing to a @c MTLTexture object. The SDK client is expected
    /// to manage the object ownership and to guarantee the pointer validity
    /// during the @c ::CardboardDistortionRenderer_renderEyeToDisplay function
    /// execution to ensure it is properly retained. Usage example:
    ///
    /// @code{.m}
    /// CardboardEyeTextureDescription leftEye;
    /// leftEye.texture = CFBridgingRetain(_texture);
    /// // Fill remaining fields in leftEye...
    /// CardboardDistortionRenderer_renderEyeToDisplay(..., &leftEye, ...);
    /// CFBridgingRelease(leftEye.texture);
    /// @endcode
    uint64_t texture;
    /// u coordinate of the left side of the eye.
    float left_u;
    /// u coordinate of the right side of the eye.
    float right_u;
    /// v coordinate of the top side of the eye.
    float top_v;
    /// v coordinate of the bottom side of the eye.
    float bottom_v;
} CardboardEyeTextureDescription;

/// Struct to set Metal distortion renderer configuration.
typedef struct CardboardMetalDistortionRendererConfig {
    /// MTLDevice id.
    /// This field holds a CFTypeRef variable pointing to a MTLDevice object.
    /// The SDK client is expected to manage the object ownership and to
    /// guarantee the pointer validity during the
    /// CardboardMetalDistortionRenderer_create function execution to ensure it
    /// is properly retained. Usage example:
    ///
    /// @code{.m}
    /// CardboardMetalDistortionRendererConfig config;
    /// config.mtl_device = CFBridgingRetain(mtlDevice);
    /// CardboardDistortionRenderer *distortionRenderer =
    ///     CardboardMetalDistortionRenderer_create(&config);
    /// CFBridgingRelease(config.mtl_device);
    /// @endcode
    uint64_t mtl_device;
    /// Color attachment pixel format.
    /// This field holds a [MTLPixelFormat enum
    /// value](https://developer.apple.com/documentation/metalkit/mtkview/1535940-colorpixelformat?language=objc).
    uint64_t color_attachment_pixel_format;
    /// Depth attachment pixel format.
    /// This field holds a [MTLPixelFormat enum
    /// value](https://developer.apple.com/documentation/metalkit/mtkview/1535940-colorpixelformat?language=objc).
    uint64_t depth_attachment_pixel_format;
    /// Stencil attachment pixel format.
    /// This field holds a [MTLPixelFormat enum
    /// value](https://developer.apple.com/documentation/metalkit/mtkview/1535940-colorpixelformat?language=objc).
    uint64_t stencil_attachment_pixel_format;
} CardboardMetalDistortionRendererConfig;

/// Struct to set Vulkan distortion renderer configuration.
typedef struct CardboardVulkanDistortionRendererConfig {
    /// The physical device available for the rendering.
    /// This field holds a [VkPhysicalDevice
    /// value](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkPhysicalDevice.html).
    /// Maintained by the user.
    uint64_t physical_device;
    /// The logical device available for the rendering.
    /// This field holds a [VkDevice
    /// value](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkDevice.html).
    /// Maintained by the user.
    uint64_t logical_device;
    /// The swapchain that owns the buffers into which the scene is rendered.
    /// This field holds a [VkSwapchainKHR
    /// value](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkSwapchainKHR.html).
    /// Maintained by the user.
    uint64_t vk_swapchain;
} CardboardVulkanDistortionRendererConfig;

/// Struct to set Metal distortion renderer target configuration.
typedef struct CardboardMetalDistortionRendererTargetConfig {
    /// MTLRenderCommandEncoder id.
    /// This field holds a CFTypeRef variable pointing to a
    /// @c MTLRenderCommandEncoder object. The SDK client is expected to manage
    /// the object ownership and to guarantee the pointer validity during the
    /// @c ::CardboardDistortionRenderer_renderEyeToDisplay function execution
    /// to ensure it is properly retained. Usage example:
    ///
    /// @code{.m}
    /// CardboardMetalDistortionRendererTargetConfig target_config;
    /// target_config.render_command_encoder =
    ///     CFBridgingRetain(renderCommandEncoder);
    /// CardboardDistortionRenderer_renderEyeToDisplay(..., &target_config,
    /// ...); CFBridgingRelease(target_config.render_command_encoder);
    /// @endcode
    uint64_t render_command_encoder;
    /// Full width of the screen in pixels.
    int screen_width;
    /// Full height of the screen in pixels.
    int screen_height;
} CardboardMetalDistortionRendererTargetConfig;

/// Struct to set Vulkan distortion renderer target.
typedef struct CardboardVulkanDistortionRendererTarget {
    /// The render pass object that will be used to bind vertex, indices and
    /// descriptor set.
    /// This field holds a [VkRenderPass
    /// value](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkRenderPass.html).
    /// Maintained by the user.
    uint64_t vk_render_pass;
    /// The command buffer object.
    /// This field holds a[VkCommandBuffer
    /// value](https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkCommandBuffer.html).
    /// Maintained by the user and this command buffer should be started before
    /// calling the rendering function.
    uint64_t vk_command_buffer;
    /// The index of the image in the swapchain.
    /// This number should NOT exceed the number of images in swapchain.
    /// If this number is above the swapchain length, the distortion renderer
    /// will stop and return directly.
    uint32_t swapchain_image_index;
} CardboardVulkanDistortionRendererTarget;

/// An opaque Lens Distortion object.
typedef struct CardboardLensDistortion CardboardLensDistortion;

/// An opaque Distortion Renderer object.
typedef struct CardboardDistortionRenderer CardboardDistortionRenderer;

/// An opaque Head Tracker object.
typedef struct CardboardHeadTracker CardboardHeadTracker;

/// @}

}  // namespace ara::cardboard

#endif