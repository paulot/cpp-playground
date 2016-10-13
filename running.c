#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
	while (1) {
		sleep(3);
		printf("Running\n");
	}
	return 0;
}
