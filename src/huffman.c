#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "huffman.h"

struct Node {
	unsigned char character;
	uint64_t count;

	struct Node *left;
	struct Node *right;
};

struct Mapping {
	unsigned char character;
	size_t char_length;
	unsigned char mapped[255];
};

struct BitWriter {
	unsigned char *data;
	size_t capacity;
	size_t byte_index;

	unsigned char current_byte;
	unsigned char bits_used;
};

struct BitReader {
	const unsigned char *data;
	size_t data_size;
	size_t byte_index;
	unsigned char bits_used;
};


/**
 * Counts the amount of occurrences of each byte the data array.
 *
 * @param data A byte array which contains the data to be scanned.
 * @param data_length The length of the data array.
 * @param final_count An empty array with a length of 256 which is to be filled with the final byte counts.
 */
static void count_bytes(const unsigned char *data, const size_t data_length, uint64_t final_count[256]) {
	memset(final_count, 0, 256 * sizeof *final_count);

	for (uint64_t i = 0; i < data_length; ++i) {
		++final_count[data[i]];
	}
}

/**
 * Counts the amount of unique bytes from a valid count_array by filtering all counts that are 0.
 *
 * @param byte_counts The valid count array.
 * @return The count of all unique bytes.
 */
static size_t count_unique_bytes(const uint64_t byte_counts[256]) {
	size_t final_count = 0;
	for (int i = 0; i < 256; ++i) if (byte_counts[i] != 0) ++final_count;
	return final_count;
}

static void convert_to_node_array(const uint64_t byte_counts[256], struct Node nodes[]) {
	int j = 0;
	for (int i = 0; i < 256; ++i) {
		if (byte_counts[i] != 0) {
			struct Node new_node;
			new_node.character = (unsigned char) i;
			new_node.count = byte_counts[i];
			new_node.left = nullptr;
			new_node.right = nullptr;
			nodes[j] = new_node;
			++j;
		}
	}
}

static void swap_nodes(struct Node **a, struct Node **b) {
	struct Node *temp = *a;
	*a = *b;
	*b = temp;
}

static void heap_insert(struct Node *heap[], size_t *heap_size, struct Node *node) {
	size_t index = *heap_size;

	heap[index] = node;
	(*heap_size)++;

	while (index > 0) {
		const size_t parent = (index - 1) / 2;

		if (heap[parent]->count <= heap[index]->count) {
			break;
		}

		swap_nodes(&heap[parent], &heap[index]);

		index = parent;
	}
}

static struct Node *heap_extract_min(struct Node *heap[], size_t *heap_size) {
	if (*heap_size == 0) return nullptr;

	struct Node *minimum = heap[0];

	(*heap_size)--;

	if (*heap_size == 0) return minimum;

	heap[0] = heap[*heap_size];

	size_t index = 0;

	while (1) {
		const size_t left = 2 * index + 1;
		const size_t right = 2 * index + 2;
		size_t smallest = index;

		if (left < *heap_size && heap[left]->count < heap[smallest]->count) smallest = left;

		if (right < *heap_size && heap[right]->count < heap[smallest]->count) smallest = right;

		if (smallest == index) break;

		swap_nodes(&heap[index], &heap[smallest]);

		index = smallest;
	}

	return minimum;
}

static void create_mappings_recursive(const struct Node *node, unsigned char code[], const size_t depth,
                                      struct Mapping mappings[]) {
	if (node->left == NULL && node->right == NULL) {
		mappings[node->character].character = node->character;
		mappings[node->character].char_length = depth;
		memcpy(mappings[node->character].mapped, code, depth);

		return;
	}

	code[depth] = 0;
	create_mappings_recursive(node->left, code, depth + 1, mappings);

	code[depth] = 1;
	create_mappings_recursive(node->right, code, depth + 1, mappings);
}

static struct Node *build_huffman_tree(struct Node nodes[], const size_t nodes_size, struct Node **out_root) {
	if (nodes_size == 0) {
		*out_root = nullptr;
		return nullptr;
	}

	const size_t storage_size = 2 * nodes_size - 1;
	struct Node *node_storage = malloc(sizeof *node_storage * storage_size);
	struct Node **heap = malloc(sizeof *heap * storage_size);
	if (!node_storage || !heap) {
		free(node_storage);
		free(heap);
		*out_root = nullptr;
		return nullptr;
	}

	// Copy leaf nodes into storage
	for (size_t i = 0; i < nodes_size; ++i) {
		node_storage[i] = nodes[i];
		heap[i] = &node_storage[i];
	}

	size_t heap_size = nodes_size;
	size_t next_free_node = nodes_size;

	// Build Huffman tree
	while (heap_size > 1) {
		struct Node *left = heap_extract_min(heap, &heap_size);
		struct Node *right = heap_extract_min(heap, &heap_size);
		struct Node *parent = &node_storage[next_free_node++];

		parent->character = 0;
		parent->count = left->count + right->count;
		parent->left = left;
		parent->right = right;

		heap_insert(heap, &heap_size, parent);
	}

	struct Node *root = heap_extract_min(heap, &heap_size);

	free(heap);

	*out_root = root;
	return node_storage;
}

static void create_mappings(struct Node nodes[], const size_t nodes_size, struct Mapping mappings[]) {
	struct Node *root;

	struct Node *storage = build_huffman_tree(nodes, nodes_size, &root);
	if (root == NULL) return; // nothing to map

	unsigned char code[256];

	create_mappings_recursive(root, code, 0, mappings);

	free(storage);
}

static void bit_writer_init(struct BitWriter *writer, unsigned char *output, const size_t capacity) {
	writer->data = output;
	writer->capacity = capacity;
	writer->byte_index = 0;
	writer->current_byte = 0;
	writer->bits_used = 0;
}

static void bit_writer_write_bit(struct BitWriter *writer, const unsigned char bit) {
	writer->current_byte <<= 1;
	writer->current_byte |= bit & 1;

	writer->bits_used++;

	if (writer->bits_used == 8) {
		writer->data[writer->byte_index++] = writer->current_byte;

		writer->current_byte = 0;
		writer->bits_used = 0;
	}
}

static void bit_writer_flush(struct BitWriter *writer) {
	if (writer->bits_used <= 0) return;

	writer->current_byte <<= 8 - writer->bits_used;

	writer->data[writer->byte_index++] = writer->current_byte;

	writer->current_byte = 0;
	writer->bits_used = 0;
}

static void encode_data(const unsigned char *data, const size_t data_length, struct Mapping mappings[256],
                        struct BitWriter *writer) {
	for (size_t i = 0; i < data_length; ++i) {
		const struct Mapping *mapping = &mappings[data[i]];

		for (size_t j = 0; j < mapping->char_length; ++j) bit_writer_write_bit(writer, mapping->mapped[j]);
	}
}

static void uint64_to_bytes(uint64_t value, unsigned char bytes[8]) {
	for (size_t i = 0; i < 8; ++i) {
		bytes[7 - i] = (unsigned char) (value & 0xFF);
		value >>= 8;
	}
}

static uint64_t bytes_to_uint64(const unsigned char bytes[8]) {
	uint64_t value = 0;

	for (size_t i = 0; i < 8; ++i) {
		value <<= 8;
		value |= bytes[i];
	}

	return value;
}

static size_t calculate_header_size(const uint64_t byte_counts[256]) {
	return 22 + count_unique_bytes(byte_counts) * 9;
}

static void create_header(const uint64_t byte_counts[256], const uint64_t original_data_size,
                          const uint64_t encoded_data_size, unsigned char header_data[]) {
	const size_t unique_bytes = count_unique_bytes(byte_counts);

	// Magic number
	header_data[0] = 's';
	header_data[1] = 'h';
	header_data[2] = 'u';
	header_data[3] = 'f';
	header_data[4] = 1;

	// Size of original data
	unsigned char original_data_size_chars[8];
	uint64_to_bytes(original_data_size, original_data_size_chars);
	for (size_t i = 0; i < 8; ++i) header_data[i + 5] = original_data_size_chars[i];

	// Size of encoded data
	unsigned char encoded_data_size_chars[8];
	uint64_to_bytes(encoded_data_size, encoded_data_size_chars);
	for (size_t i = 0; i < 8; ++i) header_data[i + 13] = encoded_data_size_chars[i];

	// Amount of unique bytes for header length
	header_data[21] = (unsigned char) unique_bytes;

	// Write character counts
	int j = 0;
	for (size_t i = 0; i < 256; ++i) {
		if (byte_counts[i] == 0) continue;

		unsigned char current_byte_count[8];
		uint64_to_bytes(byte_counts[i], current_byte_count);

		header_data[22 + j * 9] = (unsigned char) i;
		for (size_t k = 0; k < 8; ++k) {
			header_data[22 + j * 9 + k + 1] = current_byte_count[k];
		}

		++j;
	}
}

struct Encoding_Result huffman_encode(const unsigned char *data, const size_t data_length) {
	if (data == NULL && data_length != 0) {
		const struct Encoding_Result result = {
			.encoded_data = nullptr,
			.data_length = 0,
			.header_data = nullptr,
			.header_length = 0,
			.error_code = 8
		};

		return result;
	}

	// Count occurrences of each byte.
	uint64_t byte_counts[256];
	count_bytes(data, data_length, byte_counts);

	// Convert each element of the count array into nodes.
	const size_t unique_count = count_unique_bytes(byte_counts);
	struct Node nodes[unique_count];
	convert_to_node_array(byte_counts, nodes);

	// Create mappings out of the Nodes.
	struct Mapping mappings[256] = {0};
	create_mappings(nodes, unique_count, mappings);

	// Calculate size of encoded data to allocate a proper buffer.
	size_t final_bit_size = 0;
	for (size_t i = 0; i < 256; ++i) {
		final_bit_size += mappings[i].char_length * byte_counts[i];
	}
	const size_t final_byte_size = (final_bit_size + 7) / 8;

	unsigned char *output = nullptr;
	if (final_byte_size > 0) output = malloc(final_byte_size);
	struct BitWriter bit_writer;
	bit_writer_init(&bit_writer, output, final_byte_size);
	encode_data(data, data_length, mappings, &bit_writer);
	bit_writer_flush(&bit_writer);
	const size_t header_len = calculate_header_size(byte_counts);
	unsigned char *header_data = malloc(header_len);
	create_header(byte_counts, data_length, final_byte_size, header_data);

	const struct Encoding_Result result = {
		.encoded_data = output,
		.data_length = final_byte_size,
		.header_data = header_data,
		.header_length = header_len,
		.error_code = 0
	};

	return result;
}

static unsigned char bit_reader_read_bit(struct BitReader *reader) {
	const unsigned char byte = reader->data[reader->byte_index];

	const unsigned char bit = (byte >> (7 - reader->bits_used)) & 1;

	++reader->bits_used;

	if (reader->bits_used == 8) {
		reader->bits_used = 0;
		++reader->byte_index;
	}

	return bit;
}

static size_t decode_data(const unsigned char *encoded_data, const size_t encoded_size, const struct Node *root,
                          unsigned char *output, const size_t original_size) {
	struct BitReader reader = {
		.data = encoded_data,
		.data_size = encoded_size,
		.byte_index = 0,
		.bits_used = 0
	};

	const struct Node *current = root;
	size_t output_count = 0;

	while (output_count < original_size) {
		const unsigned char bit = bit_reader_read_bit(&reader);

		if (bit == 0) current = current->left;
		else current = current->right;

		if (current->left == NULL && current->right == NULL) {
			output[output_count] = current->character;
			++output_count;

			current = root;
		}
	}

	return output_count;
}

static void extract_header_data(const unsigned char *encoded_data, unsigned char magic_number[4], uint8_t *version,
                                uint64_t *original_data_size, uint64_t *encoded_data_size, uint64_t byte_counts[256],
                                uint64_t *encoded_data_start) {
	// Initialize byte_counts to zero to avoid uninitialized values
	for (size_t i = 0; i < 256; ++i) byte_counts[i] = 0;
	for (size_t i = 0; i < 4; ++i) magic_number[i] = encoded_data[i];
	*version = (uint8_t) encoded_data[4];
	unsigned char original_data_size_chars[8];
	for (size_t i = 0; i < 8; ++i) original_data_size_chars[i] = encoded_data[i + 5];
	*original_data_size = bytes_to_uint64(original_data_size_chars);
	unsigned char encoded_data_size_chars[8];
	for (size_t i = 0; i < 8; ++i) encoded_data_size_chars[i] = encoded_data[i + 13];
	*encoded_data_size = bytes_to_uint64(encoded_data_size_chars);

	size_t unique_bytes = encoded_data[21];
	if (unique_bytes == 0 && *original_data_size != 0) unique_bytes = 256;
	for (size_t i = 0; i < unique_bytes * 9; i += 9) {
		unsigned char current_byte_count[8];
		for (size_t j = 0; j < 8; ++j) current_byte_count[j] = encoded_data[22 + i + j + 1];
		byte_counts[encoded_data[22 + i]] = bytes_to_uint64(current_byte_count);
	}
	*encoded_data_start = calculate_header_size(byte_counts);
}

struct Decoding_Result huffman_decode(const unsigned char *encoded_data, const size_t encoded_data_length) {
	struct Decoding_Result final_result;

	unsigned char magic_number[4];
	uint8_t version;
	uint64_t original_data_size;
	uint64_t encoded_data_size;
	uint64_t byte_counts[256];
	uint64_t encoded_data_start;

	extract_header_data(encoded_data, magic_number, &version, &original_data_size, &encoded_data_size, byte_counts,
	                    &encoded_data_start);

	// Check header validity
	int error_code = 0;

	constexpr unsigned char check_magic_number[4] = {'s', 'h', 'u', 'f'};
	if (memcmp(magic_number, check_magic_number, 4) != 0) error_code = 1;
	if (version != 1) error_code = 2;
	const size_t header_size = calculate_header_size(byte_counts);
	if (encoded_data_length < header_size || encoded_data_length - header_size != encoded_data_size) error_code = 4;

	if (error_code != 0) {
		final_result.decoded_data = nullptr;
		final_result.data_length = 0;
		final_result.error_code = error_code;
		return final_result;
	}

	const size_t unique_count = count_unique_bytes(byte_counts);
	struct Node nodes[unique_count];
	convert_to_node_array(byte_counts, nodes);

	struct Node *root;
	struct Node *storage = build_huffman_tree(nodes, unique_count, &root);

	unsigned char *output = malloc(original_data_size);

	if (root != NULL) {
		decode_data(encoded_data + encoded_data_start, encoded_data_size, root, output, original_data_size);
	}

	// free storage allocated by build_huffman_tree
	free(storage);

	final_result.decoded_data = output;
	final_result.data_length = original_data_size;
	final_result.error_code = 0;

	return final_result;
}
