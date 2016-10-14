#include <cstdio>
#include <cstdlib>

#include <sys/types.h>
#include <sys/stat.h> 
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>


using namespace std;

int main(int argc, char *argv[]) {
  if (argc != 2) printf("Usage: %s <pid>\n", argv[0]), exit(1);

  pid_t pid = atoi(argv[1]);
  char file[64];

  sprintf(file, "/proc/%ld/mem", (long) pid);
  int fd = open(file, O_RDWR);

  if (ptrace(PTRACE_ATTACH, pid, 0, 0) == -1) perror("ptrace attach failed: "), exit(1);
  if (waitpid(pid, NULL, 0) == -1) perror("ptrace failed: "), exit(1);

  off_t addr = 0;
  int value;
  if (pread(fd, &value, sizeof(value), addr) == -1) perror("pread failed: "), exit(1);
  printf("I read: %d\n", value);
  // or
  // pwrite(fd, &value, sizeof(value), addr);
  if (ptrace(PTRACE_DETACH, pid, 0, 0) == -1) perror("ptrace detach failed: "), exit(1);

  return 0;
}
