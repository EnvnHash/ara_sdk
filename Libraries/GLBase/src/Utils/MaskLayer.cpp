//
// Created by user on 10.07.2020.
//

#include "MaskLayer.h"

#include <GLBase.h>
#include <Property.h>
#include <Utils/FBO.h>

using namespace glm;
using namespace std;
using namespace pugi;

namespace ara {

MaskLayer::MaskLayer(maskType mType, GLBase *glbase)
    : m_type(mType), m_glbase(glbase), m_shCol(&glbase->shaderCollector()) {
    init();
}

MaskLayer::MaskLayer(filesystem::path inPath, maskType mType, GLBase *glbase)
    : m_path(std::move(inPath)), m_type(mType), m_glbase(glbase), m_shCol(&glbase->shaderCollector()) {
    init();
}

void MaskLayer::init() {
    if (m_inited) return;

    m_texture   = make_unique<Texture>(m_glbase);
    m_transform = mat4(1.f);

    // try to init the Mask object
    switch (m_type) {
        case maskType::Vector:
            if (m_path.empty()) {
                m_polygon.setShaderCollector(m_shCol);
                m_polygonInv.setShaderCollector(m_shCol);
            } else {
                // load svg
            }
            break;
        case maskType::Bitmap:
            m_polygon.setShaderCollector(m_shCol);
            m_polygonInv.setShaderCollector(m_shCol);
            if (!m_path.empty()) loadBitmap();
            break;
        default: break;
    }

    m_name   = "MaskLayer";
    m_inited = true;
}

void MaskLayer::updatePerspMat() {
    int    map[4]       = {0, 1, 3, 2};
    double refMap[4][2] = {{-1.f, -1.f}, {1.f, -1.f}, {1.f, 1.f}, {-1.f, 1.f}};
    double x[4], y[4], _x[4], _y[4];
    for (auto i = 0; i < 4; i++) {
        x[i] = refMap[map[i]][0];
        y[i] = refMap[map[i]][1];

        _x[i] = m_polygon.getPoint(0, map[i])->position.x;
        _y[i] = m_polygon.getPoint(0, map[i])->position.y;
    }
    matrix *projection = projection_matrix(x, y, _x, _y);

    m_transform = glm::inverse(matrixToGlm(projection));
    m_changed   = true;
}

CtrlPoint *MaskLayer::addPoint(uint32_t shapeLevel, float x, float y) {
    if (m_type == maskType::Vector) {
        m_changed = true;
        return m_polygon.addPoint(shapeLevel, x, y);
    }
    return nullptr;
}

CtrlPoint *MaskLayer::addPoint(uint32_t shapeLevel, float x, float y, float tex_x, float tex_y) {
    if (m_type == maskType::Vector && m_inited) {
        m_changed = true;
        return m_polygon.addPoint(shapeLevel, x, y, tex_x, tex_y);
    }
    return nullptr;
}

void MaskLayer::removeAllPoints() {
    m_polygon.clear();
    m_changed = true;
    m_valid   = false;
}

void MaskLayer::tesselate() {
    if (m_inited && m_changed) {
        if (m_inverted && m_type == maskType::Vector) {
            m_polygonInv.createInv(&m_polygon);
            m_polygonInv.tesselate();
        } else
            m_polygon.tesselate();

        m_changed = false;
    }
}

void MaskLayer::setInverted(bool val) {
    m_inverted = val;
    m_changed  = true;
}

void MaskLayer::loadBitmap(bool resetPoly) {
#ifdef ARA_USE_FREEIMAGE
    m_type = maskType::Bitmap;

    if (!m_path.empty()) {
        if (m_copyLoadedBitmap) {
            FIBITMAP *pBitmap = m_texture->ImageLoader(m_path.string().c_str(), 0);
            // compose save path
            auto savePath = m_tempBitmapPath / (m_path.stem().string() + ".png");
            m_path        = savePath;
            // save a copy
            FreeImage_Save(FIF_PNG, pBitmap, savePath.string().c_str(), 0);
            // upload to GL, automatically frees pBitmap memory implicitly
            m_texture->loadFromFib(pBitmap, GL_TEXTURE_2D, 8, false);
        } else {
            m_texture->loadTexture2D(m_path.string().c_str(), 8);
        }

        if (resetPoly) {
            createStdQuad();
            m_inverted = false;
            m_changed  = false;
        }

        m_inited = true;
    }
#endif
}

void MaskLayer::loadBitmapFromMem(uint8_t *data, int width, int height, int bpp, bool inverted, bool hFlip) {
    GLenum uplType    = GL_RGBA;
    GLenum uplTypeInt = GL_RGBA8;

    switch (bpp) {
        case 8:
            uplType    = GL_RED;
            uplTypeInt = GL_R8;
            break;
        case 16:
            uplType    = GL_RG;
            uplTypeInt = GL_RG8;
            break;
        case 24:
            uplType    = GL_RGB;
            uplTypeInt = GL_RGB8;
            break;
        case 32:
            uplType    = GL_RGBA;
            uplTypeInt = GL_RGBA8;
            break;
        default: break;
    }

    if (m_texture->isAllocated()) {
        GLuint id = m_texture->getId();
        glDeleteTextures(1, &id);
    }

    // MaskLayer textures are always Gl_RGBA8
    m_texture->allocate2D(width, height, GL_RGBA8, getExtType(GL_RGBA8), GL_TEXTURE_2D, GL_UNSIGNED_BYTE);

    // in case the bitmap to upload is not RGBA, create a temporary texture,
    // upload the bitmap and convert to GL_RGBA via a shader
    if (uplType != GL_RGBA) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);

        //-----------------------------------------------------------------

        // get active states for restore after FBO use
        GLint                m_lastBoundFbo    = 0;
        GLboolean            m_lastMultiSample = 0;
        std::vector<GLint>   m_lastDrawBuffers(m_glbase->maxNrDrawBuffers());
        std::array<GLint, 4> m_csVp{0};

        glGetIntegerv(GL_VIEWPORT, &m_csVp[0]);
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &m_lastBoundFbo);
#ifndef ARA_USE_GLES31
        glGetBooleanv(GL_MULTISAMPLE, &m_lastMultiSample);
#endif
        for (GLuint i = 0; i < (GLuint)m_glbase->maxNrDrawBuffers(); i++)
            glGetIntegerv(GL_DRAW_BUFFER0 + i, &m_lastDrawBuffers[i]);

        //-----------------------------------------------------------------

        // create a temporary VAO for drawing
        GLuint tempVao = 0;
        glGenVertexArrays(1, &tempVao);

        // create a temporary texture for upload
        GLuint tempTexId = 0;
        glGenTextures(1, &tempTexId);
        glBindTexture(GL_TEXTURE_2D, tempTexId);
        glTexStorage2D(GL_TEXTURE_2D, 1, uplTypeInt, width, height);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, uplType, GL_UNSIGNED_BYTE, data);

        // get the color conversion shader
        if (!m_convShader && m_shCol) m_convShader = m_shCol->getStdTexColConv();

        // create a temporary FBO and attach the texture, to write to it
        GLuint tempFbo = 0;
        glGenFramebuffers(1, &tempFbo);
        glBindFramebuffer(GL_FRAMEBUFFER, tempFbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture->getId(), 0);

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (GL_FRAMEBUFFER_COMPLETE != status) LOGE << "FBO incomplete";

        glViewport(0, 0, width, height);
        glScissor(0, 0, width, height);
        glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_BLEND);

        // bind shader and texture
        m_convShader->begin();
        m_convShader->setUniform1i("tex", 0);
        m_convShader->setUniform1i("inverted", inverted);
        m_convShader->setUniform1i("hFlip", (int)hFlip);
        glBindTexture(GL_TEXTURE_2D, tempTexId);

        // draw
        glBindVertexArray(tempVao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);

        ara::Shaders::end();

        glEnable(GL_BLEND);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &tempFbo);
        glDeleteTextures(1, &tempTexId);

        glDeleteVertexArrays(1, &tempVao);

        //-----------------------------------------------------------------

        // restore states
        // rebind last s_fbo
        glBindFramebuffer(GL_FRAMEBUFFER, m_lastBoundFbo);
        glViewport(m_csVp[0], m_csVp[1], m_csVp[2], m_csVp[3]);
        glScissor(m_csVp[0], m_csVp[1], m_csVp[2], m_csVp[3]);

        if (m_lastBoundFbo != 0) {
            std::vector<GLenum> attachments;
            int                 nrValidBuffers = 0;
            for (GLuint i = 0; i < (GLuint)m_glbase->maxNrDrawBuffers(); i++) {
                if (m_lastDrawBuffers[i]) {
                    attachments.push_back(GL_COLOR_ATTACHMENT0 + i);
                    nrValidBuffers++;
                }
            }
            glDrawBuffers(nrValidBuffers, &attachments[0]);
        }

#ifndef ARA_USE_GLES31
        if (!m_lastMultiSample)
            glDisable(GL_MULTISAMPLE);
        else
            glEnable(GL_MULTISAMPLE);
#endif
    } else {
        m_texture->upload(data);
    }

    if (m_polygon.getPoints(0)->empty()) createStdQuad();

    // if (m_copyLoadedBitmap)
    //{
    //  compose save path
    auto savePath = m_tempBitmapPath / "core_import_temp.png";
    m_path        = savePath;
#ifdef ARA_USE_FREEIMAGE
    // save a copy
    m_texture->saveTexToFile2D(savePath.string().c_str(), FIF_PNG, width, height, GL_RGBA8, (GLint)m_texture->getId());
    //}
#endif
}

void MaskLayer::createStdQuad() {
    // remove polygon
    // if (m_polygon) m_polygon.reset();

    m_polygon.clear();

    // create a new pseudo polygon with 4 control points to control the texture
    // transformation matrix
    auto p          = m_polygon.addPoint(0, -1.f, 1.f, 0.f, 1.f);
    p->texBasePoint = true;

    p               = m_polygon.addPoint(0, -1.f, -1.f, 0.f, 0.f);
    p->texBasePoint = true;

    p               = m_polygon.addPoint(0, 1.f, -1.f, 1.f, 0.f);
    p->texBasePoint = true;

    p               = m_polygon.addPoint(0, 1.f, 1.f, 1.f, 1.f);
    p->texBasePoint = true;

    m_changed = true;
    tesselate();
}

void MaskLayer::loadVml(const filesystem::path &filename) {
    try {
        // Read XML string into temporary document
        pugi::xml_document doc;
        std::ifstream      t(filename);
        std::string        xmlString((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
        if (doc.load_string(xmlString.c_str())) {
            auto root = doc.child("MaskLayer");
            if (root) parseFromXml(root);
        } else
            LOGE << " error reading xml";

    } catch (std::exception &e) {
        throw runtime_error("MaskLayer ERROR, could not load vml file " + filename.string() + " " + e.what());
    }
}

void MaskLayer::saveAs(const filesystem::path &filename) {
    if (!m_inited) return;

    try {
        // check extension, if it's a bitmap save m_fbo
        if (!filename.extension().compare(".vml")) {
            xml_document doc;
            xml_node     root = doc.append_child("MaskLayer");

            serializeToXml(root);

            stringstream xmlStream;
            doc.save(xmlStream);
            string xmlString = xmlStream.str();
            auto   splitStr  = xmlString.substr(xmlString.find("?>") + 3, xmlString.size());

            std::ofstream out_file(filename);
            out_file << splitStr;

        } /*
     else if (!filename.extension().compare(".xml"))
     {
         // TODO: add Bitmap exporting here

     } else {
       //  m_fbo->saveToFile(filename.string().c_str(), 0);
     }*/
    } catch (...) {
        throw runtime_error("MaskLayer,could not load vml file " + filename.string());
    }
}

void MaskLayer::serializeToXml(pugi::xml_node &parent) {
    // if this is a bitmap layer, save the layer as a png beside the xml file
    // and give it the name of the mask layer + the index of the layer
    if (m_type == maskType::Bitmap) {
        auto sp      = filesystem::path(m_stackSaveFileName);
        m_exportPath = sp.stem().string() + "_" + std::to_string(m_layerIdx);

        // there is always a temporary copy of the image for undo, so just
        // create a copy of it
        if (!m_path.empty()) {
            m_exportPath = m_exportPath.string() + m_path.extension().string();
            filesystem::copy(m_path, sp.parent_path() / m_exportPath, filesystem::copy_options::overwrite_existing);
        }
    }

    auto node = parent.append_child("Polygon");
    m_polygon.serializeToXml(node);

    node = parent.append_child("PolygonInv");
    m_polygonInv.serializeToXml(node);

    node = parent.append_child("maskType");
    node.text().set((int)m_type == 0 ? "Vector" : "Bitmap");
    node.append_attribute("propertyName").set_value("maskType");

    node = parent.append_child("Bool");
    node.text().set(m_visible);
    node.append_attribute("propertyName").set_value("visible");

    node = parent.append_child("Bool");
    node.text().set(m_selected);
    node.append_attribute("propertyName").set_value("selected");

    node = parent.append_child("Bool");
    node.text().set(m_inverted);
    node.append_attribute("propertyName").set_value("inverted");

    node = parent.append_child("UInt");
    node.text().set(m_layerIdx);
    node.append_attribute("propertyName").set_value("layerIdx");

    node = parent.append_child("Bool");
    node.text().set(m_copyLoadedBitmap);
    node.append_attribute("propertyName").set_value("copyLoadedBitmap");

    node = parent.append_child("String");
    node.text().set(m_name.c_str());
    node.append_attribute("propertyName").set_value("name");

    node = parent.append_child("String");
    node.text().set(m_path.string().c_str());
    node.append_attribute("propertyName").set_value("path");

    node = parent.append_child("String");
    node.text().set(m_tempBitmapPath.string().c_str());
    node.append_attribute("propertyName").set_value("dataPath");

    node = parent.append_child("String");
    node.text().set(m_exportPath.string().c_str());
    node.append_attribute("propertyName").set_value("exportPath");

    node = parent.append_child("vec4");
    node.append_attribute("propertyName").set_value("idColor");
    node.append_attribute("x").set_value(m_idColor.r);
    node.append_attribute("y").set_value(m_idColor.g);
    node.append_attribute("z").set_value(m_idColor.b);
    node.append_attribute("w").set_value(m_idColor.a);

    node = parent.append_child("vec4");
    node.append_attribute("propertyName").set_value("fillColor");
    node.append_attribute("x").set_value(m_fillColor.r);
    node.append_attribute("y").set_value(m_fillColor.g);
    node.append_attribute("z").set_value(m_fillColor.b);
    node.append_attribute("w").set_value(m_fillColor.a);

    node = parent.append_child("vec4");
    node.append_attribute("propertyName").set_value("fillColorInv");
    node.append_attribute("x").set_value(m_fillColorInv.r);
    node.append_attribute("y").set_value(m_fillColorInv.g);
    node.append_attribute("z").set_value(m_fillColorInv.b);
    node.append_attribute("w").set_value(m_fillColorInv.a);

    node = parent.append_child("mat4");
    node.append_attribute("propertyName").set_value("transform");

    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            node.append_attribute(("v" + std::to_string(i * 4 + j)).c_str()).set_value(m_transform[i][j]);
}

void MaskLayer::parseFromXml(pugi::xml_node &node) {
    pugi::xml_attribute attr;

    auto child = node.first_child();
    m_polygon.parseFromXml(child);

    child = child.next_sibling();
    m_polygonInv.parseFromXml(child);

    child  = child.next_sibling();
    m_type = !strcmp(child.text().get(), "Vector") ? maskType::Vector : maskType::Bitmap;

    child     = child.next_sibling();
    m_visible = child.text().as_bool();

    child      = child.next_sibling();
    m_selected = child.text().as_bool();

    child      = child.next_sibling();
    m_inverted = child.text().as_bool();

    child      = child.next_sibling();
    m_layerIdx = child.text().as_uint();

    child              = child.next_sibling();
    m_copyLoadedBitmap = child.text().as_bool();

    child  = child.next_sibling();
    m_name = child.text().as_string();

    child  = child.next_sibling();
    m_path = filesystem::path(child.text().as_string());

    child            = child.next_sibling();
    m_tempBitmapPath = filesystem::path(child.text().as_string());

    child        = child.next_sibling();
    m_exportPath = filesystem::path(child.text().as_string());

    child = child.next_sibling();
    if ((attr = child.attribute("x"))) m_idColor.r = attr.as_float();
    if ((attr = child.attribute("y"))) m_idColor.g = attr.as_float();
    if ((attr = child.attribute("z"))) m_idColor.b = attr.as_float();
    if ((attr = child.attribute("w"))) m_idColor.a = attr.as_float();

    child = child.next_sibling();
    if ((attr = child.attribute("x"))) m_fillColor.r = attr.as_float();
    if ((attr = child.attribute("y"))) m_fillColor.g = attr.as_float();
    if ((attr = child.attribute("z"))) m_fillColor.b = attr.as_float();
    if ((attr = child.attribute("w"))) m_fillColor.a = attr.as_float();

    child = child.next_sibling();
    if ((attr = child.attribute("x"))) m_fillColorInv.r = attr.as_float();
    if ((attr = child.attribute("y"))) m_fillColorInv.g = attr.as_float();
    if ((attr = child.attribute("z"))) m_fillColorInv.b = attr.as_float();
    if ((attr = child.attribute("w"))) m_fillColorInv.a = attr.as_float();

    child = child.next_sibling();
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            if ((attr = child.attribute(("v" + std::to_string(i * 4 + j)).c_str())))
                m_transform[i][j] = attr.as_float();

    // if this is a bitmap layer, check if there is a bitmap file relative to
    // the loaded xml
    if (m_type == maskType::Bitmap && !m_exportPath.empty() && !m_stackLoadFileName.empty()) {
        auto lp  = filesystem::path(m_stackLoadFileName);
        auto lpa = lp.parent_path() / m_exportPath;

        if (filesystem::exists(lpa)) {
            m_path = lpa;
            loadBitmap(false);
        }
    }

    m_valid   = true;  // it's not possible to save invalid layers
    m_changed = true;
}
}  // namespace ara