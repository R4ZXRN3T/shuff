//
// Created by rizel on 12.09.2026.
//

#ifndef SHUFF_TERMINAL_UTILS_H
#define SHUFF_TERMINAL_UTILS_H
#include <stdint.h>

void init_progress_bar(int64_t p_max_progress);

void finish_progress_bar(void);

void update_progress(int64_t new_progress);

#endif //SHUFF_TERMINAL_UTILS_H
