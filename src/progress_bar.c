#include "progress_bar.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#define FILLED_STRING "#"
#define UNFILLED_STRING " "

#define MAX_CHARS 70

static int64_t max_progress = 0;
static uint8_t percent = 0;
static uint8_t chars_filled = 0;

static bool initialized = false;

static uint8_t round_uint8(const double input) {
	return (uint8_t) ((input < 0.5 ? 0 : input) > 254.5 ? 255 : (input < 0.5 ? 0 : input) + 0.5);
}

static void print_progress(void) {
	printf("\033[0G\033[2K");
	printf("[");
	for (int i = 0; i < chars_filled; ++i) printf(FILLED_STRING);
	for (int i = 0; i < MAX_CHARS - chars_filled; ++i) printf(UNFILLED_STRING);
	printf("] %d%%", percent);
}

void init_progress_bar(const int64_t p_max_progress) {
	if (initialized) return;
	max_progress = p_max_progress;
	percent = 0;
	chars_filled = 0;
	initialized = true;
}

void finish_progress_bar(void) {
	if (!initialized) return;
	initialized = false;
	percent = 100;
	chars_filled = MAX_CHARS;
	print_progress();
	printf("\n");
	percent = 0;
	max_progress = 0;
	chars_filled = 0;
}

void update_progress(const int64_t new_progress) {
	if (!initialized) return;
	if (new_progress >= max_progress) {
		finish_progress_bar();
		return;
	}
	const uint8_t new_percentage = round_uint8((double) new_progress / (double) max_progress * 100.0);
	const uint8_t new_chars_filled = round_uint8((double) MAX_CHARS * (double) new_progress / (double) max_progress);
	if (new_percentage != percent || new_chars_filled != chars_filled) {
		chars_filled = new_chars_filled;
		percent = new_percentage;
		print_progress();
	}
}
