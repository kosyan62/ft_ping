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
		fprintf(stderr, "ft_ping: unknown host\n");
		return NULL;
	}
	return result;
}