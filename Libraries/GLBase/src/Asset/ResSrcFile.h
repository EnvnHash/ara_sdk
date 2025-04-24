//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#pragma once

#include "Asset/ResNode.h"

namespace ara {

class SrcFile {
public:
    explicit SrcFile(GLBase *glbase) : m_glbase(glbase) {}
    virtual ~SrcFile() = default;

    bool process(ResNode *root, std::vector<uint8_t> &vp);

    std::vector<SrcLine> m_line;

private:
    static void                skipNewline(std::vector<uint8_t>::iterator& it, std::vector<uint8_t> &vp);
    bool                extractLines(std::vector<uint8_t> &vp);
    bool                process(ResNode *root);
    static const char*  readStr(std::string &dest, const char *e, std::vector<SrcLine>::iterator &line,
                                const std::vector<SrcLine>::iterator &src_end);
    static const char*  readPar(std::string &dest, const char *e, std::vector<SrcLine>::iterator &line,
                                const std::vector<SrcLine>::iterator &src_end);
    static bool         error(SrcLine &line, const std::string& msg);

    GLBase* m_glbase = nullptr;

public:
    static const char * clearSpaces(const char *e);
    static const char * processRawText(std::string &dest, const char *e, std::vector<SrcLine>::iterator &line,
                                       const std::vector<SrcLine>::iterator &src_end);
};

}  // namespace ara
