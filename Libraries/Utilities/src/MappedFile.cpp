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
#include "MappedFile.h"

#if defined(unix) || defined(__unix__) || defined(__unix) || defined(__MACH__)
#	include <unistd.h>
#	ifndef HAVE_MMAP
#		if _POSIX_VERSION >= 199506L
#			define HAVE_MMAP 1
#		endif /* _POSIX_VERSION */
#	endif /* HAVE_MMAP */
#endif

#ifdef _WIN32
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#elif HAVE_MMAP
#	include <fcntl.h>
#	include <sys/types.h>
#	include <sys/mman.h>
#else /* !_WIN32 && !HAVE_MMAP */
#	include <stdlib.h>
#	include <stdio.h>
#endif /* !_WIN32 && !HAVE_MMAP */

namespace ara{
std::pair<uint8_t*, size_t> MappedFile::mapFile(const std::string& path) {
#ifdef _WIN32
	return mapFileWin(path);
#elif HAVE_MMAP
	return mapFileMmap(path);
#else
	return mapFileDefault(path);
#endif
}

std::pair<uint8_t*, size_t> MappedFile::mapFileWin(const std::string& path) {
#ifdef _WIN32
  HANDLE hMap, hFile;
  try {
	hFile = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
					   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE) {
        throw("MappedFile::mapFileWin Error: CreateFileA returned INVALID_HANDLE_VALUE");
	}

	auto sz = GetFileSize(hFile, nullptr);
	if (sz == INVALID_FILE_SIZE || sz == 0) {
        throw("MappedFile::mapFileWin Error: GetFileSize returned zero");
	}

	hMap = CreateFileMappingA(hFile, nullptr, PAGE_READONLY, 0, sz, nullptr);
	if (!hMap) {
		throw("MappedFile::mapFileWin Error: CreateFileMappingA failed");
	}

	auto data = static_cast<uint8_t*>(MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, sz));

	// We can call CloseHandle here, but it will not be closed until we unmap the view
	CloseHandle(hMap);
    return {data, sz};
  } catch (const std::runtime_error& e) {
    if (hFile == INVALID_HANDLE_VALUE) {
    	CloseHandle(hFile);
    }
	return {};
  }
#else
	return {};
#endif
}

std::pair<uint8_t*, size_t> MappedFile::mapFileMmap(const std::string& path) {
#ifdef HAVE_MMAP
  	int fd{};
    try {
		fd = open(path.c_str(), O_RDONLY);
		if (fd < 0) {
           throw("MappedFile::mapFileWin Error: open failed");
		}

		// lseek returns the offset from the beginning of the file
		auto sz = lseek(fd, 0, SEEK_END);
		if (sz <= 0) {
           throw("MappedFile::mapFileWin Error: lseek failed");
		}

		// we don't need to lseek again as mmap ignores the offset
		auto data = static_cast<uint8_t*>(mmap(nullptr, sz, PROT_READ, MAP_SHARED, fd, 0));
		if (data == MAP_FAILED) {
			data = nullptr;
		}
        return {data, sz};
    } catch (const std::runtime_error& e) {
		close(fd);
        return {};
    }
#else
	return {};
#endif
}

// Note: this is just a fallback function which needs to allocate mem and thus doesn't do what this class
// is supposed to do.
std::pair<uint8_t*, size_t> MappedFile::mapFileDefault(const std::string& path) {
#if !defined(_WIN32) && !defined(HAVE_MMAP)
	FILE *fd=nullptr;
    try {
  		FILE *fd = fopen(path, "rb");
		if (!fd) {
			return nullptr;
		}

		fseek(fd, 0, SEEK_END);
		size = ftell(fd); // returns the size of the file
		if (size <= 0) {
        	throw("MappedFile::mapFileWin Error: GetFileSize returned zero");
		}

		rewind(fd);
		data = static_cast<uint8_t*>(malloc(size));
		if (!data) {
            throw("MappedFile::mapFileWin Error: GetFileSize returned zero");
		}

		// only return the data if the read was successful
		if (fread(data, size, 1, fd) != 1) {
			free(data);
			data = nullptr;
		}
		return {data, size};
	} catch (const std::runtime_error& e) {
	   	fclose(fd);
		return {};
    }
#else
	return {};
#endif
}

void MappedFile::unmap() {
#ifdef _WIN32
	UnmapViewOfFile(m_mappedFile.first);
#elif HAVE_MMAP
	munmap(m_mappedFile.first, m_mappedFile.second);
#else /* !_WIN32 && !HAVE_MMAP */
	free(m_mappedFile.first);
#endif /* !_WIN32 && !HAVE_MMAP */
}

void MappedFile::unmap(const uint8_t* data, size_t sz) {
#ifdef _WIN32
	UnmapViewOfFile(data);
#elif HAVE_MMAP
	munmap((void*)data, sz);
#else /* !_WIN32 && !HAVE_MMAP */
	free(data);
#endif /* !_WIN32 && !HAVE_MMAP */
}

}
