#include "Spinner.h"

#include <Asset/AssetImageSection.h>
#include <Asset/AssetImageSource.h>
#include <DrawManagers/DrawManager.h>
#include <ObjectMapInteraction.h>

#include "UIApplication.h"

using namespace glm;
using namespace std;

namespace ara {

Spinner::Spinner() : Div() {
    setName(getTypeName<Spinner>());
    setFocusAllowed(false);
    UINode::setColor(1.f, 1.f, 1.f, 1.f);
}

Spinner::Spinner(const filesystem::path& filepath, int x, int y, unsigned width, unsigned height) : Div() {
    setName(getTypeName<Spinner>());
    setFocusAllowed(false);
    setPos(static_cast<int>(x), static_cast<int>(y));
    setSize(static_cast<int>(width), static_cast<int>(height == 0 ? width : height));
    UINode::setColor(1.f, 1.f, 1.f, 1.f);
    m_filepath = filepath;
}

void Spinner::init() {
    if (!m_filepath.empty()) {
        setImage(m_filepath.string());
    }

    if ((m_shader = m_shCol->get("UI_UISpinner")) == nullptr) {
        string vert = STRINGIFY(layout(location = 0) in vec4 position;\n
            layout(location = 1) in vec4 normal;\n
            uniform mat4 m_pvm;\n
            uniform vec2 size;\n
            out vec2 tex_coord;\n
            const vec2[4] quadVertices = vec2[4](vec2(0., 0.), vec2(1., 0.), vec2(0., 1.), vec2(1., 1.));\n
            void main() { \n
                tex_coord   = quadVertices[gl_VertexID]; \n
                gl_Position = m_pvm * vec4(quadVertices[gl_VertexID] * size, position.z, 1.0); \n
            });
        vert = ara::ShaderCollector::getShaderHeader() + vert;

        string frag = STRINGIFY(
            in vec2 tex_coord; \n
            layout(location = 0) out vec4 color; \n
            uniform sampler2D tex; \n
            uniform float t; \n
            void main() {
                \n vec2 p = tex_coord - vec2(0.5);
                vec2    tc = vec2((cos(t) * p.x - sin(t) * p.y + 0.5), 1.0 - (cos(t) * p.y + sin(t) * p.x + 0.5));
                color = texture(tex, max(min(tc, 1.0), 0.0));\n
        );
        frag = ara::ShaderCollector::getShaderHeader() + m_shCol->getUiObjMapUniforms() + frag + m_shCol->getUiObjMapMain() + "}";
        m_shader = m_shCol->add("UI_UISpinner", vert, frag);
    }
    m_initTime = std::chrono::system_clock::now();
}

void Spinner::updateStyleIt(ResNode * node, state st, const std::string& styleClass) {
    UINode::updateStyleIt(node, st, styleClass);

    if (!m_sharedRes) {
        LOGE << "Image::updateStyleIt Error m_sharedRes not set";
        return;
    }

    // is there a style defined?
    if (m_sharedRes->res->img(styleClass)) {
        setImgBase(m_sharedRes->res->img(styleClass));
    }

    if (typeid(*node) == typeid(AssetImageSource) || typeid(*node) == typeid(AssetImageSection)) {
        if (m_sharedRes->res->img(m_baseStyleClass)) {
            setImgBase(m_sharedRes->res->img(m_baseStyleClass));
        }
    } else {
        if (auto* inode = node->findNode<ResNode>("image")) {
            std::string name                     = inode->m_value;
            m_setStyleFunc[st][styleInit::image] = [name, this]() { setImgBase(m_sharedRes->res->img(name)); };
        }
    }
}

void Spinner::setImgBase(AssetImageBase * imgBase) {
    m_imgBase = imgBase;
}

bool Spinner::draw(uint32_t& objId) {
    Div::draw(objId);
    return drawFunc(objId);
}

bool Spinner::drawIndirect(uint32_t& objId) {
    Div::drawIndirect(objId);

    if (m_sharedRes && m_sharedRes->drawMan) {
        m_sharedRes->drawMan->pushFunc([this, objId] {
            drawFunc(objId);
        });
    }

    return true;  // count up objId
}

bool Spinner::drawFunc(const uint32_t& objId) {
    if (!m_inited) {
        return false;
    }

    if (m_shader && (m_tex || m_imgBase)) {
        m_refTime = chrono::duration<float, deci>(chrono::system_clock::now() - m_initTime).count() * 0.6f;

        m_shader->begin();
        m_shader->setUniform1i("tex", 0);
        m_shader->setUniform1f("t", m_refTime);
        m_shader->setUniformMatrix4fv("m_pvm", getMVPMatPtr());
        m_shader->setUniform2fv("size", &getSize()[0]);
        m_shader->setUniform1f("objId", static_cast<float>(objId));
        m_shader->setUniform1f("alpha", m_absoluteAlpha);

        glActiveTexture(GL_TEXTURE0);
        if (m_imgBase) {
            glBindTexture(GL_TEXTURE_2D, m_imgBase->getTexID());
        } else if (m_tex) {
            m_tex->bind();
        }

        glBindVertexArray(*m_sharedRes->nullVao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    }

    m_sharedRes->requestRedraw = true;

    return true;
}

Texture* Spinner::setImage(const filesystem::path& filepath, int mipmapLevels) {
    if (filepath.empty()) {
        return nullptr;
    }

    m_filepath = filepath;
    auto& texCol = getSharedRes()->glbase->textureCollector();
    m_tex = texCol.addFromMem(filepath, dataPath(), 8);
    texCol.addRemoveCb(filepath.string(), [this]{
        m_tex = nullptr;
    });

    return m_tex;
}

}
