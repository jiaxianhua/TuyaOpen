#pragma once

#include <stddef.h>

int epub_extract_text(const char *epub_path, const char *cache_dir, char *out_text_path, size_t out_text_path_len);
