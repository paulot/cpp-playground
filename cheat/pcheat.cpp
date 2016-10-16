#include <string>
#include <iostream>
#include <fstream>
#include <vector>

#include <cstdio>
#include <cstdlib>

#include <sys/types.h>
#include <sys/uio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <limits.h>

#define READ_LEN 5000000

using namespace std;

struct MemoryMapping {
  unsigned long long addr_begin;
  unsigned long long addr_end;
  unsigned long long size;
  unsigned long long inode;
  string mapname, dev, permissions;
};

void split(int num, char buf[4]) {
	for (int i = 0; i < 4; i++) {
		buf[i] = num & 0xff;
		num >>= 8;
	}
}

void get_addresses(pid_t pid, vector<MemoryMapping> &addrs) {
  fstream maps("/proc/" + to_string(pid) + "/maps");
  string line;

  unsigned long long begin, end, size, inode, foo;
  char permissions[5], dev[6], mapname[PATH_MAX];

  while (getline(maps, line)) {
    sscanf(line.c_str(), "%llx-%llx %4s %llx %5s %lld %s", &begin, &end, permissions, &foo, dev, &inode, mapname);

    MemoryMapping mapping;
    mapping.addr_begin = begin;
    mapping.addr_end = end;
    mapping.size = end - begin;
    mapping.inode = inode;
    mapping.mapname = mapname;
    mapping.dev = dev;
    mapping.permissions = permissions;

    addrs.push_back(mapping);
  }
}

int main(int argc, char *argv[]) {
  if (argc != 2) printf("Usage: %s <pid>\n", argv[0]), exit(1);

  pid_t pid = atoi(argv[1]);

  vector<MemoryMapping> addrs;
  get_addresses(pid, addrs);

  cout << addrs.size() << endl;
  return 0;

	
	struct iovec local[1];
	struct iovec remote[1];
	char buf[READ_LEN];	
	ssize_t nread;

	long long stack_address = 0x00600000;
	
	local[0].iov_base = buf;
	local[0].iov_len = READ_LEN;
	remote[0].iov_base = (void *) stack_address;
	remote[0].iov_len = READ_LEN;

	nread = process_vm_readv(pid, local, 1, remote, 1, 0);
	if (nread == -1) perror("Could not read value: ");
	
	printf("Read %ld\n", nread);

	char needle_buf[4], new_value_buf[4];
	split(666, needle_buf);
	split(12039487, new_value_buf);

	for (int i = 0; i < nread; i += 4) {
		bool match = true;
		for (int j = 0; j < 4; j++) {
			if (needle_buf[j] != buf[i + j]) { match = false; break; }
		}

		if (match) {
			long long addr = stack_address + (i * 1);
			printf("Found the value at: 0x%llx\n", addr);

			local[0].iov_base = new_value_buf;
			local[0].iov_len = 4;
			remote[0].iov_base = (void *) addr;
			remote[0].iov_len = 4;

			ssize_t nwrite = process_vm_writev(pid, local, 1, remote, 1, 0);
			if (nwrite == -1) perror("Could not write the new value: ");		
		}
	}
	
	

  return 0;
}
