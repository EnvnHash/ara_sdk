#include "ResFont.h"

#include "Res/ResGlFont.h"

using namespace std;

namespace ara {

bool ResFont::OnProcess() {
    if ((m_Size = value1i("size", 0)) <= 0) {
        return error((char *)"font: Invalid size (%d)", m_Size);
    }

    if (getFlag("bold")) {
        m_Flags |= bold;
    }

    if (getFlag("italic")) {
        m_Flags |= italic;
    }

    m_FontPath = getValue("font");

    if (m_FontPath.empty()) {
        m_FontPath = getValue("src");
    }

    if (m_FontPath.empty()) {
        return error((char *)"No font path");
    }

    return true;
}

bool ResFont::OnResourceChange(bool deleted, const string &res_fpath) {
    if (deleted){
        return false;
    }
    return true;
}

}  // namespace ara
