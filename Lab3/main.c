#include "printf.h"
#include "pmap.h"

int main() {
	printf("main.c:\tmain is start ...\n");

	mips_init();
	panic("main is over is error!");

	return 0;
}
