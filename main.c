
/*

   A5/1 Stream-Cipher
   
   Date: April 14, 2018

   Test with GDB online compiler tool for C: https://www.onlinegdb.com

*/

#include <stdio.h>

/* Optional for USER KEYBOARD INPUT: uncomment the lib */

/* #include "utils.h" */

/* 3 Shift registers */
#define R1REG	0x07FFFF /* 19 bits, numbered 0..18 */
#define R2REG	0x3FFFFF /* 22 bits, numbered 0..21 */
#define R3REG	0x7FFFFF /* 23 bits, numbered 0..22 */

/* clock control */
#define R1MID	0x000100 /* bit 8 */
#define R2MID	0x000400 /* bit 10 */
#define R3MID	0x000400 /* bit 10 */

/* shift registers taps.*/
#define R1TAPS	0x072000 /* bits 18,17,16,13 */
#define R2TAPS	0x300000 /* bits 21,20 */
#define R3TAPS	0x700080 /* bits 22,21,20,7 */

/* Output taps, for output generation */
#define R1OUT	0x040000 /* bit 18 (the high bit) */
#define R2OUT	0x200000 /* bit 21 (the high bit) */
#define R3OUT	0x400000 /* bit 22 (the high bit) */

typedef unsigned char byte;
typedef unsigned long word;
typedef word bit;

/* Calculate the sum of word bits modulo 2 */
bit parity(word x) {
	x ^= x>>16;
	x ^= x>>8;
	x ^= x>>4;
	x ^= x>>2;
	x ^= x>>1;
	return x&1;
}

/* Clock one shift register */
word clockone(word reg, word mask, word taps) {
	word t = reg & taps;
	reg = (reg << 1) & mask;
	reg |= parity(t);
	return reg;
}

/* The three shift registers: global vars */
word R1, R2, R3;

/* return the majority R1,R2,R3 */
bit majority() {
	int sum;
	sum = parity(R1&R1MID) + parity(R2&R2MID) + parity(R3&R3MID);
	if (sum >= 2)
		return 1;
	else
		return 0;
}

/* Shift R1,R2,R3 per the majority value*/
void clock() {
	bit maj = majority();
	if (((R1&R1MID)!=0) == maj)
		R1 = clockone(R1, R1REG, R1TAPS);
	if (((R2&R2MID)!=0) == maj)
		R2 = clockone(R2, R2REG, R2TAPS);
	if (((R3&R3MID)!=0) == maj)
		R3 = clockone(R3, R3REG, R3TAPS);
}

/* Clock all R1,R2,R3, used for key setup. */
void clockallthree() {
	R1 = clockone(R1, R1REG, R1TAPS);
	R2 = clockone(R2, R2REG, R2TAPS);
	R3 = clockone(R3, R3REG, R3TAPS);
}

/* Get a bit from registers R1,R2,R3 then XOR bits. */
bit getbit() {
	return parity(R1&R1OUT)^parity(R2&R2OUT)^parity(R3&R3OUT);
}

/* Do the A5/1 key setup: params a 64-bit key and
 * a 22-bit frame number. */
void keysetup(byte key[8], word frame) {
	int i;
	bit keybit, framebit;

	/* Zero out the shift registers. */
	R1 = R2 = R3 = 0;

	/* Load the key into the shift registers */
	for (i=0; i<64; i++) {
		clockallthree(); /* always clock */
		keybit = (key[i/8] >> (i&7)) & 1; 
		R1 ^= keybit; R2 ^= keybit; R3 ^= keybit;
	}

	/* Load the frame number into the shift
	 * registers */
	for (i=0; i<22; i++) {
		clockallthree(); /* always clock */
		framebit = (frame >> i) & 1; 

		R1 ^= framebit; R2 ^= framebit; R3 ^= framebit;
	}

	/* Run the shift registers for 100 clocks
	 * to mix the key and frame number
	 * together */
	for (i=0; i<100; i++) {
		clock();
	}
	/* the key is set up. */
}
	
/* Generate 228 bits of keystream output */
void run(byte Ekeystream[], byte Dkeystream[]) {
	int i;

	/* Zero out the output buffers. */
	for (i=0; i<=113/8; i++)
		Ekeystream[i] = Dkeystream[i] = 0;
	
	/* Generate 114 bits of keystream for the
	 * ENCRYPTED direction.  */
	for (i=0; i<114; i++) {
		clock();
		Ekeystream[i/8] |= getbit() << (7-(i&7));
	}

	/* Generate 114 bits of keystream for the
	 * DECRYPTED direction.  */
	for (i=0; i<114; i++) {
		clock();
		Dkeystream[i/8] |= getbit() << (7-(i&7));
	}
}

/* Test the code by comparing it against
 * an expected vector */
void testA51() {
	byte key[8] = {0x12, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
	word frame = 0x134;
	byte ExpectedEncrypt[15] = { 0x53, 0x4E, 0xAA, 0x58, 0x2F, 0xE8, 0x15,
	                      0x1A, 0xB6, 0xE1, 0x85, 0x5A, 0x72, 0x8C, 0x00 };
	byte ExpectedDecrypt[15] = { 0x24, 0xFD, 0x35, 0xA3, 0x5D, 0x5F, 0xB6,
	                      0x52, 0x6D, 0x32, 0xF9, 0x06, 0xDF, 0x1A, 0xC0 };
	byte EtoD[15], DtoE[15];
	int i, failed=0;

	/* GENERATE THE A5/1 ENCRYPTED and DECRYPTED STREAMS */
	
	keysetup(key, frame);
	
	run(EtoD, DtoE);

	/* Compare against the expected */
	for (i=0; i<15; i++)
		if (EtoD[i] != ExpectedEncrypt[i])
			failed = 1;
	for (i=0; i<15; i++)
		if (DtoE[i] != ExpectedDecrypt[i])
			failed = 1;

	/* Print output. */
	printf("key: 0x");
	for (i=0; i<8; i++)
		printf("%02X", key[i]);
	printf("\n");
	printf("frame number: 0x%06X\n", (unsigned int)frame);
	printf("\nEXPECTED ENCRYPTED STREAM:\n");
	printf("     0x");
	for (i=0; i<15; i++)
		printf("%02X", ExpectedEncrypt[i]);
	printf("\nEXPECTED DECRYPTED STREAM:\n");
	printf("     0x");
	for (i=0; i<15; i++)
		printf("%02X", ExpectedDecrypt[i]);
	printf("\n");
	printf("\nACTUAL ENCRYPTED STREAM:\n");
	printf("     0x");
	for (i=0; i<15; i++)
		printf("%02X", EtoD[i]);
	printf("\nACTUAL DECRYPTED STREAM:\n");
	printf("     0x");
	for (i=0; i<15; i++)
		printf("%02X", DtoE[i]);
	printf("\n");
	
	if (!failed) {
		printf("\nSUCCESS: test passed.\n");
		return;
	} else {
		/* Problems!  */
		printf("\nCipher needs a check.\n");
		exit(1);
	}
}

int main(void) {
	testA51();
	return 0;
}

