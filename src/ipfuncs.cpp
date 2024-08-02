#include <stdio.h>
#include <winsock2.h>
	int inet_pton_clone(const int family, char *instring, unsigned long *addr) {
		unsigned char ip[4];
		char ipnum[4];

		unsigned char i = 0;
		unsigned char i2 = 0;
		unsigned char i3 = 0;
		ipnum[3] = 0;
		while (i2 < 4) {
			i3 = 0;
			while (instring[i] != 0x2E && instring[i] != 0) {
				ipnum[i3] = instring[i];
				i3++;
				i++;
			}
			i++;
			ipnum[i3] = 0;
			ip[i2] = atoi(ipnum);
			i2++;
		}
		int *retval = (int*)(ip);
		addr[0] = *retval;
		return *retval;
	}

	char* inet_ntop_clone(const int family, IN_ADDR* addr, char *stringbuff, int stringbuffsize) {
		unsigned char* addr2 = (unsigned char*)&addr;
		sprintf(stringbuff, "%i.%i.%i.%i", addr2[0], addr2[1], addr2[2], addr2[3]);
		return stringbuff;
	}
	