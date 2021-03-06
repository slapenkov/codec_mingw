/*
 * codec27.c
 *
 *  Created on: 2 ����. 2017 �.
 *      Author: sergey.lapenkov
 */

#include "codec27.h"

/* Normal function integrated from -Inf to x. Range: 0-1 */
//#define	normal(x)	(0.5 + 0.5*erf((x)/M_SQRT2))
float normal(float x) {
	if (x > 0) {
		return (0.5 + 0.5 * erf((x) / M_SQRT2));
	} else {
		return (1 - (0.5 + 0.5 * erf((-x) / M_SQRT2)));
	}
}

/* Logarithm base 2 */
#define	log2(x)	(log(x)*M_LOG2E)

/* 8-bit parity lookup table, generated by partab.c */
uint8_t Partab[] = { 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1,
		0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0,
		1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1,
		0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1,
		0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1,
		0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0,
		1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0,
		1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0,
		1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1,
		0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0,
		1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0, };

/* Convolutionally encode data into binary symbols */
int encode27(unsigned char *symbols, unsigned char *data, unsigned int nbytes) {
	int i;
	int encstate = 0;
	int bitcount = 0;
	uint16_t res = 0;

	for (uint16_t c = 0; c < nbytes; c++) {
		res = 0;
		for (i = 7; i >= 0; i--) {
			encstate = (encstate << 1) | ((*data >> i) & 1);
			res = res << 1;
			res += Partab[encstate & POLYA];
			res = res << 1;
			res += Partab[encstate & POLYB];
		}
		data++;
		*symbols++ = (uint8_t) (res >> 8);
		*symbols++ = (uint8_t) (res & 0x00ff);
	}
	/* Flush out tail */
	res = 0;
	for (i = 5; i >= 0; i--) {
		encstate = (encstate << 1);
		res = res << 1;
		res += Partab[encstate & POLYA];
		res = res << 1;
		res += Partab[encstate & POLYB];
	}
	res <<= 4;
	*symbols++ = (uint8_t) (res >> 8);
	*symbols++ = (uint8_t) (res & 0x00ff);
	return 0;
}

/* Generate log-likelihood metrics for 8-bit soft quantized channel
 * assuming AWGN and BPSK
 */
int gen_met(int mettab[2][256], /* Metric table, [sent sym][rx symbol] */
int amp, /* Signal amplitude, units */
double noise, /* Relative noise voltage */
double bias, /* Metric bias; 0 for viterbi, rate for sequential */
int scale /* Scale factor */
) {
	int s, bit;
	float metrics[2][256];
	float p0, p1;

	/* Zero is a special value, since this sample includes all
	 * lower samples that were clipped to this value, i.e., it
	 * takes the whole lower tail of the curve
	 */
	float argum = 0.0 - OFFSET + 0.5;
	argum = argum / amp - 1;
	argum = argum / noise;
	p1 = normal(argum); /* P(s|1) */

	/* Prob of this value occurring for a 0-bit *//* P(s|0) */
	p0 = normal(((0 - OFFSET + 0.5) / amp + 1) / noise);
	metrics[0][0] = log2(2*p0/(p1+p0)) - bias;
	metrics[1][0] = log2(2*p1/(p1+p0)) - bias;

	printf("test p1 %f p0 %f \n", p1, p0);

	for (s = 1; s < 255; s++) {
		/* P(s|1), prob of receiving s given 1 transmitted */
		p1 = normal(((s - OFFSET + 0.5) / amp - 1) / noise)
				- normal(((s - OFFSET - 0.5) / amp - 1) / noise);

		/* P(s|0), prob of receiving s given 0 transmitted */
		p0 = normal(((s - OFFSET + 0.5) / amp + 1) / noise)
				- normal(((s - OFFSET - 0.5) / amp + 1) / noise);

#ifdef notdef
		printf("P(%d|1) = %lg, P(%d|0) = %lg\n",s,p1,s,p0);
#endif
		metrics[0][s] = log2(2*p0/(p1+p0)) - bias;
		metrics[1][s] = log2(2*p1/(p1+p0)) - bias;
	}
	/* 255 is also a special value */
	/* P(s|1) */
	p1 = 1 - normal(((255 - OFFSET - 0.5) / amp - 1) / noise);
	/* P(s|0) */
	p0 = 1 - normal(((255 - OFFSET - 0.5) / amp + 1) / noise);

	metrics[0][255] = log2(2*p0/(p1+p0)) - bias;
	metrics[1][255] = log2(2*p1/(p1+p0)) - bias;
#ifdef	notdef
	/* The probability of a raw symbol error is the probability
	 * that a 1-bit would be received as a sample with value
	 * 0-128. This is the offset normal curve integrated from -Inf to 0.
	 */
	printf("symbol Pe = %lg\n",normal(-1/noise));
#endif
	for (bit = 0; bit < 2; bit++) {
		for (s = 0; s < 256; s++) {
			/* Scale and round to nearest integer */
			mettab[bit][s] = (int) floor(metrics[bit][s] * scale + 0.5);
#ifdef	notdef
			printf("metrics[%d][%d] = %lg, mettab = %d\n",
					bit,s,metrics[bit][s],mettab[bit][s]);
#endif
		}
	}
	return 0;
}

/* Viterbi decoder */
int viterbi27(long *metric, /* Final path metric (returned value) */
unsigned char *data, /* Decoded output data */
unsigned char *symbols, /* Raw deinterleaved input symbols */
int mettab[2][256], /* Metric table, [sent sym][rx symbol] */
int nbits, /* Number of output bits; 2*(nbits+6) input symbols will be read */
unsigned int startstate, /* Encoder starting state */
unsigned int endstate /* Encoder ending state */
) {
	int bitcnt, i, mets[4];
	//nbits = 512;
	bitcnt = -6; /* -(K-1) */
	int32_t dec, paths[(nbits + 6) * 2], *pp;
	int32_t cmetric[64], nmetric[64];
	uint8_t exit = 0;

	memset(paths, 0, (nbits + 6) * 2);

	startstate &= ~((1 << (7 - 1)) - 1);
	endstate &= ~((1 << (7 - 1)) - 1);

	/* Initialize starting metrics */
	for (i = 0; i < 64; i++)
		cmetric[i] = -999999;
	cmetric[startstate] = 0;

	pp = paths;
	for (;;) {
		unsigned char curSym = *symbols;
		for (int count = 0; count < 2; count++) {
			int sym0 = (curSym & 0x80) ? 1 : 0;
			int sym1 = (curSym & 0x40) ? 1 : 0;
			//printf ("%i%i", sym0, sym1);
			/* Read input symbol pair and compute branch metrics */
			mets[0] = mettab[0][sym0 * 255] + mettab[0][sym1 * 255];
			mets[1] = mettab[0][sym0 * 255] + mettab[1][sym1 * 255];
			mets[3] = mettab[1][sym0 * 255] + mettab[1][sym1 * 255];
			mets[2] = mettab[1][sym0 * 255] + mettab[0][sym1 * 255];
			curSym <<= 2;

			/* These macro calls were generated by genbut.c
			 * and rearranged by hand for speed
			 */
			dec = 0;
			BUTTERFLY(0, 0);
			BUTTERFLY(6, 0);
			BUTTERFLY(8, 0);
			BUTTERFLY(14, 0);
			BUTTERFLY(2, 3);
			BUTTERFLY(4, 3);
			BUTTERFLY(10, 3);
			BUTTERFLY(12, 3);
			BUTTERFLY(1, 1);
			BUTTERFLY(7, 1);
			BUTTERFLY(9, 1);
			BUTTERFLY(15, 1);
			BUTTERFLY(3, 2);
			BUTTERFLY(5, 2);
			BUTTERFLY(11, 2);
			BUTTERFLY(13, 2);
			*pp++ = dec;
			dec = 0;
			BUTTERFLY(19, 0);
			BUTTERFLY(21, 0);
			BUTTERFLY(27, 0);
			BUTTERFLY(29, 0);
			BUTTERFLY(17, 3);
			BUTTERFLY(23, 3);
			BUTTERFLY(25, 3);
			BUTTERFLY(31, 3);
			BUTTERFLY(18, 1);
			BUTTERFLY(20, 1);
			BUTTERFLY(26, 1);
			BUTTERFLY(28, 1);
			BUTTERFLY(16, 2);
			BUTTERFLY(22, 2);
			BUTTERFLY(24, 2);
			BUTTERFLY(30, 2);
			*pp++ = dec;

			if (++bitcnt == nbits) {
				*metric = nmetric[endstate];
				exit = 1;
				break;
			}

			/* Read input symbol pair and compute branch metrics */
			sym0 = (curSym & 0x80) ? 1 : 0;
			sym1 = (curSym & 0x40) ? 1 : 0;
			//printf ("%i%i\n", sym0, sym1);
			/* Read input symbol pair and compute branch metrics */
			mets[0] = mettab[0][sym0 * 255] + mettab[0][sym1 * 255];
			mets[1] = mettab[0][sym0 * 255] + mettab[1][sym1 * 255];
			mets[3] = mettab[1][sym0 * 255] + mettab[1][sym1 * 255];
			mets[2] = mettab[1][sym0 * 255] + mettab[0][sym1 * 255];
			curSym <<= 2;

			/* These macro calls were generated by genbut.c
			 * and rearranged by hand for speed
			 */
			dec = 0;
			BUTTERFLY2(0, 0);
			BUTTERFLY2(6, 0);
			BUTTERFLY2(8, 0);
			BUTTERFLY2(14, 0);
			BUTTERFLY2(2, 3);
			BUTTERFLY2(4, 3);
			BUTTERFLY2(10, 3);
			BUTTERFLY2(12, 3);
			BUTTERFLY2(1, 1);
			BUTTERFLY2(7, 1);
			BUTTERFLY2(9, 1);
			BUTTERFLY2(15, 1);
			BUTTERFLY2(3, 2);
			BUTTERFLY2(5, 2);
			BUTTERFLY2(11, 2);
			BUTTERFLY2(13, 2);
			*pp++ = dec;
			dec = 0;
			BUTTERFLY2(19, 0);
			BUTTERFLY2(21, 0);
			BUTTERFLY2(27, 0);
			BUTTERFLY2(29, 0);
			BUTTERFLY2(17, 3);
			BUTTERFLY2(23, 3);
			BUTTERFLY2(25, 3);
			BUTTERFLY2(31, 3);
			BUTTERFLY2(18, 1);
			BUTTERFLY2(20, 1);
			BUTTERFLY2(26, 1);
			BUTTERFLY2(28, 1);
			BUTTERFLY2(16, 2);
			BUTTERFLY2(22, 2);
			BUTTERFLY2(24, 2);
			BUTTERFLY2(30, 2);
			*pp++ = dec;

			if (++bitcnt == nbits) {
				*metric = cmetric[endstate];
				exit = 1;
				break;
			}
		}
		if (exit) {
			break;
		}
		symbols++; //next byte (4 symbols)
	}
	/* Chain back from terminal state to produce decoded data */
	if (data == NULL)
		return 0;/* Discard output */
	memset(data, 0, (nbits + 7) / 8); /* round up in case nbits % 8 != 0 */

	for (i = nbits - 1; i >= 0; i--) {
		pp -= 2;
		if (pp[endstate >> 5] & (1 << (endstate & 31))) {
			endstate |= 64; /* 2^(K-1) */
			data[i >> 3] |= 0x80 >> (i & 7);
		}
		endstate >>= 1;
	}
	return 0;
}
