// esp_hw_support: but its behavior is same with assert()
#pragma once

#include <assert.h>
#define ESP_FAULT_ASSERT(CONDITION)     assert(CONDITION)
