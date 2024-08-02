#include <winsock2.h>
#ifndef inet_pton
#ifndef inet_ntop
	int inet_pton_clone(const int family, char *instring, unsigned long *addr);
	char* inet_ntop_clone(const int family, IN_ADDR* addr, char *stringbuff, int stringbuffsize);

	#define inet_pton inet_pton_clone
	#define inet_ntop inet_ntop_clone
	#define InetPtonA inet_pton_clone
	#define InetNtopA inet_ntop_clone
#endif
#endif