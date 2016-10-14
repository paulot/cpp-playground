#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

int main(int ac, char **av) {
    char    name[64];
    int             fd;
    if (ac != 4) {
        fprintf(stderr, "usage: %s pid address value\n", av[1]);
        exit(1); }
    // sprintf(name, "/proc/%.10s/mem", av[1]);
    sprintf(name, "bla");
    if ((fd = open(name, O_WRONLY)) < 0) {
        fprintf(stderr, "Can't access pid %s", av[1]);
        perror(":");
        exit(1); }
    lseek(fd, strtol(av[2], 0, 0), SEEK_SET);
    printf("%d %s %d\n", fd, av[3], strlen(av[3]));
    if (write(fd, av[3], strlen(av[3])) < 0)
        perror("write");
    return 0;
}
