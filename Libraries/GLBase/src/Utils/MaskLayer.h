//
// Created by user on 10.07.2020.
//

#pragma once

#include <GLBase.h>
#include <GeoPrimitives/Polygon.h>
#include <Utils/Texture.h>

namespace ara {

class Texture;

class MaskLayer {
public:
    MaskLayer() = default;
    MaskLayer(maskType mType, GLBase *glbase);
    MaskLayer(std::filesystem::path inPath, maskType mType, GLBase *glbase);

    void       init();
    void       drawOutline() { m_polygon.drawOutline(m_valid ? GL_LINE_LOOP : GL_LINE_STRIP); }
    void       drawOutlineInv() { m_polygonInv.drawOutline(GL_LINE_LOOP); }
    void       updatePerspMat();
    CtrlPoint *addPoint(uint32_t shapeLevel, float x, float y);
    CtrlPoint *addPoint(uint32_t shapeLevel, float x, float y, float tex_x, float tex_y);
    void       removeAllPoints();
    void       createStdQuad();
    void       tesselate();
    void       setInverted(bool val);
    void       loadBitmap(bool resetPoly = true);
    void       loadBitmapFromMem(uint8_t *data, int width, int height, int bpp, bool inverted, bool hFlip);
    void       loadVml(const std::filesystem::path &filename);
    void       saveAs(const std::filesystem::path &filename);

    void setShaderCollector(ShaderCollector *shCol) {
        m_shCol = shCol;
        m_polygon.setShaderCollector(shCol);
        m_polygonInv.setShaderCollector(shCol);
    }

    void setFillColor(glm::vec4 col) {
        m_fillColor = col;
        for (int i = 0; i < 3; i++) m_fillColorInv[i] = 1.f - col[i];
        m_fillColorInv.a = col.a;
    }

    void               setType(maskType mtyp) { m_type = mtyp; }
    void               setIdColor(glm::vec4 col) { m_idColor = col; }
    void               setId(uint32_t id) { m_id = id; }
    void               setLayerIdx(uint32_t idx) { m_layerIdx = idx; }
    void               setInited(bool val) { m_inited = val; }
    void               setIsValid(bool val) { m_valid = val; }
    void               setVisible(bool val) { m_visible = val; }
    void               setSelected(bool val) { m_selected = val; }
    void               setFilePath(const char *fileP) { m_path = std::filesystem::path(fileP); }
    void               setCopyLoadedBitmap(bool val) { m_copyLoadedBitmap = val; }
    void               setTempBitmapPath(std::filesystem::path dPath) { m_tempBitmapPath = std::move(dPath); }
    void               setChanged() { m_changed = true; }
    void               setName(const std::string &n) { m_name = n; }
    void               setSaveFileName(const std::string &fn) { m_stackSaveFileName = fn; }
    void               setLoadFileName(const std::string &fn) { m_stackLoadFileName = fn; }
    [[nodiscard]] bool isInited() const { return m_inited; }
    [[nodiscard]] bool isInverted() const { return m_inverted; }
    [[nodiscard]] bool isVisible() const { return m_visible; }
    [[nodiscard]] bool isSelected() const { return m_selected; }
    [[nodiscard]] bool isValid() const { return m_valid; }
    maskType           getType() { return m_type; }
    Polygon           *getPolygon() { return &m_polygon; }
    Polygon           *getPolygonInv() { return &m_polygonInv; }
    float             *getIdColorPtr() { return &m_idColor[0]; }
    glm::vec4         *getIdColorVecPtr() { return &m_idColor; }
    float             *getFillColorPtr() { return &m_fillColor[0]; }
    glm::vec4         *getFillColorVecPtr() { return &m_fillColor; }
    float             *getFillColorInvPtr() { return &m_fillColorInv[0]; }
    glm::vec4         *getFillColorInvVecPtr() { return &m_fillColorInv; }
    float             *getTransformPtr() { return &m_transform[0][0]; }
    Texture           *getTexture() { return m_texture.get(); }
    std::string       &getName() { return m_name; }
    glm::mat4         &getTransformMat() { return m_transform; }

    void serializeToXml(pugi::xml_node &parent);
    void parseFromXml(pugi::xml_node &parent);

    template <class B>
    void serialize(B &buf) const {
        buf << m_name;
        buf << m_layerIdx;
        buf << (int)m_type << (int)m_visible << (int)m_valid << (int)m_inverted << (int)m_copyLoadedBitmap
            << (int)m_selected;
        buf << m_path.string();
        buf << m_tempBitmapPath.string();
        buf << m_exportPath.string();
        buf << m_idColor.r << m_idColor.g << m_idColor.b << m_idColor.a;
        buf << m_fillColor.r << m_fillColor.g << m_fillColor.b << m_fillColor.a;
        buf << m_fillColorInv.r << m_fillColorInv.g << m_fillColorInv.b << m_fillColorInv.a;

        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++) buf << m_transform[i][j];

        buf << m_polygon;
        buf << m_polygonInv;
    }

    template <class B>
    void parse(B &buf) {
        buf >> m_name;
        buf >> m_layerIdx;

        int temp;
        buf >> temp;
        m_type = (maskType)temp;
        buf >> temp;
        m_visible = (bool)temp;
        buf >> temp;
        m_valid = (bool)temp;
        buf >> temp;
        m_inverted = (bool)temp;
        buf >> temp;
        m_copyLoadedBitmap = (bool)temp;
        buf >> temp;
        m_selected = (bool)temp;

        std::string tempPath;
        buf >> tempPath;
        m_path = std::filesystem::path(tempPath);
        buf >> tempPath;
        m_tempBitmapPath = std::filesystem::path(tempPath);
        buf >> tempPath;
        m_exportPath = std::filesystem::path(tempPath);
        buf >> m_idColor.r >> m_idColor.g >> m_idColor.b >> m_idColor.a;
        buf >> m_fillColor.r >> m_fillColor.g >> m_fillColor.b >> m_fillColor.a;
        buf >> m_fillColorInv.r >> m_fillColorInv.g >> m_fillColorInv.b >> m_fillColorInv.a;

        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++) buf >> m_transform[i][j];

        // in case this was a bitmap layer, try to load the corresponding bitmap
        // file
        if (m_type == maskType::Bitmap && !m_path.empty()) {
            try {
                m_texture->loadTexture2D(m_path.string().c_str(), 8);
            } catch (...) {
                throw std::runtime_error("MaskLayer ERROR, could not load bitmap file " + m_path.string());
            }
        }

        buf >> m_polygon;
        buf >> m_polygonInv;
    }

private:
    std::string m_name;
    std::string m_stackSaveFileName;
    std::string m_stackLoadFileName;

    bool m_visible          = true;
    bool m_inited           = false;
    bool m_inverted         = false;
    bool m_selected         = false;
    bool m_copyLoadedBitmap = false;
    bool m_changed          = true;
    bool m_valid            = false;

    uint32_t m_id       = 0;
    uint32_t m_layerIdx = 0;

    glm::vec4 m_idColor{0.f, 0.f, 0.f, 1.f};
    glm::vec4 m_fillColor{0.f, 0.f, 0.f, 1.f};
    glm::vec4 m_fillColorInv{1.f, 1.f, 1.f, 1.f};
    glm::mat4 m_transform = glm::mat4(1.f);

    maskType                 m_type = maskType::Vector;
    Polygon                  m_polygon;
    Polygon                  m_polygonInv;
    std::unique_ptr<Texture> m_texture;
    ShaderCollector         *m_shCol          = nullptr;
    std::filesystem::path    m_path           = std::filesystem::path();
    std::filesystem::path    m_tempBitmapPath = std::filesystem::path();
    std::filesystem::path    m_exportPath;
    Shaders                 *m_convShader = nullptr;
    GLBase                  *m_glbase     = nullptr;
};

}  // namespace ara