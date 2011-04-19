#include <avr/io.h>
#include <avr/pgmspace.h> 

#include "../draw.h"

/*
 * The font's code page to map chars to glyphs.  The index of this array is the value of 
 * each char in the string; the value of the array will point to an entry in the font table.
 * 0xFF has the special meaning of "Not implemented; leave blank".
 */
prog_uchar codepage_tiny[] PROGMEM = {
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, //0x00-0x0F
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF, //0x10-0x1F
	0xFF,0x24,0xFF,0xFF,0xFF,0x2A,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x26,0xFF,0x25,0x28, //0x20-0x2F
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0xFF,0xFF,0xFF,0xFF,0xFF,0x27, //0x30-0x3F
	0xFF,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18, //0x40-0x4F
	0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23,0xFF,0x29,0xFF,0xFF,0xFF, //0x50-0x5F
	0xFF,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18, //0x60-0x6F
	0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23,0xFF,0xFF,0xFF,0xFF,0xFF, //0x70-0x7F
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF  //0x80-0x8F
};

/*
 * A 5x3 pixel font.  The bits are read across from top left to top right, then down
 * to the next line.  Each character is represented by a 16bit unsigned integer, with 
 * bit 15 ignored.
 */
prog_uint16_t font_tiny[] PROGMEM = {
	0x2b, 0x6a,		//0, 0x0
	0x2c, 0x97,		//1
	0x73, 0xe7,		//2
	0x72, 0xcf,		//3
	0x5b, 0xc9,		//4
	0x79, 0xcf,		//5
	0x79, 0xef,		//6
	0x72, 0x49,		//7
	0x7b, 0xef,		//8
	0x7b, 0xcf,		//9, 0x9
	0x2b, 0xed,		//A, 0xA
	0x6b, 0xae,		//B, 0xB
	0x79, 0x27,		//C
	0x6b, 0x6e,		//D
	0x79, 0xe7,		//E
	0x79, 0xe4,		//F
	0x79, 0x6f,		//G, 0x10
	0x5b, 0xed,		//H, 0x11
	0x74, 0x97,		//I
	0x12, 0x6f,		//J
	0x5b, 0xad,		//K
	0x49, 0x27,		//L
	0x7f, 0x6d,		//M
	0x7b, 0x6d,		//N
	0x7b, 0x6f,		//O
	0x7b, 0xe4,		//P
	0x2b, 0x7b,		//Q
	0x7b, 0xad,		//R
	0x79, 0xcf,		//S
	0x74, 0x92,		//T
	0x5b, 0x6f,		//U
	0x5b, 0x6a,		//V
	0x5b, 0xfa,		//W, 0x20
	0x5a, 0xad,		//X
	0x5b, 0xd2,		//Y
	0x72, 0xa7,		//Z, 0x23
	0x24, 0x82,		//!, 0x24
	0x00, 0x02,		//.
	0x00, 0x12,		//,
	0x72, 0x82,		//?
	0x12, 0xa4,		///
	0x48, 0x89,		//\, 0x29
	0x42, 0xa1		//%, 0x2A
	
};
