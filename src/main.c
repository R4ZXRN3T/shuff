#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <locale.h>

#include "huffman.h"
#include "progress_bar.h"

#ifdef _WIN32
typedef __int64 file_offset_t;
#define file_seek _fseeki64
#define file_tell _ftelli64
#else
#define file_seek fseek
#define file_tell ftell
#endif

#define CHUNK 65536

static void print_usage(void) {
	printf(
		"\nUsage:\n\nshuff [<action>] [<input>] [<output (optional)>]\n\naction:\tEither 'encode' or 'decode'\ninput:\tA valid os path to a file.\noutput:\tA valid os path to a file. Output file may be overwritten.");
}

static char *make_encoded_filename(const char *input_filename) {
	const size_t length = strlen(input_filename);

	const char extension[] = ".shuf";
	const size_t extension_length = sizeof(extension) - 1;

	char *output = malloc(length + extension_length + 1);

	if (output == NULL) {
		return NULL;
	}

	memcpy(output, input_filename, length);
	memcpy(output + length, extension, extension_length);

	output[length + extension_length] = '\0';

	return output;
}

static char *toggle_shuf_extension(const char *filename) {
	const char *shuf = ".shuf";
	const size_t filename_len = strlen(filename);
	const size_t shuf_len = strlen(shuf);

	// Case 1: filename ends in ".shuf" -> remove it
	if (filename_len >= shuf_len && strcmp(filename + filename_len - shuf_len, shuf) == 0) {
		const size_t new_len = filename_len - shuf_len;
		char *result = malloc(new_len + 1);

		if (result == NULL)return NULL;

		memcpy(result, filename, new_len);
		result[new_len] = '\0';

		return result;
	}

	// Case 2: filename doesn't end in ".shuf" -> add "(1)"
	// Find the last '.' in the filename.
	const char *dot = strrchr(filename, '.');

	size_t base_len;
	size_t extension_len;

	if (dot != NULL && dot != filename) {
		// There is an extension.
		base_len = (size_t) (dot - filename);
		extension_len = filename_len - base_len;
	} else {
		// No extension.
		base_len = filename_len;
		extension_len = 0;
	}

	// +3 for "(1)", +1 for '\0'
	char *result = malloc(base_len + 3 + extension_len + 1);

	if (result == NULL)return NULL;

	memcpy(result, filename, base_len);
	memcpy(result + base_len, "(1)", 3);
	memcpy(result + base_len + 3, filename + base_len, extension_len);

	result[base_len + 3 + extension_len] = '\0';

	return result;
}

unsigned int digits(const size_t n) {
	unsigned int count = 1;
	size_t m = n;

	while (m >= 10) {
		m /= 10;
		count++;
	}

	return count;
}

unsigned int split_digits(const size_t n) {
	return digits(n) + (digits(n) - 1) / 3;
}

void print_number(const int n) {
	if (n >= 1000) {
		print_number(n / 1000);
		printf(".%03d", n % 1000);
	} else printf("%d", n);
}

static void print_size_summary(const char *title, const size_t input_size, const size_t output_size) {
	const unsigned int total_size_length = split_digits(input_size) + split_digits(output_size) + 4;
	const unsigned int size_length_before = (unsigned int) (((double) total_size_length - 9.0) / 2.0);
	const unsigned int size_length_after = (unsigned int) (((double) total_size_length - 9.0) / 2 + 0.5 + 7.0);

	printf("\n%s\n\n", title);
	for (int i = 0; i < (int) size_length_before; ++i) printf(" ");
	printf("File size");
	for (int i = 0; i < (int) size_length_after; ++i) printf(" ");
	printf("Ratio\n");
	for (int i = 0; i < (int) total_size_length; ++i) printf("-");
	printf("    ----------\n");
	print_number((int) input_size);
	printf(" -> ");
	print_number((int) output_size);
	printf("       ");

	if (input_size == 0) {
		printf("n/a\n\n");
	} else {
		const double ratio = ((double) output_size / (double) input_size) * 100.0;
		printf("%3.0f%%\n\n", ratio);
	}
}

static int write_encoded_file(const char *filename, const struct Encoding_Result *result) {
	printf("Writing encoded file...\n");
	FILE *file = fopen(filename, "wb");

	if (file == NULL) return 1;

	const size_t total = result->header_length + result->data_length;
	init_progress_bar((int64_t) total);

	size_t written = 0;
	// write header (could be single call but count it)
	size_t h = 0;
	while (h < result->header_length) {
		size_t to_write = result->header_length - h;
		if (to_write > CHUNK) to_write = CHUNK;
		size_t w = fwrite(result->header_data + h, 1, to_write, file);
		if (w != to_write) {
			fclose(file);
			return 1;
		}
		h += w;
		written += w;
		update_progress((int64_t) written);
	}

	// write encoded data
	size_t d = 0;
	while (d < result->data_length) {
		size_t to_write = result->data_length - d;
		if (to_write > CHUNK) to_write = CHUNK;
		size_t w = fwrite(result->encoded_data + d, 1, to_write, file);
		if (w != to_write) {
			fclose(file);
			return 1;
		}
		d += w;
		written += w;
		update_progress((int64_t) written);
	}

	finish_progress_bar();

	if (fclose(file) != 0) return 1;

	return 0;
}

static int write_decoded_file(const char *filename, const struct Decoding_Result *result) {
	printf("Writing decoded file...\n");
	FILE *file = fopen(filename, "wb");

	if (file == NULL) return 1;

	init_progress_bar((int64_t) result->data_length);

	size_t written = 0;

	// write encoded data
	size_t d = 0;
	while (d < result->data_length) {
		size_t to_write = result->data_length - d;
		if (to_write > CHUNK) to_write = CHUNK;
		const size_t w = fwrite(result->decoded_data + d, 1, to_write, file);
		if (w != to_write) {
			fclose(file);
			return 1;
		}
		d += w;
		written += w;
		update_progress((int64_t) written);
	}

	finish_progress_bar();

	if (fclose(file) != 0) return 1;

	return 0;
}

static unsigned char *read_file(const char *filename, size_t *file_length) {
	printf("Reading file...\n");
	FILE *file = fopen(filename, "rb");

	if (file == NULL) return NULL;

	if (file_seek(file, 0, SEEK_END) != 0) {
		fclose(file);
		return NULL;
	}

	const long long size = file_tell(file);

	if (size < 0) {
		fclose(file);
		return NULL;
	}

	rewind(file);

	init_progress_bar(size);

	unsigned char *data = malloc((size_t) size);
	if (data == NULL) return NULL;

	size_t bytes_read = 0;
	while (bytes_read < (size_t) size) {
		size_t to_read = (size_t) size - bytes_read;
		if (to_read > CHUNK) to_read = CHUNK;

		const size_t r = fread(data + bytes_read, 1, to_read, file);
		if (r != to_read) {
			free(data);
			fclose(file);
			return NULL;
		}

		bytes_read += r;
		update_progress((int64_t) bytes_read);
	}

	finish_progress_bar();

	fclose(file);

	*file_length = (size_t) size;
	return data;
}


static int encode(const int argc, char *argv[]) {
	size_t file_size;
	const unsigned char *file_content = read_file(argv[2], &file_size);
	if (file_content == NULL) {
		printf("\nError 7: File could not be found or read.\n");
		return 7;
	}

	const struct Encoding_Result result = huffman_encode(file_content, file_size);
	if (result.error_code != 0) {
		printf("\nError %d: Error during encoding.\n", result.error_code);
		return result.error_code;
	}
	free((void *) file_content);

	const size_t encoded_size = result.header_length + result.data_length;
	print_size_summary("Encoding summary:", file_size, encoded_size);

	char *output_file_name;
	int free_output_file_name = 0;
	if (argc > 3) {
		output_file_name = argv[3];
	} else {
		output_file_name = make_encoded_filename(argv[2]);
		free_output_file_name = 1;
	}

	if (output_file_name == NULL) {
		printf("\nError 1: Could not allocate output filename.\n");
		return 1;
	}

	const int write_result = write_encoded_file(output_file_name, &result);
	if (write_result != 0) printf("\nError %d: Error writing to file.\n", write_result);
	if (free_output_file_name) free(output_file_name);
	return write_result;
}


static int decode(const int argc, char *argv[]) {
	size_t file_size;
	const unsigned char *file_content = read_file(argv[2], &file_size);
	if (file_content == NULL) {
		printf("\nError 7: File could not be found or read.\n");
		return 7;
	}

	const struct Decoding_Result result = huffman_decode(file_content, file_size);
	if (result.error_code != 0) {
		printf("\nError %d: Error during decoding.\n", result.error_code);
		return result.error_code;
	}
	free((void *) file_content);

	print_size_summary("Decoding summary:", file_size, result.data_length);

	char *output_file_name;
	int free_output_file_name = 0;
	if (argc > 3) {
		output_file_name = argv[3];
	} else {
		output_file_name = toggle_shuf_extension(argv[2]);
		free_output_file_name = 1;
	}

	if (output_file_name == NULL) {
		printf("\nError 1: Could not allocate output filename.\n");
		return 1;
	}

	const int write_result = write_decoded_file(output_file_name, &result);
	if (write_result != 0) printf("\nError %d: Error writing to file.\n", write_result);
	if (free_output_file_name) free(output_file_name);
	return write_result;
}

int main(const int argc, char *argv[]) {
	setlocale(LC_NUMERIC, "");

	if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
		print_usage();
		return 0;
	}

	if (argc != 3 && argc != 4) {
		printf("\nError 5: Illegal arguments\n");
		print_usage();
		return 5;
	}

	if (strcmp(argv[1], "encode") == 0) return encode(argc, argv);
	if (strcmp(argv[1], "decode") == 0) return decode(argc, argv);
	printf("\nError 6: Illegal action\n");
	print_usage();
	return 6;
}
