#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#define PAGE_MANAGER_USE_GC 0
#define PAGE_MANAGER_USE_LOG 1

#if PAGE_MANAGER_USE_GC
#define PM_MALLOC(x) lv_mem_alloc(x);
#define FREE(x) lv_men_free(x)
#else
#define PM_MALLOC(x) malloc(x);
#define PM_FREE(x) free(x)
#endif

#if PAGE_MANAGER_USE_LOG
#define _PM_LOG(format, ...) printf("[PM]" format "\r\n", ##__VA_ARGS__)
#define PM_LOG_INFO(format, ...) _PM_LOG("[Info] " format, ##__VA_ARGS__)
#define PM_LOG_WARN(format, ...) _PM_LOG("[Warn] " format, ##__VA_ARGS__)
#define PM_LOG_ERROR(format, ...) _PM_LOG("[Error] " format, ##__VA_ARGS__)
#else
#define PM_LOG_INFO(...)
#define PM_LOG_WARN(...)
#define PM_LOG_ERROR(...)
#endif
