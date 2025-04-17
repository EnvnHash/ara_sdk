//
// Created by user on 04.02.2021.
//

#pragma once

#include "Res/ResNode.h"

namespace ara {

class SrcFile {
public:
    SrcFile(GLBase *glbase) : m_glbase(glbase) {}

    virtual ~SrcFile() = default;

    std::vector<SrcLine> m_Line;

    bool Process(ResNode *root, std::vector<uint8_t> &vp);

private:
    bool ExtractLines(std::vector<uint8_t> &vp);
    bool Process(ResNode *root);

    const char *readStr(std::string &dest, const char *e, std::vector<SrcLine>::iterator &line,
                        std::vector<SrcLine>::iterator &src_end, ResNode *node);

    const char *readPar(std::string &dest, const char *e, std::vector<SrcLine>::iterator &line,
                        std::vector<SrcLine>::iterator &src_end, ResNode *node);

    bool error(SrcLine &line, std::string msg);

    GLBase *m_glbase = nullptr;

public:
    static const char *clearSpaces(const char *e);

    const char *processRawText(std::string &dest, const char *e, std::vector<SrcLine>::iterator &line,
                               std::vector<SrcLine>::iterator &src_end, ResNode *node);
};

}  // namespace ara
