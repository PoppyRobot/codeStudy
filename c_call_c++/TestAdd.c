//#include "t.h"
#include <stdio.h>

extern int Add( int x, int y );

int main(int argc, char *argv[])
{
	int a = 10;
	int b = 6;
	printf("Max= %d\n", ((a + b) + abs(a - b)) / 2);

	printf("Add(1, 2)= %d\n", Add(1, 2));
}
