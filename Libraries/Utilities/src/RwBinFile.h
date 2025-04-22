#pragma once

#include <util_common.h>

namespace ara {
std::size_t ReadBinFile(std::vector<uint8_t> &vp, const std::filesystem::path& filepath);
std::size_t WriteBinFile(std::vector<uint8_t> &vp, const std::filesystem::path& filepath);
std::size_t WriteBinFile(void *buff, std::size_t size, const std::filesystem::path& filepath);
}  // namespace ara