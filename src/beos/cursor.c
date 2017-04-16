#include <stdio.h>

char line[80];

unsigned char data[16];
unsigned char mask[16];

#define BLACK '*'
#define WHITE '.'
#define BG ' '

void convert(int index, unsigned char *data, unsigned char *mask) {
int i;
	*data = 0; *mask = 0;
	for (i = 0; i < 8; i++, index++) {
		*data <<= 1; *mask <<= 1;
		if (line[index] == BLACK) *data |= 1;
		if (line[index] != BG) *mask |= 1;
	}
}

int main(int argc, char *argv[]) {
int row = 1;
int byte = 0;
	if (NULL!=gets(line)) {
		printf("unsigned char cursor[] =\n"
				"{16, // width & hight\n"
	            "1, // color depth\n"
	            "8, 8, // hotspot coordinates\n"
	            "// color data (32 bytes)\n");
		while ((row <= 16) && (NULL!=gets(line))) {
			convert(1, &data[byte], &mask[byte]); byte++;
			convert(9, &data[byte], &mask[byte]); byte++;
			row++;
		}
		for (byte = 0, row = 1; row <= 16; row++) {
			printf("0x%2.2X, ", (int)data[byte]); byte++;
			printf("0x%2.2X, ", (int)data[byte]); byte++;
			//printf(" // %2.2d\n", row);
			if ((row % 4) == 0) printf("\n"); 
		}
		printf("// transparency bitmask (32 bytes)\n");
		for (byte = 0, row = 1; row <= 16; row++) {
			printf("0x%2.2X, ", (int)mask[byte]); byte++;
			printf("0x%2.2X", (int)mask[byte]); byte++;
			if (row != 16) printf(","); else printf(" ");
			//printf(" // %2.2d\n", row);
			if ((row % 4) == 0) printf("\n"); 
		}
		printf("};\n");
	}	
}
