
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
#include <thread>
#include <variant>
#include <unordered_map>

#ifdef __ANDROID__
#include <jni.h>
#endif

namespace ara {

enum class restCallType : int { post = 0, get, downloadBuffer, downloadFile };

enum class tpi : int {
    tp_string = 0,
    tp_char,
    tp_int32,
    tp_uint32,
    tp_float,
    tp_double,
    tp_int64,
    tp_uint64,
    tp_bool,
    none,
    count
};

// helper enum for performance optimization. typeid can't be stored,
// type_info.name() is too costly

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
    auto hex = [](int n) -> char {
        if (n < 10){
            return static_cast<char>('0' + n);
        } else {
            return static_cast<char>('A' + (n - 10));
        }
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

static float getRandF(float min, float max) {
    static std::random_device rd;          // Obtain a random number from hardware
    static std::mt19937 gen(rd());       // Seed the generator
    std::uniform_real_distribution<> dis(min, max); // Define the range
    return static_cast<float>(dis(gen));
}

// calculates value in between the indices of an array
// inInd ranges from 0.0f to arraySize
static float interpolVal(float inInd, int arraySize, const float *array) {
    float outVal   = 0.0f;
    auto lowerInd = static_cast<int>(std::floor(inInd));
    auto upperInd = static_cast<int>(std::min(static_cast<float>(lowerInd + 1), static_cast<float>(arraySize - 1)));
    auto weight   = inInd - static_cast<float>(lowerInd);

    if (weight == 0.0) {
        outVal = array[lowerInd];
    } else {
        outVal = array[lowerInd] * (1.0f - weight) + array[upperInd] * weight;
    }
    return outVal;
}

// calculates value in between the indices of an array
// index ranges from 0-1
static float interpolVal2(float inInd, int arraySize, const float *array) {
    float outVal     = 0.0f;
    auto  fArraySize = static_cast<float>(arraySize);
    float fInd       = std::fmod(inInd, 1.0f) * (fArraySize - 1.0f);

    auto lowerInd = static_cast<int>(std::floor(fInd));
    auto upperInd = static_cast<int>(std::min(static_cast<float>(lowerInd + 1), fArraySize - 1.0f));
    auto weight   = fInd - static_cast<float>(lowerInd);

    if (weight == 0.0) {
        outVal = array[lowerInd];
    } else {
        outVal = array[lowerInd] * (1.0f - weight) + array[upperInd] * weight;
    }
    return outVal;
}

// calculates value in between the indices of an array
// enter index ranges from 0-1
static float interpolVal(float inInd, int arraySize, const std::vector<float> *array) {
    float outVal     = 0.0f;
    auto  fArraySize = static_cast<float>(arraySize);
    float fInd       = std::fmod(inInd, 1.0f) * (fArraySize - 1.0f);

    auto lowerInd = static_cast<int>(std::floor(fInd));
    auto upperInd = static_cast<int>(std::min(static_cast<float>(lowerInd + 1), fArraySize - 1.0f));
    auto weight   = fInd - static_cast<float>(lowerInd);

    if (weight == 0.0) {
        outVal = array->at(lowerInd);
    } else {
        outVal = array->at(lowerInd) * (1.0f - weight) + array->at(upperInd) * weight;
    }

    return outVal;
}

}  // namespace ara