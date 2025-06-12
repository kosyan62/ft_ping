#include "../includes/ping.h"
#include <netdb.h>
#include <strings.h>  /* For bzero */


struct addrinfo * get_addrinfo(const char *host)
{
	struct addrinfo hints;
	struct addrinfo *result;

	bzero(&hints, sizeof(struct addrinfo));
	/* Allow IPv4 only */
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_RAW;
	hints.ai_protocol = IPPROTO_ICMP;

	/* Getting the host addrinfo */
	int err = getaddrinfo(host, NULL, &hints, &result);
	if (err != 0)
	{

		printf("ping: %s: Name or service not known\n", host);
		return NULL;
	}
	
	/* Count results for debugging */
	int count = 0;
	struct addrinfo *tmp = result;
	while (tmp) {
		count++;
		if (tmp->ai_family == AF_INET) {
			struct sockaddr_in *addr = (struct sockaddr_in *)tmp->ai_addr;
		}
		tmp = tmp->ai_next;
	}

	
	return result;
}