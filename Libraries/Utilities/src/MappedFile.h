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

class MappedFile {
public:
	/*!
	 * Maps the named file into memory.
	 * The file is mapped into memory. All filehandles are closed afterwards.
	 * \param path path of the file being mapped
	 * \exception IOException the file couldn't be opened
	 */
	explicit MappedFile(const std::string& path) {
		m_mappedFile = mapFile(path);
	}

	/* Unmaps the file and releases all memory. */
	~MappedFile() {
		// unmapFile();
	}

	/*!
	 * Maps the specified file into memory.
	 * \param path the path of the file being mapped
	 * \return the pointer to the mapping and the file size. On failure nullptr and 0 is returned.
	 */
	std::pair<uint8_t*, size_t> mapFile(const std::string& path);

	std::pair<uint8_t*, size_t> mapFileWin(const std::string& path);
	std::pair<uint8_t*, size_t> mapFileMmap(const std::string& path);
	std::pair<uint8_t*, size_t> mapFileDefault(const std::string& path);

	/*! Releases memory allocated by map_file(). */
	void unmap();

	static void unmap(const uint8_t*, size_t);

	/* Get the size of the file in memory.  */
	[[nodiscard]] auto size() const { return m_mappedFile.second; }

	/* Gets the nth byte from the mapped file. */
	uint8_t operator[](const std::size_t n) const { return m_mappedFile.first[n]; }

	/* Gets a read-only pointer to the mapped data.	 */
	const uint8_t* operator()() const { return m_mappedFile.first; }

private:
	std::pair<uint8_t*, size_t> m_mappedFile;
};

}
