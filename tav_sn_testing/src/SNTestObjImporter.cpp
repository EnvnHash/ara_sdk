/*
 * SNTestObjImporter.cpp
 *
 *  Created on: 14.06.2016
 *      Copyright by Sven Hahne
 *      Leightweight obj importer for usage in web applikations
 */

#include "SNTestObjImporter.h"

#define STRINGIFY(A) #A

namespace tav
{

SNTestObjImporter::SNTestObjImporter(sceneData* _scd, std::map<std::string, float>* _sceneArgs):
		SceneNode(_scd, _sceneArgs), floatTextures(true)
{
	shCol = static_cast<ShaderCollector*>(_scd->shaderCollector);
	osc = static_cast<OSCData*>(scd->osc);
	winMan = static_cast<GWindowManager*>(scd->winMan);

    winMan->addKeyCallback(0, [this](int key, int scancode, int action, int mods) {
        return this->onKey(key, scancode, action, mods); });

    winMan->addCursorCallback(0, [this](double xpos, double ypos) {
        return this->onCursor(xpos, ypos); });

    
	importer = new ObjImporter(*scd->dataPath+"models/ikin_logo_3d_low.obj", true);
    
    // gles 2.0 ... gl_vertexId ist not available, do the indexing by writing
    // the indexes of the positions manually into a separate vbo (the COLOR vbo)
    std::vector<VAO*>* importVaos = importer->getVaos();
    for (std::vector<VAO*>::iterator it = importVaos->begin(); it != importVaos->end(); ++it)
    {
        GLfloat* indexes = new GLfloat[(*it)->getNrVertices() *4];
        for (unsigned int i=0;i<(*it)->getNrVertices();i++)
            indexes[i*4] = GLfloat(i);
        
        (*it)->addBuffer(tav::COLOR); // 25
        (*it)->upload(tav::COLOR, &indexes[0], (*it)->getNrVertices());
    }

    // -- object to morph to ----
    
    sphere = new Sphere(1.f, 40, true, true);

    GLfloat* pos = sphere->getPositions();
    GLfloat* norm = sphere->getNormals();
    GLfloat* posNormIntr = new GLfloat[sphere->getNrVertices() *6];
    for (int i=0;i<sphere->getNrVertices();i++)
    {
        for (int j=0;j<3;j++)
        {
            posNormIntr[i*6 +j] = pos[i*3 +j];
            posNormIntr[i*6 +3 +j] = norm[i*3 +j];
        }
    }
    
    // create a texture with the positions and normals to morph to
    if(floatTextures)
        texWidth = sphere->getNrVertices() * 6; // 3 rgba values to get a vec3
    else
        texWidth = sphere->getNrVertices() * 2;
    
    texHeight = ((int(std::sqrt(double(texWidth))) / 2) +1) * 2;
    texWidth = texHeight;

    
    positionTex = new TextureManager();
    
    if(floatTextures)
    {
        positionTex->allocate(texWidth, texHeight, GL_RGBA8, GL_RGBA, GL_TEXTURE_2D, GL_UNSIGNED_BYTE);
        positionTex->bind(0);
        
        unsigned char* posNormIntr8bit = new unsigned char[sphere->getNrVertices() *24]; // pos + norm (2) * 3 floats (3) * (4) x 8bit
        
        for (int i=0;i<sphere->getNrVertices();i++)
        {
            for (int j=0;j<3;j++)
            {
                unsigned char const* p = reinterpret_cast<unsigned char const *>(&pos[i*3 +j]);
                unsigned char const* n = reinterpret_cast<unsigned char const *>(&norm[i*3 +j]);

                for (int k=0; k<sizeof(float); k++)
                {
                    posNormIntr8bit[i*24 +j*4 +k] = p[sizeof(float) -1 -k];
                    posNormIntr8bit[i*24 +12 +j*4 +k] = n[sizeof(float) -1 -k];
                }
            }
        }
        
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texWidth, texHeight, GL_RGBA, GL_UNSIGNED_BYTE, posNormIntr8bit);
        
    } else
    {
        positionTex->allocate(texWidth, texHeight, GL_RGB16F, GL_RGB, GL_TEXTURE_2D, GL_FLOAT);
        positionTex->bind(0);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texWidth, texHeight, GL_RGB, GL_FLOAT, posNormIntr);
    }
    
    
    // --- Texturen fuer den Litsphere Shader  ---

    litsphereTex = new TextureManager();
    litsphereTex->loadTexture2D(*scd->dataPath+"/textures/litspheres/ikin_logo_lit.jpeg");
    
    bumpMap = new TextureManager();
    bumpMap->loadTexture2D(*scd->dataPath+"/textures/bump_maps/Unknown-2.jpeg");

    cubeTex = new TextureManager();
    cubeTex->loadTextureCube( ((*scd->dataPath)+"textures/skyboxsun5deg2.png").c_str() );

    osc->addPar("rotScene", 0.0f, 1.f, 0.001f, 0.f, OSCData::LIN);
    osc->addPar("heightOffs", -12.0f, 0.f, 0.001f, -1.2f, OSCData::LIN);

    if(floatTextures)
        initShaderNoFloat();
    else
        initShader();
    
    
    float testF = 1.3983098f;
    unsigned char const* p = reinterpret_cast<unsigned char const *>(&testF);
    glm::vec4 testVec = glm::vec4(p[3], p[2], p[1], p[0]) / 255.f;
    std::cout << glm::to_string(testVec) << std::endl;
    std::cout << decode32(testVec * 255.f) << std::endl;
}


float SNTestObjImporter::decode32(glm::vec4 rgba)
{
    float Sign = 1.0 - (rgba[0] < 128.0 ? 0.0 : 1.0) *2.0;
    float Exponent = 2.0 * std::fmod(rgba[0], 128.0) + (rgba[1] < 128.0 ? 0.0 : 1.0) - 127.0;
    float Mantissa = std::fmod(rgba[1], 128.0) *65536.0 + rgba[2] * 256.0 + rgba[3] + float(0x800000);
    float Result = Sign * std::exp2(Exponent) * (Mantissa * std::exp2(-23.0 ));
    return Result;
}


//----------------------------------------------------

void SNTestObjImporter::draw(double time, double dt, camPar* cp, Shaders* _shader, TFO* _tfo)
{
    if (_tfo) {
    	_tfo->setBlendMode(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    // translate model back to origin
    glm::mat4 rotMatr = glm::translate(cp->model_matrix_mat4, glm::vec3(0.f, 0.f, osc->getPar("heightOffs")));
    // rotate
    rotMatr = glm::rotate(rotMatr, float(M_PI * std::fmod(time * 0.25f, 2.f)), glm::vec3(0.f, 1.f, 0.f));

    glm::mat4 mvp = cp->projection_matrix_mat4 * cp->view_matrix_mat4 * rotMatr;
    glm::mat4 mv = cp->view_matrix_mat4 * rotMatr;
    glm::mat3 normalMat = glm::mat3(mv);

    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    litShader->begin();
    litShader->setUniformMatrix4fv("m_pvm", &mvp[0][0]);
    litShader->setUniformMatrix4fv("m_vm", &mv[0][0]);
    litShader->setUniformMatrix3fv("m_normal", &normalMat[0][0]);
    litShader->setUniform1i("litSphereTex", 0);
    litShader->setUniform1i("bumpMap", 1);
    litShader->setUniform1i("cubeTex", 2);
    litShader->setUniform1i("positionTex", 3);
    litShader->setUniform1f("blendPos", mouseX);
    litShader->setUniform1f("texWidth", float(texWidth));
    litShader->setUniform1f("texHeight", float(texHeight));
    litShader->setUniform1f("drawNrTri", float(importer->getVaos()->at(0)->getNrVertices() / 3));
    litShader->setUniform1f("obj2NrTri", float(sphere->getNrVertices() / 3));
    litShader->setUniform3f("LPosition", -1.f, 0.2f, 1.f);

    litsphereTex->bind(0);
    bumpMap->bind(1);
    cubeTex->bind(2);
    positionTex->bind(3);
    
    importer->draw();
//    sphere->draw();
}


//----------------------------------------------------

void SNTestObjImporter::initShader()
{
    std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
    std::string stdVert = STRINGIFY(layout( location = 0 ) in vec4 position;
                                    layout( location = 1 ) in vec4 normal;
                                    layout( location = 2 ) in vec2 texCoord;
                                    layout( location = 3 ) in vec4 color;   // position indices auf x

                                    uniform mat4 m_pvm;
                                    uniform mat4 m_vm;
                                    uniform mat3 m_normal;
                                    
                                    uniform sampler2D positionTex;
                                    uniform float texWidth;
                                    uniform float drawNrTri;
                                    uniform float obj2NrTri;
                                    uniform float blendPos;
                                    
                                    float ind;
                                    float triInd;
                                    float triOffs;
                                    vec2 morphObjPosTc;
                                    vec2 morphObjNormTc;

                                    vec4 mixPos;
                                    vec4 mixNorm;

                                    out vec3 n_eye;
                                    out vec4 pos_eye;

                                    void main()
                                    {
                                        triInd = float(int(color.x / 3.0));
                                        triOffs = mod(color.x, 3.0);
                                        
                                        triInd = float(int((triInd / drawNrTri) * obj2NrTri));
                                        ind = triInd * 3.0 + triOffs;
                                        
                                        morphObjPosTc = vec2(mod(ind *2.0, texWidth) / texWidth,
                                                            int(ind *2.0 / texWidth) / texWidth);
                                        morphObjPosTc += vec2(0.5 / texWidth);

                                        morphObjNormTc = vec2(mod(ind *2.0 +1.0, texWidth) / texWidth,
                                                             int((ind *2.0 +1.0) / texWidth) / texWidth);
                                        morphObjNormTc += vec2(0.5 / texWidth);

                                        mixPos = mix(position, texture(positionTex, morphObjPosTc), blendPos);
                                        mixNorm = mix(normal, texture(positionTex, morphObjNormTc), blendPos);
                                        
                                        pos_eye = m_vm * mixPos;
                                        n_eye = m_normal * mixNorm.xyz;

                                        gl_Position = m_pvm * mixPos;
                                    });

     stdVert = "// SNTestObjImporter shader\n" +shdr_Header +stdVert;

     std::string frag = STRINGIFY(in vec4 pos_eye;
                                  in vec3 n_eye;
                                  
                                  uniform sampler2D litSphereTex;
                                  uniform sampler2D bumpMap;
                                  uniform samplerCube cubeTex;
                                  uniform vec3 LPosition;

                                  vec3 bumpNorm;
                                  vec4 envColor;
                                  layout (location = 0) out vec4 color;

                                  void main()
                                  {
                                      bumpNorm = n_eye;
                                      vec3 tc = reflect(-pos_eye.xyz, normalize(n_eye));
                                      envColor = texture(cubeTex, tc);
                                      color = mix(texture(litSphereTex, bumpNorm.xy * 0.3 + vec2(0.5)), envColor, 0.2);
                                  });

     frag = "// SNTFluidDepthHeightMap shader\n"+shdr_Header+frag;

     litShader = shCol->addCheckShaderText("SNTestObjImporter_lit", stdVert.c_str(), frag.c_str());
}
    

//----------------------------------------------------

void SNTestObjImporter::initShaderNoFloat()
{
    std::string shdr_Header = "#version 410 core\n#pragma optimize(on)\n";
    std::string stdVert = STRINGIFY(layout( location = 0 ) in vec4 position;
                                    layout( location = 1 ) in vec4 normal;
                                    layout( location = 2 ) in vec2 texCoord;
                                    layout( location = 3 ) in vec4 color;   // position indices auf x
                                    
                                    uniform mat4 m_pvm;
                                    uniform mat4 m_vm;
                                    uniform mat3 m_normal;
                                    
                                    uniform sampler2D positionTex;
                                    uniform float texWidth;
                                    uniform float texHeight;
                                    uniform float drawNrTri;
                                    uniform float obj2NrTri;
                                    uniform float blendPos;
                                    
                                    int i;
                                    float ind;
                                    float triInd;
                                    float triOffs;
                                    vec2 mPosTcFirst;
                                    vec2 mPosTcSecond;
                                    vec2 mNormTcFirst;
                                    vec2 mNormTcSecond;
                                    
                                    vec4 mixPos;
                                    vec4 mixNorm;
                                    
                                    out vec3 n_eye;
                                    out vec4 pos_eye;

                                    float decode32(vec4 rgba)
                                    {
                                        float Sign = 1.0 - step(128.0, rgba[0]) *2.0;
                                        float Exponent = 2.0 * mod(rgba[0], 128.0) + step(128.0, rgba[1]) - 127.0;
                                        float Mantissa = mod(rgba[1], 128.0) *65536.0 + rgba[2] *256.0 +rgba[3] + float(0x800000);
                                        float Result = Sign * exp2(Exponent) * (Mantissa * exp2(-23.0 ));
                                        return Result;
                                    }
                                    
                                    vec4 getVert(float offsPosNorm, float triangOffs, sampler2D tex, float tWidth, float tHeight)
                                    {
                                        vec4 outPos;
                                        vec4 getCoord[3];
                                        vec2 tc[3];
                                        float arInd;
                                        
                                        // read first 8 bits
                                        for (i=0;i<3;i++)
                                        {
                                            // get two neighboured texture coordinates for the correspoding
                                            // first and second 8bits
                                            arInd = triangOffs * 6.0 + float(i) + offsPosNorm;
                                            tc[i] = vec2(mod(arInd, tWidth) / tWidth,
                                                         int(arInd / tWidth) / tHeight);
                                            tc[i] += vec2(0.5 / tWidth);    // tWidth and tHeight are the same
                                            
                                            getCoord[i] = texture(tex, tc[i]) * 255.0;
                                        }
                                        
                                        // read the texture, get the actual 8bits(as "8bit-floats")
                                        outPos = vec4(decode32(getCoord[0]), decode32(getCoord[1]),
                                                      decode32(getCoord[2]), offsPosNorm == 0.0 ? 1.0 : 0.0 );
                                        return outPos;
                                    }
                                    
                                    void main()
                                    {
                                        // bestimme den index des dreiecks
                                        triInd = float(int(color.x / 3.0));
                                        // bestimme den index innerhalb des dreiecks
                                        triOffs = mod(color.x, 3.0);
                                        
                                        // mappe den index auf das morph objekt
                                        triInd = float(int((triInd / drawNrTri) * obj2NrTri));
                                        
                                        // konvertiere den Dreieck-Index zu einem absoluten Index
                                        ind = triInd * 3.0 + triOffs;
                                        
                                        mixPos = mix(position, getVert(0.0, ind, positionTex, texWidth, texHeight), blendPos);
                                        mixNorm = mix(normal, getVert(3.0, ind, positionTex, texWidth, texHeight), blendPos);
                                        
                                        pos_eye = m_vm * mixPos;
                                        n_eye = m_normal * mixNorm.xyz;
                                        
                                        gl_Position = m_pvm * mixPos;
                                    });
    
    stdVert = "// SNTestObjImporter shader\n" +shdr_Header +stdVert;
    
    std::string frag = STRINGIFY(in vec4 pos_eye;
                                 in vec3 n_eye;
                                 
                                 uniform sampler2D litSphereTex;
                                 uniform sampler2D bumpMap;
                                 uniform samplerCube cubeTex;
                                 uniform vec3 LPosition;
                                 
                                 vec3 bumpNorm;
                                 vec4 envColor;
                                 layout (location = 0) out vec4 color;
                                 
                                 void main()
                                 {
                                     bumpNorm = n_eye;
                                     vec3 tc = reflect(-pos_eye.xyz, normalize(n_eye));
                                     envColor = texture(cubeTex, tc);
                                     color = mix(texture(litSphereTex, bumpNorm.xy * 0.3 + vec2(0.5)), envColor, 0.2);
                                 });
    
    frag = "// SNTFluidDepthHeightMap shader\n"+shdr_Header+frag;
    
    litShader = shCol->addCheckShaderText("SNTestObjImporter_lit", stdVert.c_str(), frag.c_str());
}


//----------------------------------------------------

void SNTestObjImporter::onKey(int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
    {
        if(mods == GLFW_MOD_SHIFT)
        {
        }
    }
}


//----------------------------------------------------

void SNTestObjImporter::onCursor(double xpos, double ypos)
{
    mouseX = xpos / float(scd->screenWidth);
}
    

//----------------------------------------------------

void SNTestObjImporter::update(double time, double dt)
{}



SNTestObjImporter::~SNTestObjImporter()
{
}

} /* namespace tav */
