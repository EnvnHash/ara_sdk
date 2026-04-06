#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <util_common.h>
#include <Conditional.h>
#include "threadpool/BS_thread_pool.hpp"

namespace ara {

static inline BS::thread_pool g_thread_pool;

}