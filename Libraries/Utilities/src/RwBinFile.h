
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

#include <util_common.h>

namespace ara {
std::size_t ReadBinFile(std::vector<uint8_t> &vp, const std::filesystem::path& filepath);
std::size_t WriteBinFile(std::vector<uint8_t> &vp, const std::filesystem::path& filepath);
std::size_t WriteBinFile(void *buff, std::size_t size, const std::filesystem::path& filepath);
}  // namespace ara