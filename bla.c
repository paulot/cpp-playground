#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/user.h>

const int long_size = sizeof(long);
void getdata(pid_t child, long addr, char *str, int len) {
	char *laddr;
    int i, j;
    union u {
		long val;
        char chars[long_size];
    } data;
    laddr = str;
    for (i = 0; i < len / long_size; i++) {
        data.val = ptrace(PTRACE_PEEKDATA, child, addr + i * 4, NULL);
        memcpy(laddr, data.chars, long_size);
        laddr += long_size;
    }

    j = len % long_size;
    if(j != 0) {
        data.val = ptrace(PTRACE_PEEKDATA, child, addr + i * 4, NULL);
        memcpy(laddr, data.chars, j);
    }
    str[len] = '\0';
}

void putdata(pid_t child, long addr, char *str, int len) {
	char *laddr;
    int i, j;
    union u {
    	long val;
        char chars[long_size];
    } data;

    laddr = str;

    for (i = 0; i < len / long_size; i++) {
        memcpy(data.chars, laddr, long_size);
        ptrace(PTRACE_POKEDATA, child, addr + i * 4, data.val);
        laddr += long_size;
    }

    j = len % long_size;
    if(j != 0) {
        memcpy(data.chars, laddr, j);
        ptrace(PTRACE_POKEDATA, child, addr + i * 4, data.val);
    }
}


int main(int argc, char *argv[]) {
	pid_t traced_process;
	struct user_regs_struct regs, newregs;
    long ins;
	int i;

	char code[] = {0xcd,0x80,0xcc,0};
	char backup[4];

    if(argc != 2) {
        printf("Usage: %s <pid to be traced>\n", argv[0]);
        exit(1);
    }

    traced_process = atoi(argv[1]);
    int res = ptrace(PTRACE_ATTACH, traced_process, NULL, NULL);

	if (res) {
		printf("%s\n", strerror(errno));
		exit(1);
	}

    wait(NULL);
    ptrace(PTRACE_GETREGS, traced_process, NULL, &regs);
	
	// Copy instructions into backup variable
	getdata(traced_process, regs.rip, backup, 3);
	
	// Insert breakpoint
	putdata(traced_process, regs.rip, code, 3);

	wait(NULL);

	printf("The process stopped, putting back the original instructions\n");
    printf("Press <enter> to continue\n");
    getchar();

    putdata(traced_process, regs.rip, backup, 3);

    /* Setting the eip back to the original
       instruction to let the process continue */
    ptrace(PTRACE_SETREGS, traced_process, NULL, &regs);

    ptrace(PTRACE_DETACH, traced_process, NULL, NULL);
    return 0;
}
