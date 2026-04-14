
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

#include <Log.h>
#include <TypeName.h>

#include <algorithm>
#include <any>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <list>
#include <map>
#include <nameof.hpp>
#include <random>
#include <ranges>
#include <thread>
#include <variant>
#include <typeindex>
#include <unordered_map>

#include <Constants.h>

#include <glm/glm.hpp>
#include <type_traits>
#include "json/json.hpp"

#ifdef __ANDROID__
#include <jni.h>
#endif

namespace ara {

enum class restCallType : int32_t { post = 0, get, downloadBuffer, downloadFile };

enum class tpi : int32_t {
    tp_string = 0,
    tp_char,
    tp_path,
    tp_int8,
    tp_uint8,
    tp_int16,
    tp_uint16,
    tp_int32,
    tp_uint32,
    tp_int64,
    tp_uint64,
    tp_float,
    tp_double,
    tp_bool,
    tp_vector_int32,
    tp_vector_float,
    tp_vector_string,
    tp_ivec2,      // glm::ivec2
    tp_ivec3,      // glm::ivec3
    tp_ivec4,      // glm::ivec4
    tp_vec2,       // glm::vec2
    tp_vec3,       // glm::vec3
    tp_vec4,       // glm::vec4
    none,
    count
};

using nodeValue = std::variant<std::string, int32_t, float, bool>;
enum class nodeValueType : int32_t { undefined=0, boolean=1, floating=2, integer=3, string=4, array=5, object=6, root=7 };

template <typename T>
constexpr bool is_glm_vec_v =
    std::is_same_v<T, glm::ivec2> ||
    std::is_same_v<T, glm::ivec3> ||
    std::is_same_v<T, glm::ivec4> ||
    std::is_same_v<T, glm::vec2> ||
    std::is_same_v<T, glm::vec3> ||
    std::is_same_v<T, glm::vec4>;

template <typename T>
struct tpi_of_scalar {
    static constexpr auto value = tpi::none;
};

template <> struct tpi_of_scalar<std::string>           { static constexpr auto value = tpi::tp_string; };
template <> struct tpi_of_scalar<std::filesystem::path> { static constexpr auto value = tpi::tp_path; };
template <> struct tpi_of_scalar<char>                  { static constexpr auto value = tpi::tp_char; };
template <> struct tpi_of_scalar<int8_t>                { static constexpr auto value = tpi::tp_int8; };
template <> struct tpi_of_scalar<uint8_t>               { static constexpr auto value = tpi::tp_uint8; };
template <> struct tpi_of_scalar<int16_t>               { static constexpr auto value = tpi::tp_int16; };
template <> struct tpi_of_scalar<uint16_t>              { static constexpr auto value = tpi::tp_uint16; };
template <> struct tpi_of_scalar<int32_t>               { static constexpr auto value = tpi::tp_int32; };
template <> struct tpi_of_scalar<uint32_t>              { static constexpr auto value = tpi::tp_uint32; };
template <> struct tpi_of_scalar<int64_t>               { static constexpr auto value = tpi::tp_int64; };
template <> struct tpi_of_scalar<uint64_t>              { static constexpr auto value = tpi::tp_uint64; };
template <> struct tpi_of_scalar<float>                 { static constexpr auto value = tpi::tp_float; };
template <> struct tpi_of_scalar<double>                { static constexpr auto value = tpi::tp_double; };
template <> struct tpi_of_scalar<bool>                  { static constexpr auto value = tpi::tp_bool; };

template <typename T>
constexpr tpi tpiOfScalar() {
    return tpi_of_scalar<std::decay_t<T>>::value;
}

template <typename T>
constexpr bool is_supported_container_element_v =
    std::is_same_v<T, int32_t> ||
    std::is_same_v<T, float>   ||
    std::is_same_v<T, std::string> ||
    std::is_same_v<T, std::filesystem::path>;

template <typename T>
constexpr tpi glmTpiOf() {
    if constexpr (std::is_same_v<T, glm::ivec2>) return tpi::tp_ivec2;
    else if constexpr (std::is_same_v<T, glm::ivec3>) return tpi::tp_ivec3;
    else if constexpr (std::is_same_v<T, glm::ivec4>) return tpi::tp_ivec4;
    else if constexpr (std::is_same_v<T, glm::vec2>)  return tpi::tp_vec2;
    else if constexpr (std::is_same_v<T, glm::vec3>)  return tpi::tp_vec3;
    else if constexpr (std::is_same_v< T, glm::vec4>) return tpi::tp_vec4;
    else return tpi::none;
}

template <typename Elem>
constexpr tpi vectorTpiOf() {
    if constexpr (std::is_same_v<Elem, int32_t>) {
        return tpi::tp_vector_int32;
    } else if constexpr (std::is_same_v<Elem, float>) {
        return tpi::tp_vector_float;
    } else {
        return tpi::tp_vector_string;   // std::string
    }
}

template <typename T>
constexpr tpi getTpi() {
    using Decayed = std::decay_t<T>;

    if constexpr (std::is_same_v<Decayed, std::string>) {
        return tpiOfScalar<Decayed>();
    } else if constexpr (std::is_same_v<Decayed, std::filesystem::path>) {
        return tpiOfScalar<Decayed>();
    } else if constexpr (!std::is_class_v<T>) {
        return tpiOfScalar<Decayed>();
    } else if constexpr (is_glm_vec_v<Decayed>) {
        return glmTpiOf<Decayed>();
    } else if constexpr (requires { typename Decayed::value_type; std::declval<Decayed>().size(); } &&
           std::same_as<Decayed, std::vector<typename Decayed::value_type>>) {
        using Elem = Decayed::value_type;
        if constexpr (is_supported_container_element_v<Elem>) {
            return vectorTpiOf<Elem>();
        }
    }

    return tpi::none;
}

template<typename T>
concept CoordinateType = std::is_integral_v<T> || std::is_floating_point_v<T>;

template<typename T>
concept CoordinateType32Signed = std::is_same_v<T, int32_t> || std::is_same_v<T, float> ;

template<typename T>
concept PropertyType = std::is_same_v<T, std::string> || std::is_integral_v<T> || std::is_floating_point_v<T>;

template<class T>
constexpr bool isSupportedNodeValue_v =
    std::is_same_v<T, bool>          ||
    std::is_same_v<T, std::string>   ||
    std::is_same_v<T, std::filesystem::path>   ||
    std::is_integral_v<T>            ||
    std::is_floating_point_v<T>      ||
    is_glm_vec_v<T>;

template<class T>
concept NodeValueType = isSupportedNodeValue_v<std::decay_t<T>> || (
    requires { typename T::value_type; } &&
    std::is_class_v<T> &&                       // ensures we have a nested value_type
    std::tuple_size_v<T> > 0 &&                 // non‑empty array (size known at compile time)
    isSupportedNodeValue_v<typename T::value_type>);

template <typename TP>
std::time_t to_time_t(TP tp) {
    using namespace std::chrono;
    auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now() + system_clock::now());
    return system_clock::to_time_t(sctp);
}

static std::string generateUUID() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    auto hex = [](const int n) -> char {
        if (n < 10){
            return static_cast<char>('0' + n);
        }
        return static_cast<char>('A' + (n - 10));
    };
    std::stringstream ss;
    // Generate the UUID format: 8-4-4-4-12 hexadecimal digits
    for (int i = 0; i < 36; ++i) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            ss << '-';
        } else {
            ss << hex(dis(gen));
        }
    }
    return ss.str();
}

static float getRandF(const float min, const float max) {
    static std::random_device rd;          // Get a random number from hardware
    static std::mt19937 gen(rd());       // Seed the generator
    std::uniform_real_distribution<> dis(min, max); // Define the range
    return static_cast<float>(dis(gen));
}

// calculates value in between the indices of an array
// inInd ranges from 0.0f to arraySize
static float interpolVal(const float inInd, const int arraySize, const float *array) {
    float outVal   = 0.0f;
    const auto lowerInd = static_cast<int>(std::floor(inInd));
    const auto upperInd = static_cast<int>(std::min(static_cast<float>(lowerInd + 1), static_cast<float>(arraySize - 1)));

    if (const auto weight   = inInd - static_cast<float>(lowerInd); weight == 0.0) {
        outVal = array[lowerInd];
    } else {
        outVal = array[lowerInd] * (1.0f - weight) + array[upperInd] * weight;
    }
    return outVal;
}

// calculates value in between the indices of an array
// index ranges from 0-1
static float interpolVal2(const float inInd, const int arraySize, const float *array) {
    float outVal     = 0.0f;
    const auto  fArraySize = static_cast<float>(arraySize);
    const float fInd       = std::fmod(inInd, 1.0f) * (fArraySize - 1.0f);

    const auto lowerInd = static_cast<int>(std::floor(fInd));
    const auto upperInd = static_cast<int>(std::min(static_cast<float>(lowerInd + 1), fArraySize - 1.0f));

    if (const auto weight   = fInd - static_cast<float>(lowerInd); weight == 0.0) {
        outVal = array[lowerInd];
    } else {
        outVal = array[lowerInd] * (1.0f - weight) + array[upperInd] * weight;
    }
    return outVal;
}

// calculates value in between the indices of an array
// enter index ranges from 0-1
static float interpolVal(const float inInd, const int arraySize, const std::vector<float> *array) {
    float outVal     = 0.0f;
    const auto  fArraySize = static_cast<float>(arraySize);
    const float fInd       = std::fmod(inInd, 1.0f) * (fArraySize - 1.0f);

    const auto lowerInd = static_cast<int>(std::floor(fInd));
    const auto upperInd = static_cast<int>(std::min(static_cast<float>(lowerInd + 1), fArraySize - 1.0f));

    if (const auto weight   = fInd - static_cast<float>(lowerInd); weight == 0.0) {
        outVal = array->at(lowerInd);
    } else {
        outVal = array->at(lowerInd) * (1.0f - weight) + array->at(upperInd) * weight;
    }

    return outVal;
}

static std::string formatFileTime(const std::filesystem::file_time_type& time) {
    std::stringstream ss;
    ss << std::format("{}", time);
    return ss.str();
}

}  // namespace ara

namespace glm {

namespace glm_json {
    template <typename Vec, std::size_t N>
    concept GlmVec = requires(Vec v) {
        typename Vec::value_type;
        { v[0] } -> std::convertible_to<typename Vec::value_type>;
    } && Vec::length() == N;

    template <typename Vec>
    requires requires { Vec::length(); }
    void to_json(nlohmann::json& j, Vec& v) {
        constexpr std::size_t N = Vec::length();
        j = nlohmann::json::array();

        for (std::size_t i = 0; i < N; ++i) {
            j.emplace_back(v[i]);
        }
    }

    template <typename Vec>
    requires requires { Vec::length(); }
    void from_json(const nlohmann::json& j, Vec& v) {
        constexpr std::size_t N = Vec::length();

        if (!j.is_array() || j.size() != N) {
            throw std::runtime_error("Invalid JSON array size for glm vector");
        }

        for (std::size_t i = 0; i < N; ++i) {
            v[i] = j.at(i).get<typename Vec::value_type>();
        }
    }
}

template <typename T, qualifier Q>
void from_json(const nlohmann::json& j, vec<2, T, Q>& v) {
    glm_json::from_json(j, v);
}

template <typename T, qualifier Q>
void from_json(const nlohmann::json& j, vec<3, T, Q>& v) {
    glm_json::from_json(j, v);
}

template <typename T, qualifier Q>
void from_json(const nlohmann::json& j, vec<4, T, Q>& v) {
    glm_json::from_json(j, v);
}

template <typename T, qualifier Q>
void to_json(nlohmann::json& j, const vec<2, T, Q>& v) {
    glm_json::to_json(j, v);
}

template <typename T, qualifier Q>
void to_json(nlohmann::json& j, const vec<3, T, Q>& v) {
    glm_json::to_json(j, v);
}

template <typename T, qualifier Q>
void to_json(nlohmann::json& j, const vec<4, T, Q>& v) {
    glm_json::to_json(j, v);
}

}