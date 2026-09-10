#ifndef SHUFF_ALGO_H
#define SHUFF_ALGO_H

struct Encoding_Result {
	unsigned char *encoded_data;
	size_t data_length;

	unsigned char *header_data;
	size_t header_length;

	int error_code;
};

/** This struct returns a pointer to the decoded data with its size.
 *	It also has the capability of adding an error code.
 *
 *	Error codes:
 *
 *	0 - No error, decoding was successful.
 *
 *	1 - Magic number wrong, probably not valid encoded data.
 *
 *	2 - Version number mismatch. Currently, no different versions exist, so this indicates corruption.
 *
 *	3 - Decoded data size mismatch.
 *
 *	4 - Encoded data size mismatch.
 */
struct Decoding_Result {
	unsigned char *decoded_data;
	size_t data_length;

	int error_code;
};

struct Encoding_Result huffman_encode(const unsigned char *data, size_t data_length);

struct Decoding_Result huffman_decode(const unsigned char *encoded_data, size_t encoded_data_length);

#endif //SHUFF_ALGO_H
