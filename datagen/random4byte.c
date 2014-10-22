#include <stdio.h>
#include <stdlib.h>
#include <time.h>

main() {
	int datacount = 1024;
	FILE* outfile = fopen( "random.dat", "w" );
	
	unsigned int buffer[8192/sizeof(unsigned int)];
	int pageWords = 8192/sizeof(unsigned int);

	srand(time(0));

	for ( int i = 0; i < datacount; i++ ) {
		for ( int j = 0; j < pageWords; j++ ) {
			unsigned int dat = rand();
			buffer[j] = dat;
		}

		fwrite(buffer, sizeof(unsigned int), pageWords, outfile);
	}
	fclose(outfile);
}
