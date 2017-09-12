/*
 ============================================================================
 Name        : codecTest.c
 Author      : Gray
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "codec27.h"

/* metric table */
int32_t mettab[2][256];

int main(void) {
	puts("!!!This is Convolution coder-decoder test!!!"); /* prints !!!Hello World!!! */
	/*
	 * VITERBI CODEC TESTING SECTION START
	 * */

	/* test data */
#define  NBYTES 32
#define NBITS NBYTES*8
#define ENCBYTES (NBYTES+1)*2

	uint8_t data[NBYTES] = { 0 };

	uint8_t decoded[NBYTES] = { 0 };

	uint8_t encoded[ENCBYTES] = { 0 };

	data[0] = 0x01;
	data[1] = 0x23;
	data[2] = 0x45;
	data[3] = 0x67;
	data[4] = 0x89;

	data[5] = 'T';
	data[6] = 'h';
	data[7] = 'i';
	data[8] = 's';
	data[9] = ' ';
	data[10] = 't';
	data[11] = 'e';
	data[12] = 'x';
	data[13] = 't';
	data[14] = ' ';
	data[15] = 's';
	data[16] = 'h';
	data[17] = 'o';
	data[18] = 'u';
	data[19] = 'l';
	data[20] = 'd';
	data[21] = ' ';
	data[22] = 'b';
	data[23] = 'e';
	data[24] = ' ';
	data[25] = 'e';
	data[26] = 'n';
	data[27] = 'c';
	data[28] = 'o';
	data[29] = 'd';
	data[30] = 'e';
	data[31] = 'd';

	/*	 data[NBYTES - 4] = 0x55;
	 data[NBYTES - 3] = 0xaa;
	 data[NBYTES - 2] = 0xff;
	 data[NBYTES - 1] = 0xff;*/

	/* encode */
	int res = encode27(encoded, data, NBYTES);

	/* prepare metrics table for rated SNR */
	int amplitude = 100; //signal amplitude
	double esn0, ebn0 = 5.0, noise;
	esn0 = ebn0 - 10 * log10(2.0); /* Es/N0 in dB for Rate=2 */
	/* Compute noise voltage. The 0.5 factor accounts for BPSK seeing
	 * only half the noise power, and the sqrt() converts power to
	 * voltage.
	 */
	noise = sqrt(0.5 / pow(10., esn0 / 10.));
	res = gen_met(mettab, amplitude, noise, 0., 4);

	/* decode */
	long metrics;
	res = viterbi27(&metrics, decoded, encoded, mettab, NBITS, 0, 0);

	/* Test results */
	int result = 0;

	for (int i = 0; i < NBYTES; i++) {
		if (data[i] != decoded[i]) {
			result = 1;
		}
	}

	/* view decoded result*/
	if (result) {

		/* view result if not same */
		puts("=== Test failed! Data not same ===\n");
		/* view original data */
		printf("original data:\n");
		for (int i = 0; i < NBYTES; i++)
			printf("%02x", data[i]);
		printf("\n\n");

		/* view encoded data */
		printf("Encoded data:\n");
		for (int i = 0; i < ENCBYTES; i++) {
			unsigned char sym = encoded[i];
			for (int j = 0; j < 8; j++) {
				printf("%01x", (sym & 0x80) ? 1 : 0);
				sym <<= 1;
			}
			printf(" ");
		}
		printf("\n\n");
		/*  for (int i = 0; i < ENCBYTES; i++)
		 printf ("%c", encoded[i]);
		 printf ("\n\n");*/

		printf("Decoded data:\n");
		for (int i = 0; i < NBYTES; i++)
			printf("%02x", decoded[i]);
		printf("\n\n");
		puts("Test ended\n");
	} else {
		puts("Test OK!");
	}
	/*
	 * VITERBI CODEC TESTING SECTION END
	 * */

	return EXIT_SUCCESS;
}
