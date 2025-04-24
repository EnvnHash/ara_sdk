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

#include "Asset/ResSrcFile.h"

using namespace std;

namespace ara {

bool SrcFile::process(ResNode *root, std::vector<uint8_t> &vp) {
    vp.emplace_back('\n');
    if (!extractLines(vp)) {
        return false;
    }
    return process(root);
}

void SrcFile::skipNewline(std::vector<uint8_t>::iterator& it, std::vector<uint8_t> &vp) {
    if (it < vp.end() && (*it == '\n' || *it == '\r')) {
        ++it;
        if (it < vp.end() && (*it == '\n' || *it == '\r')) {
            ++it;
        }
    }
}

bool SrcFile::extractLines(std::vector<uint8_t> &vp) {
    m_line.clear();

    if (vp.empty()) {
        return false;
    }

    auto e = vp.begin();
    int lidx = 0;
    auto b = e;

    while (e < vp.end()) {
        if (e[0] == '\n' || e[0] == '\r') {
            m_line.emplace_back(SrcLine{lidx++, string(b, e)});
            skipNewline(e, vp); // Use the helper function
            b = e;
        } else {
            ++e;
        }
    }
    return true;
}


bool SrcFile::process(ResNode *root) {
    if (!root) {
        return false;
    }

    int                            rpar = 0;
    int                            parp = 0;
    string                         name;
    string                         parname;
    string                         funcname;
    ParVec                         parv;
    ResNode                       *node = root;
    auto line = m_line.begin();

    while (line < m_line.end()) {
        if (!line->isEmpty() && !line->isComment()) {
            const char *e = line->str.c_str();

            do {
                e = clearSpaces(e);

                if (rpar) {
                    auto endIt = m_line.end();
                    if ((e = readPar(name, e, line, endIt)) == nullptr) {
                        LOGE << "STOP.   Error reading parameters for " << funcname;
                        return false;
                    }

                    if (e[0]) {
                        parv.push_back(name);  // this avoids adding empty pars when next line
                    }

                    if (e[0] == ')') {
                        node->add(std::make_unique<ResNode>(parname, &(*line), m_glbase))->setFunc(funcname, parv);
                        rpar = 0;
                        parp = 0;
                        ++e;
                    }
                } else {
                    auto endIt = m_line.end();
                    if ((e = readStr(name, e, line, endIt)) == nullptr) {
                        LOGE << "STOP.   Error reading string";
                        return false;
                    }

                    if (e[0] == '(') {
                        parv.clear();
                        funcname = name;
                        rpar     = 1;
                        ++e;

                        if (e[0] == ',') {  // special case - has a comma right after the '(',
                                            // this should be done a better way, but run by now
                            parv.push_back("");
                        }
                    } else {
                        if (parp) {
                            node->add(std::make_unique<ResNode>(parname, &(*line), m_glbase))->setValue(name);
                            parp = 0;
                        } else {
                            if (e[0] == '{') {
                                node = node->add(std::make_unique<ResNode>(name, &(*line), m_glbase));  // adding a group
                                ++e;
                            } else {
                                if (!name.empty()) {
                                    if (e[0] != ':' || e[0] == '}' || !e[0]) {
                                        node->add(std::make_unique<ResNode>(name, &(*line), m_glbase));
                                    }
                                }
                            }
                        }
                    }
                }

                if (e[0] == '}') {
                    if (node == root) {
                        LOGE << "Too many closing brackets";
                        return false;
                    }

                    node = node->getParent();
                    parp = 0;
                    rpar = 0;
                    ++e;
                } else if (e[0] == ':') {
                    parname = name;
                    parp    = 1;
                    ++e;
                } else if (e[0] == ',') {
                    ++e;
                }
            } while (e[0]);
        }

        ++line;
    }

    if (node != root) {
        LOGE << "Closing bracket missing";
        return true;
    }

    return true;
}

const char *SrcFile::clearSpaces(const char *e) {
    while (e[0] && e[0] <= 32) {
        ++e;
    }
    return e;
}

const char *SrcFile::processRawText(std::string &dest, const char *e, std::vector<SrcLine>::iterator &line,
                                    const std::vector<SrcLine>::iterator &src_end) {
    if (strncmp(e, "!<[[", 4)) {
        return e;
    }

    dest.clear();
    auto b = e + 4;

    while (line < src_end) {
        while (e[0]) {
            if (!strncmp(e, "]]>", 3)) {
                dest += std::string(b, std::distance(b, e));
                return e + 3;
            }
            ++e;
        }

        if (!e[0]) {
            dest += std::string(b, std::distance(b, e)) + "\r\n";
        }
        ++line;
        e = b = line->str.c_str();
    }

    return nullptr;
}

const char *SrcFile::readStr(std::string &dest, const char *e, std::vector<SrcLine>::iterator &line,
                             const std::vector<SrcLine>::iterator &src_end) {
    auto b = e = clearSpaces(e);

    if ((e = processRawText(dest, e, line, src_end)) != b) {
        return e;
    }

    if (e[0] == '\"') {
        ++e;

        while (e[0] && e[0] != '\"') {
            ++e;
        }

        if (e[0] == '\"') {
            dest = std::string(b + 1, std::distance(b, e) - 1);

            while (e[0] && e[0] > 32 && e[0] != '{' && e[0] != '}' && e[0] != ':' && e[0] != '(' && e[0] != ')') {
                ++e;
            }

            return clearSpaces(e);
        }
    }

    while (e[0] && e[0] > 32 && e[0] != '{' && e[0] != '}' && e[0] != ':' && e[0] != '(' && e[0] != ')') {
        ++e;
    };

    dest = std::string(b, std::distance(b, e));
    return clearSpaces(e);
}

const char *SrcFile::readPar(std::string &dest, const char *e, std::vector<SrcLine>::iterator &line,
                             const std::vector<SrcLine>::iterator &src_end) {
    const char *b = e = clearSpaces(e);

    if ((e = processRawText(dest, e, line, src_end)) != b) {
        if (e == nullptr) {
            return nullptr;
        }

        while (e[0] && e[0] <= 32 && e[0] != ',' && e[0] != ')') {
            ++e;
        }

        return clearSpaces(e);
    }

    if (e[0] == '\"') {
        ++e;

        while (e[0] && e[0] != '\"') {
            ++e;
        }

        dest = std::string(b + 1, std::distance(b, e) - 1);

        while (e[0] && e[0] != ',' && e[0] != ')') {
            ++e;
        }

        return clearSpaces(e);
    }

    while (e[0] && e[0] > 32 && e[0] != ',' && e[0] != ')') {
        ++e;
    }

    dest = std::string(b, std::distance(b, e));

    while (e[0] && e[0] <= 32 && e[0] != ',' && e[0] != ')') {
        ++e;
    }

    return clearSpaces(e);
}

bool SrcFile::error(SrcLine &line, const string& msg) {
    return false;
}

}  // namespace ara