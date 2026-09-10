#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "huffman.h"

static void print_usage() {
	printf("\nUsage:\n\nshuff [<action>] [<input>] [<output (optional)>]\n\naction: Either 'encode' or 'decode'");
}

static char *make_encoded_filename(const char *input_filename) {
	const size_t length = strlen(input_filename);

	constexpr char extension[] = ".shuf";
	constexpr size_t extension_length = sizeof(extension) - 1;

	char *output = malloc(length + extension_length + 1);

	if (output == NULL) {
		return nullptr;
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

		if (result == NULL)return nullptr;

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

	if (result == NULL)return nullptr;

	memcpy(result, filename, base_len);
	memcpy(result + base_len, "(1)", 3);
	memcpy(result + base_len + 3, filename + base_len, extension_len);

	result[base_len + 3 + extension_len] = '\0';

	return result;
}

static int write_encoded_file(const char *filename, const struct Encoding_Result *result) {
	FILE *file = fopen(filename, "wb");

	if (file == NULL) return 1;

	if (fwrite(result->header_data, 1, result->header_length, file) != result->header_length) {
		fclose(file);
		return 1;
	}

	if (fwrite(result->encoded_data, 1, result->data_length, file) != result->data_length) {
		fclose(file);
		return 1;
	}

	if (fclose(file) != 0) return 1;

	return 0;
}

static int write_decoded_file(const char *filename, const struct Decoding_Result *result) {
	FILE *file = fopen(filename, "wb");

	if (file == NULL) return 1;

	if (fwrite(result->decoded_data, 1, result->data_length, file) != result->data_length) {
		fclose(file);
		return 1;
	}

	if (fclose(file) != 0) return 1;

	return 0;
}

static unsigned char *read_file(const char *filename, size_t *file_length) {
	FILE *file = fopen(filename, "rb");

	if (file == NULL) return nullptr;

	if (fseek(file, 0, SEEK_END) != 0) {
		fclose(file);
		return nullptr;
	}

	const long size = ftell(file);

	if (size < 0) {
		fclose(file);
		return nullptr;
	}

	rewind(file);

	unsigned char *data = malloc((size_t) size);

	if (data == NULL && size != 0) {
		fclose(file);
		return nullptr;
	}

	if (size > 0) {
		if (fread(data, 1, (size_t) size, file) != (size_t) size) {
			free(data);
			fclose(file);
			return nullptr;
		}
	}

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

	char *output_file_name;
	if (argc > 3) {
		output_file_name = argv[3];
	} else {
		output_file_name = make_encoded_filename(argv[2]);
	}

	const int write_result = write_encoded_file(output_file_name, &result);
	if (write_result != 0) printf("\nError %d: Error writing to file.", write_result);
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

	char *output_file_name;
	if (argc > 3) {
		output_file_name = argv[3];
	} else {
		output_file_name = toggle_shuf_extension(argv[2]);
	}

	const int write_result = write_decoded_file(output_file_name, &result);
	if (write_result != 0) printf("\nError %d: Error writing to file.", write_result);
	return write_result;
}

int main(const int argc, char *argv[]) {
	if (argc != 3 && argc != 4) {
		printf("Error 5: Illegal arguments");
		print_usage();
		return 5;
	}

	if (strcmp(argv[1], "encode") == 0) return encode(argc, argv);
	if (strcmp(argv[1], "decode") == 0) return decode(argc, argv);
	printf("Error 6: Illegal action");
	print_usage();
	return 6;
}
