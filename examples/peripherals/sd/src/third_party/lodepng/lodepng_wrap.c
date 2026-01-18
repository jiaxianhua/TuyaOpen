#include <string.h>
#include "tal_memory.h"

#define LV_USE_PNG 1
#define LV_PNG_USE_PSRAM 0
#define LV_UNUSED(x) ((void)(x))
#define lv_memcpy memcpy
#define lv_memset memset

#define LODEPNG_NO_COMPILE_DISK
#define LODEPNG_NO_COMPILE_ENCODER
#define LODEPNG_NO_COMPILE_ANCILLARY_CHUNKS
#define LODEPNG_NO_COMPILE_CPP
#define LODEPNG_NO_COMPILE_ALLOCATORS

#include "lodepng.h"

void* lodepng_malloc(size_t size) { return tal_malloc(size); }
void* lodepng_realloc(void* ptr, size_t new_size) { return tal_realloc(ptr, new_size); }
void lodepng_free(void* ptr) { tal_free(ptr); }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#include "../../../../../../platform/T5AI/t5_os/ap/components/lvgl/src/extra/libs/png/lodepng.c"
#pragma GCC diagnostic pop
