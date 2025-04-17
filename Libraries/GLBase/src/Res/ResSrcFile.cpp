#include "Res/ResSrcFile.h"

using namespace std;

namespace ara {

bool SrcFile::Process(ResNode *root, std::vector<uint8_t> &vp) {
    vp.push_back('\n');  // marco.m_g : Important to avoid problems when having a valid
                         // closing bracket at the end or some definitions
    if (!ExtractLines(vp)) {
        return false;
    }
    return Process(root);
}

bool SrcFile::ExtractLines(std::vector<uint8_t> &vp) {
    m_Line.clear();

    if (!vp.size()) {
        return false;
    }

    std::vector<uint8_t>::iterator e    = vp.begin(), b;
    int                            lidx = 0;

    b = e;

    while (e < vp.end()) {
        if (e[0] == '\n' || e[0] == '\r') {
            m_Line.push_back(SrcLine{lidx++, string(b, e)});
            e++;

            if (e < vp.end()) {
                if (e[-1] == '\n' && e[0] == '\r') {
                    e++;
                } else if (e[-1] == '\r' && e[0] == '\n') {
                    e++;
                }
            }

            b = e;
        } else {
            e++;
        }
    }

    return true;
}

bool SrcFile::Process(ResNode *root) {
    using namespace std;

    if (root == nullptr) return false;

    int                            rpar = 0;
    int                            parp = 0;
    string                         name;
    string                         parname;
    string                         funcname;
    ParVec                         parv;
    ResNode                       *node = root;
    const char                    *e, *b;
    auto line = m_Line.begin();

    // for (SrcLine& line : m_Line) {

    while (line < m_Line.end()) {
        if (!line->isEmpty() && !line->isComment()) {
            e = b = line->str.c_str();

            do {
                e = b = clearSpaces(e);

                if (rpar) {
                    auto endIt = m_Line.end();
                    if ((e = readPar(name, e, line, endIt, node)) == nullptr)
                        return node->error((char *)"STOP.   Error reading parameters for %s", funcname.c_str());

                    if (e[0])
                        parv.push_back(name);  // this avoids adding empty pars
                                               // when next line

                    if (e[0] == ')') {
                        node->add(std::make_unique<ResNode>(parname, &(*line), m_glbase))->setFunc(funcname, parv);
                        rpar = 0;
                        parp = 0;
                        e++;
                    }
                } else {
                    auto endIt = m_Line.end();
                    if ((e = readStr(name, e, line, endIt, node)) == nullptr)
                        return node->error((char *)"STOP.   Error reading string");

                    if (e[0] == '(') {
                        parv.clear();
                        funcname = name;
                        rpar     = 1;
                        e++;

                        if (e[0] == ',') {  // special case (marco.g: should improve) -
                                            // has a comma right after the '(', this
                                            // should be donde better, but run by now
                            parv.push_back("");
                        }
                    } else {
                        if (parp) {
                            node->add(std::make_unique<ResNode>(parname, &(*line), m_glbase))->setValue(name);
                            parp = 0;
                        } else {
                            if (e[0] == '{') {
                                node = node->add(std::make_unique<ResNode>(name, &(*line),
                                                                           m_glbase));  // adding a group
                                e++;
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
                    if (node == root) return node->error((char *)"Too many closing brackets");

                    node = node->getParent();

                    parp = 0;
                    rpar = 0;
                    e++;
                } else if (e[0] == ':') {
                    parname = name;
                    parp    = 1;
                    e++;
                } else if (e[0] == ',')
                    e++;

                b = e;

            } while (e[0]);
        }

        line++;
    }

    if (node != root) return node->error((char *)"Closing bracket missing");

    return true;
}

const char *SrcFile::clearSpaces(const char *e) {
    while (e[0] && e[0] <= 32) e++;
    return e;
}

const char *SrcFile::processRawText(std::string &dest, const char *e, std::vector<SrcLine>::iterator &line,
                                    std::vector<SrcLine>::iterator &src_end, ResNode *node) {
    const char *b;

    if (strncmp(e, "!<[[", 4)) return e;

    dest.clear();
    b = e + 4;

    while (line < src_end) {
        while (e[0]) {
            if (!strncmp(e, "]]>", 3)) {
                dest += std::string(b, std::distance(b, e));
                return e + 3;
            }

            e++;
        }

        if (!e[0]) dest += std::string(b, std::distance(b, e)) + "\r\n";
        line++;
        e = b = line->str.c_str();
    }

    return nullptr;
}

const char *SrcFile::readStr(std::string &dest, const char *e, std::vector<SrcLine>::iterator &line,
                             std::vector<SrcLine>::iterator &src_end, ResNode *node) {
    const char *b;

    b = e = clearSpaces(e);

    if ((e = processRawText(dest, e, line, src_end, node)) != b) return e;

    if (e[0] == '\"') {
        e++;

        while (e[0] && e[0] != '\"') e++;

        if (e[0] == '\"') {
            dest = std::string(b + 1, std::distance(b, e) - 1);

            while (e[0] && e[0] > 32 && e[0] != '{' && e[0] != '}' && e[0] != ':' && e[0] != '(' && e[0] != ')') e++;

            return clearSpaces(e);
        }
    }

    while (e[0] && e[0] > 32 && e[0] != '{' && e[0] != '}' && e[0] != ':' && e[0] != '(' && e[0] != ')') e++;

    dest = std::string(b, std::distance(b, e));

    return clearSpaces(e);
}

const char *SrcFile::readPar(std::string &dest, const char *e, std::vector<SrcLine>::iterator &line,
                             std::vector<SrcLine>::iterator &src_end, ResNode *node) {
    const char *b;

    b = e = clearSpaces(e);

    if ((e = processRawText(dest, e, line, src_end, node)) != b) {
        if (e == nullptr) return nullptr;

        while (e[0] && e[0] <= 32 && e[0] != ',' && e[0] != ')') e++;

        return clearSpaces(e);
    }

    if (e[0] == '\"') {
        e++;

        while (e[0] && e[0] != '\"') e++;

        dest = std::string(b + 1, std::distance(b, e) - 1);

        while (e[0] && e[0] != ',' && e[0] != ')') e++;

        return clearSpaces(e);
    }

    while (e[0] && e[0] > 32 && e[0] != ',' && e[0] != ')') e++;

    dest = std::string(b, std::distance(b, e));

    while (e[0] && e[0] <= 32 && e[0] != ',' && e[0] != ')') e++;

    return clearSpaces(e);
}

bool SrcFile::error(SrcLine &line, string msg) {
    // appmsg("[ERROR] Line %d / %s",line.index+1,msg.c_str());

    return false;
}

}  // namespace ara