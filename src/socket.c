/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   socket.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kosyan62 <kosyan62@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/23 12:00:00 by kosyan62          #+#    #+#             */
/*   Updated: 2026/10/23 12:00:00 by kosyan62         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ping.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>

/*
 * Resolves an IPv4 address or a hostname (FQDN) into ping->dst.
 * Only the forward lookup is done: replies are printed with the bare IP.
 */
int	resolve_host(t_ping *ping)
{
	struct addrinfo	hints;
	struct addrinfo	*res;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_RAW;
	hints.ai_protocol = IPPROTO_ICMP;
	if (getaddrinfo(ping->host, NULL, &hints, &res) != 0 || res == NULL)
	{
		fprintf(stderr, "ft_ping: unknown host\n");
		return (-1);
	}
	memcpy(&ping->dst, res->ai_addr, sizeof(ping->dst));
	freeaddrinfo(res);
	return (0);
}

int	open_socket(t_ping *ping)
{
	ping->sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (ping->sock < 0)
	{
		if (errno == EPERM || errno == EACCES)
			fprintf(stderr, "ft_ping: Lack of privilege (run as root)\n");
		else
			fprintf(stderr, "ft_ping: socket: %s\n", strerror(errno));
		return (-1);
	}
	if (ping->ttl > 0
		&& setsockopt(ping->sock, IPPROTO_IP, IP_TTL,
			&ping->ttl, sizeof(ping->ttl)) < 0)
	{
		fprintf(stderr, "ft_ping: setsockopt(IP_TTL): %s\n", strerror(errno));
		return (-1);
	}
	return (0);
}
