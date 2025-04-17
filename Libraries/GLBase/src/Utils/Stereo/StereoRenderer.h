//
// Created by sven on 18-10-22.
//

#pragma once

#ifdef __ANDROID__
/*#include <Android/carboard/Cardboard.h>
namespace ara {
    using StereoRenderer = cardboard::Cardboard;
}*/
#include <Utils/Stereo/StereoRendererGeneral.h>
namespace ara {
using StereoRenderer = StereoRendererGeneral;
}
#else

#include <Utils/Stereo/StereoRendererGeneral.h>

namespace ara {
using StereoRenderer = StereoRendererGeneral;
}
#endif