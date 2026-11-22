#include "PaintImage.h"
#include <UISharedRes.h>
#include <Shaders/ShaderCollector.h>
#include <Utils/Texture.h>
#include <GLBase.h>

#include "UIWindow.h"

using namespace glm;
using namespace std;

namespace ara {

PaintImage::PaintImage() {
    setTypeName<PaintImage>();
    setName(getTypeName<PaintImage>());
    m_maxUndoBufSize = 40;
}

void PaintImage::init() {
    Image::init();

    if (m_sharedRes && m_sharedRes->shCol) {
        const auto vert = ShaderCollector::getShaderHeader() + STRINGIFY(
            layout(location = 0) in vec3 position;
            uniform float size;
            uniform vec2 pos;
            out vec2 texCoord;
            void main() {
                const vec2[4] quadVertices = vec2[4](vec2(-1., -1.), vec2(1., -1.), vec2(-1., 1.), vec2(1., 1.));
                texCoord = quadVertices[gl_VertexID];
                gl_Position = vec4(quadVertices[gl_VertexID], 0.0, 1.0);
            }
        );

        const auto frag = ShaderCollector::getShaderHeader() + STRINGIFY(
            layout(location = 0) out vec4 fragColor;
            in vec2 texCoord;
            struct Brush {
                float size;
                float hardness;
                vec4 color;
                float opacity;
            };
            uniform vec2 pos;
            uniform vec4 limits;
            uniform int masked;
            uniform sampler2D maskTex;
            layout(std140) uniform BrushBlock {
                Brush brush;
            };
            void main() {
                if (texCoord.x < limits.x || texCoord.x > limits.z || texCoord.y > limits.y || texCoord.y < limits.w) {
                    discard;
                }
                float dist = distance(texCoord, pos);
                float edge = clamp((1.0 - brush.hardness), 0.0001, 1.0);
                float maskAlpha = 1.0;
                if (bool(masked)) {
                    maskAlpha = texture(maskTex, texCoord).a;
                    if (maskAlpha == 0.0) {
                        discard;
                    }
                }
                float falloff = 1.0 - smoothstep(brush.size * (1.0 - edge), brush.size, dist);
                if (falloff == 0.0) {
                    discard;
                }
                fragColor = vec4(brush.color.rgb, brush.opacity * falloff * brush.color.a * maskAlpha);
            }
        );

        m_paintShader = m_sharedRes->shCol->add("paintImageShader", vert, frag);

        if (m_paintShader) {
            m_scaledBrush = m_brush.size;
            m_brushBlock.init(m_paintShader->getProgram(), "BrushBlock");
            m_brushBlock.addVarName("brush.size", &m_scaledBrush, GL_FLOAT);
            m_brushBlock.addVarName("brush.hardness", &m_brush.hardness, GL_FLOAT);
            m_brushBlock.addVarName("brush.color", &m_brush.color, GL_FLOAT_VEC4);
            m_brushBlock.addVarName("brush.opacity", &m_brush.opacity, GL_FLOAT);
        }
    }

    initFbo();
    saveState();
}

void PaintImage::initFbo() {
    if (!m_fbo) {
        m_fbo = std::make_unique<FBO>();
        m_fbo->setGlbase(getSharedRes()->glbase);
    }

    if (m_tex) {
        m_fbo->fromTexMan(m_tex);
    } else if (m_texId) {
        m_fbo->fromTex(m_texId, m_texSize.x, m_texSize.y, GL_RGBA8, 1, GL_LINEAR, GL_LINEAR);
    }
}

void PaintImage::setSharedTexture(const std::shared_ptr<Texture>& tex) {
    m_tex = tex.get();
    m_ownedTex = tex;
    initFbo();
    if (m_drawMasked) {
        initMaskTex();
    }
}

void PaintImage::setMasking(const bool val) {
    m_drawMasked = val;
    if (val) {
        initMaskTex();
    }
}

void PaintImage::initMaskTex() {
    if (!m_ownedTex) {
        return;
    }

    const int width = m_ownedTex->getWidth();
    const int height = m_ownedTex->getHeight();
    if (width <= 0 || height <= 0) {
        return;
    }

    if (!m_maskTex || m_maskTex->getWidth() != width || m_maskTex->getHeight() != height) {
        m_maskTex = std::make_shared<Texture>();
        m_maskTex->setGlbase(getSharedRes()->glbase);
        m_maskTex->allocate2D(width, height, GL_R8, GL_RED, GL_TEXTURE_2D, GL_UNSIGNED_BYTE);
        m_maskTex->clear();
    }
}

void PaintImage::mouseDown(hidData& data) {
    Image::mouseDown(data);
    if (m_paintMode != mode::colorPick) {
        paint(data.mousePosNodeRel);
    }
}

void PaintImage::mouseDrag(hidData& data) {
    Image::mouseDrag(data);
    if (m_paintMode != mode::colorPick && data.mousePressed[mouseButt::left]) {
        paint(data.mousePosNodeRel);
    }
}

void PaintImage::mouseWheel(hidData& data) {
    setCursor();
}

void PaintImage::mouseUp(hidData& data) {
    Image::mouseUp(data);
    if (m_paintMode == mode::colorPick) {
        pickColor(data.mousePosNodeRel);
    } else if (data.dragging) {
        saveState();
    }
}

void PaintImage::mouseIn(hidData& data) {
    Image::mouseIn(data);
    setCursor();
}

void PaintImage::mouseOut(hidData& data) {
    Image::mouseIn(data);
    resetCursor();
}

void PaintImage::paint(const vec2& mousePos) {
    if (!(m_tex || m_texId) || !m_paintShader || !m_sharedRes || m_locked) {
        return;
    }

    if (!m_fbo) {
        initFbo();
    }

    m_fbo->bind();
    m_paintShader->begin();

    glBlendEquation(m_paintMode == mode::add ? GL_FUNC_ADD : GL_FUNC_REVERSE_SUBTRACT);
    glBlendFunc(m_paintMode == mode::add ? GL_ONE : GL_SRC_ALPHA, GL_ONE);

    const vec2 fboSize{ static_cast<float>(m_fbo->getWidth()), static_cast<float>(m_fbo->getHeight()) };

    // Set uniform block
    m_scaledBrush = m_brush.size * 2.f / fboSize.x;
    m_brushBlock.update();
    m_brushBlock.bind();

    std::array transPos = { mousePos, vec2(0.0), vec2(m_secSize) };
    if (m_secSize.x != 0 && m_secSize.y != 0) {
        for (auto &it : transPos) {
            it = vec2(m_secPos) + it / m_size * vec2(m_secSize); // fbo pos in pixels
            it = vec2{ it.x / fboSize.x * 2.f - 1.f,
                        it.y / fboSize.y * -2.f + 1.f }; // normalized fbo pos
        }
    }

    m_paintShader->setUniform2f("pos", transPos[0].x, transPos[0].y);
    m_paintShader->setUniform4f("limits", transPos[1].x, transPos[1].y, transPos[2].x, transPos[2].y);

    m_paintShader->setUniform1i("masked", m_drawMasked);
    if (m_maskTex) {
        m_maskTex->bind(0);
        m_paintShader->setUniform1i("maskTex", 0);
    }

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    if (m_drawMasked && m_maskTex) {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    m_fbo->unbind();

    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_sharedRes->setDrawFlag(true); // Request redrawing of the UI
}

void PaintImage::setBrushSize(const float brushSize) {
    m_brush.size = brushSize;
}

void PaintImage::setBrushHardness(const float hardness) {
    m_brush.hardness = hardness;
}

void PaintImage::saveToFile(const std::filesystem::path &filename) const {
    if (!m_fbo) {
        return;
    }
    m_fbo->saveToFile(filename, 0, GL_RGBA8);
}

void PaintImage::saveState() {
    if (m_undoing || !m_fbo || m_imageFile.empty()) {
        return;
    }

    const auto p = AssetLoader::getAssetPath() / std::filesystem::path(m_imageFile);
    const auto ext = p.extension().empty() ? ".png" : p.extension().string();
    std::error_code ec;

    // if there were new changes after one or several undos
    // remove all entries which lie in the future after the actual entry
    if (!m_backupCopies.empty() && m_backupCopiesIt != m_backupCopies.end() - 1) {
        for (auto it = std::next(m_backupCopiesIt, 1); it != m_backupCopies.end(); ++it) {
            std::filesystem::remove(*it, ec);
        }
        m_backupCopies.erase(std::next(m_backupCopiesIt, 1), m_backupCopies.end());
    }

    // if the undo queue is filled, delete the first element
    if (m_backupCopies.size() + 1 >= m_maxUndoBufSize) {
        std::filesystem::remove(m_backupCopies.front(), ec);
        m_backupCopies.erase(m_backupCopies.begin());
    }

    // serialize the actual state to a binary json and push it to the undo queue
    const std::string uuidCurrent = generateUUID();
    const auto currentBackupPath = AssetLoader::getAssetPath() / m_backupPath / (uuidCurrent + ext);
    m_fbo->saveToFile(currentBackupPath, 0, GL_RGBA8);
    m_fbo->saveToFile(p, 0, GL_RGBA8);
    m_backupCopies.emplace_back(currentBackupPath);
    m_backupCopiesIt = m_backupCopies.end() - 1; // set the undoBufPtr to the new entry
}

void PaintImage::undo() {
    if (m_backupCopies.empty() || m_backupCopiesIt == m_backupCopies.begin()) {
        return;
    }

    --m_backupCopiesIt;  // set back one entry

    m_undoing = true;
    if (const auto& restorePath = *m_backupCopiesIt; std::filesystem::exists(restorePath)) {
        restoreFile(restorePath);
    }
    m_undoing = false;

    for (auto &it : collectCallbacks(cbType::undo, true)) {
        it(this, "");
    }
}

void PaintImage::redo() {
    if (m_backupCopies.empty()) {
        return;
    }

    if (m_backupCopiesIt != std::prev(m_backupCopies.end())) {
        ++m_backupCopiesIt;
        m_undoing = true;
        if (const auto& restorePath = *m_backupCopiesIt; std::filesystem::exists(restorePath)) {
            restoreFile(restorePath);
        }
        m_undoing = false;
    }
}

void PaintImage::clear() {
    if (m_tex) {
        m_tex->clear();
    }
    if (m_fbo) {
        m_fbo->clear();
    }
    saveState();
}

void PaintImage::restoreFile(const std::filesystem::path &filename) {
    const auto tf = m_imageFile;
    setImg(filename.string());
    loadImg(true);
    m_imageFile = tf;
    initFbo();
    if (getSharedRes()) {
        getSharedRes()->reqRedraw();
    }
}

void PaintImage::setBackupPath(const std::filesystem::path& p) {
    m_backupPath = p;
    if (const auto bp = AssetLoader::getAssetPath() / m_backupPath; !std::filesystem::exists(bp)) {
        std::filesystem::create_directory(bp);
    }
}

void PaintImage::clearBackupPath() const {
    for (const auto p = AssetLoader::getAssetPath() / m_backupPath; const auto& entry : std::filesystem::directory_iterator(p)) {
        std::filesystem::remove_all(entry.path());
    }
}

void PaintImage::pickColor(const glm::vec2& mousePos) {
    const auto tex = getTexture();
    if (!tex) {
        return;
    }

    const int width = tex->getWidth();
    const int height = tex->getHeight();
    if (width <= 0 || height <= 0) {
        return;
    }

    // Convert mouse position to pixel coordinates in the texture
    const float x = mousePos.x / getSize().x * width;
    const float y = (1.f - mousePos.y / getSize().y) * height;

    // Clamp to valid range
    const int px = std::max(0, std::min(width - 1, static_cast<int>(x)));
    const int py = std::max(0, std::min(height - 1, static_cast<int>(y)));

    // Create a temporary FBO to read from the texture
    FBO readFbo;
    readFbo.setGlbase(getSharedRes()->glbase);
    readFbo.fromTex(tex->getId(), width, height, GL_RGBA8, 1, GL_NEAREST, GL_NEAREST);
    readFbo.bind();

    // Read the pixel color using glReadPixels
    std::array<GLubyte, 4> pixelData;
    glReadPixels(px, py, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixelData.data());
    readFbo.unbind();

    // Store the picked color (normalize to 0-1 range)
    m_pickColor = glm::vec4(
        pixelData[0] / 255.f,
        pixelData[1] / 255.f,
        pixelData[2] / 255.f,
        pixelData[3] / 255.f
    );
}

void PaintImage::setCursor() {
#ifdef ARA_USE_GLFW
    if (m_paintMode == mode::colorPick) {
        getSharedRes()->winHandle->setMouseCursor(WinMouseIcon::crossHair);
    } else {
        setCircleCursor();
    }
#endif
}

void PaintImage::setCircleCursor() {
#ifdef ARA_USE_GLFW
    const auto glfwWin = getWindow()->getWinHandle();
    m_cursor = GLFWWindow::createCircularMouseCursor(static_cast<int>(m_brush.size * getParentContentScale().x), {0.7f, 0.7f, 0.7f, 0.6f});
    glfwSetCursor(static_cast<GLFWwindow*>(glfwWin->getWin()), m_cursor);
#endif
}

void PaintImage::resetCursor() {
#ifdef ARA_USE_GLFW
    if (m_cursor) {
        glfwDestroyCursor(m_cursor);
        m_cursor = nullptr;
    }
#endif
}

} // namespace ara
