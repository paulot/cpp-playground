#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int a = 1337;

int main() {
	while (1) {
		sleep(3);
		printf("Running: %d\n", a);
	}
	return 0;
}
