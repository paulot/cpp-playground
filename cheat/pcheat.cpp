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



void printbincharpad(char c) {
  for (int i = 7; i >= 0; --i) {
    putchar( (c & (1 << i)) ? '1' : '0' );
  }
  putchar('\n');
}

// Splits a given integer into 4 characters based on the binary value.
// Given a integer 0b1001010111010101 this function will split it into the folowing chars:
// [0b00000000, 0b00000000, 0b10010101, 0b11010101]
//
// @param num: the number to split.
// @param buf: the array to populate with characters.
void split(unsigned long long num, int size, vector<char> &buf) {
  int mask = (1 << CHAR_BIT) - 1;

	for (int i = 0; i < size; i++) {
		buf.push_back(num & mask);
		num >>= CHAR_BIT;
	}
}

// Gets the memory addresses mapped to a given process.
//
// @param pid: the pid of the process to get the memory mapping.
// @param addrs: vector to populate with the memory mappings.
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

void find_value(pid_t pid, vector<char> &needle, MemoryMapping mapping, vector<unsigned long long> &addrs) {
	struct iovec local[1], remote[1];
  char *buf = new char[mapping.size];
	ssize_t nread;
	
	local[0].iov_base = buf;
	local[0].iov_len = mapping.size;
	remote[0].iov_base = (void *) mapping.addr_begin;
	remote[0].iov_len = mapping.size;

	nread = process_vm_readv(pid, local, 1, remote, 1, 0);
	if (nread == -1) perror("Could not read value: ");

  for (int i = 0; i < nread; i += needle.size()) {
		bool match = true;

		for (int j = 0; j < needle.size(); j++)
			if (needle[j] != buf[i + j]) { match = false; break; }

    if (match) addrs.push_back(mapping.addr_begin + i);
  }

  delete[] buf;
}

void write_address(pid_t pid, vector<char> &value, unsigned long long addr) {
	struct iovec local[1], remote[1];
  char *buf = new char[value.size()];

  // Copy value to buffer
  for (int i = 0; i < value.size(); i++) buf[i] = value[i];

  local[0].iov_base = buf;
  local[0].iov_len = value.size();
  remote[0].iov_base = (void *) addr;
  remote[0].iov_len = value.size();


  ssize_t nwrite = process_vm_writev(pid, local, 1, remote, 1, 0);
  if (nwrite == -1) perror("Could not write the new value: ");		

  delete[] buf;
}


int main(int argc, char *argv[]) {
  if (argc != 2) printf("Usage: %s <pid>\n", argv[0]), exit(1);

  pid_t pid = atoi(argv[1]);

  vector<MemoryMapping> mappings;
  get_addresses(pid, mappings);


  unsigned int input;
  vector<unsigned long long> addrs;
  vector<char> to_find;

  cout << "Which number are you looking for?" << endl;

  do {
    addrs.clear();
    cin >> input;

    // split(12039487, 4, to_find);
    split(input, 4, to_find);

    for (int i = 0; i < mappings.size(); i++) {
      find_value(pid, to_find, mappings[i], addrs);
    }

    mappings.clear();

    for (int i = 0; i < addrs.size(); i++) {
      MemoryMapping cur;
      cur.size = 4;
      cur.addr_begin = addrs[i];
      cur.addr_end = addrs[i] + 4;

      mappings.push_back(cur);
    }

    if (addrs.size() == 1) cout << "BOOOOYAAAA!" << endl;
    else cout << "Found " << addrs.size() << " matches! Change the number and input again:"<< endl;
  } while(addrs.size() > 1 and addrs.size() != 0);

  if (addrs.size() == 0) { cout << "Could not find the value you're looking for..." << endl; return 0; }

  to_find.clear();
  printf("Found value at: 0x%llx\n", addrs[0]);
  cout << "What do you want to set the value as: " << endl;
  cin >> input;
  split(input, 4, to_find);

  write_address(pid, to_find, addrs[0]);

  return 0;

  /*
	
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
  */
}
