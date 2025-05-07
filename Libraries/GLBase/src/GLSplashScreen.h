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

#ifdef ARA_USE_GLFW

#include "GeoPrimitives/Quad.h"
#include "Shaders/ShaderCollector.h"
#include "Utils/Texture.h"
#include "WindowManagement/GLWindow.h"

namespace ara {

class GLSplashScreen {
public:
    void open();
    void close();

private:
    GLWindow                         m_win{};
    std::unique_ptr<Quad>            m_quad;
    std::unique_ptr<ShaderCollector> m_shCol;
    Shaders                         *m_stdTex{};
    Texture                          m_tex;
};

}  // namespace ara

#endif