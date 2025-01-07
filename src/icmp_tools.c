#include "../includes/ping.h"
#include <netdb.h>


struct addrinfo * get_addrinfo(const char *host)
{
	struct addrinfo hints;
	struct addrinfo *result;

	bzero(&hints, sizeof(struct addrinfo));
	result = malloc(sizeof(struct addrinfo **));
	/* Allow IPv4 only */
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_RAW;
	hints.ai_protocol = IPPROTO_ICMP;

	/* Getting the host addrinfo */
	if (getaddrinfo(host, NULL, &hints, &result) != 0)
	{
		printf("ping: %s: Name or service not known\n", host);
		return NULL;
	}
	return result;
}
