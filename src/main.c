/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kosyan62 <kosyan62@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/13 23:01:17 by kosyan62          #+#    #+#             */
/*   Updated: 2026/10/23 12:00:00 by kosyan62         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ping.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>

volatile sig_atomic_t	g_stop = 0;

static void	on_sigint(int sig)
{
	(void)sig;
	g_stop = 1;
}

double	tv_diff_ms(const struct timeval *a, const struct timeval *b)
{
	return ((a->tv_sec - b->tv_sec) * 1000.0
		+ (a->tv_usec - b->tv_usec) / 1000.0);
}

void	print_stats(t_ping *ping)
{
	double	avg;

	printf("--- %s ping statistics ---\n", ping->host);
	printf("%ld packets transmitted, %ld packets received, ",
		ping->transmitted, ping->received);
	if (ping->transmitted > 0)
		printf("%ld%% packet loss",
			(ping->transmitted - ping->received) * 100 / ping->transmitted);
	printf("\n");
	if (ping->received > 0 && ping->datalen >= sizeof(struct timeval))
	{
		avg = ping->tsum / ping->received;
		printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n",
			ping->tmin, avg, ping->tmax,
			sqrt(fabs(ping->tsum2 / ping->received - avg * avg)));
	}
}

/*
 * Returns how many milliseconds we may wait in select() before something
 * has to happen, or -1 if pinging is over.
 */
static double	next_timeout(t_ping *ping)
{
	struct timeval	now;
	double			wait;
	double			left;

	gettimeofday(&now, NULL);
	if (ping->count && ping->transmitted >= ping->count)
	{
		if (ping->received >= ping->transmitted)
			return (-1);
		wait = ping->linger * 1000.0 - tv_diff_ms(&now, &ping->last_send);
		if (wait <= 0)
			return (-1);
	}
	else
		wait = 1000.0 - tv_diff_ms(&now, &ping->last_send);
	if (ping->deadline)
	{
		left = ping->deadline * 1000.0 - tv_diff_ms(&now, &ping->start);
		if (left <= 0)
			return (-1);
		if (left < wait)
			wait = left;
	}
	if (wait < 0)
		wait = 0;
	return (wait);
}

static void	receive_one(t_ping *ping)
{
	static unsigned char	buf[RECV_BUF_SIZE];
	struct sockaddr_in		from;
	socklen_t				fromlen;
	ssize_t					n;

	fromlen = sizeof(from);
	n = recvfrom(ping->sock, buf, sizeof(buf), 0,
			(struct sockaddr *)&from, &fromlen);
	if (n < 0)
	{
		if (errno != EINTR && errno != EAGAIN)
			fprintf(stderr, "ft_ping: recvfrom: %s\n", strerror(errno));
		return ;
	}
	handle_packet(ping, buf, n, &from);
}

/*
 * Main loop: one echo request every second, replies are read as soon as
 * they arrive. With -c, after the last request we wait for the missing
 * replies at most -W seconds. -w stops everything after N seconds.
 */
static void	ping_loop(t_ping *ping)
{
	fd_set			fds;
	struct timeval	tv;
	double			wait;
	int				ret;

	gettimeofday(&ping->start, NULL);
	send_echo(ping);
	while (!g_stop)
	{
		wait = next_timeout(ping);
		if (wait < 0)
			break ;
		FD_ZERO(&fds);
		FD_SET(ping->sock, &fds);
		tv.tv_sec = (long)(wait / 1000);
		tv.tv_usec = (long)((wait - tv.tv_sec * 1000.0) * 1000);
		ret = select(ping->sock + 1, &fds, NULL, NULL, &tv);
		if (ret < 0 && errno != EINTR)
		{
			fprintf(stderr, "ft_ping: select: %s\n", strerror(errno));
			break ;
		}
		if (ret > 0)
			receive_one(ping);
		else if (ret == 0 && (!ping->count || ping->transmitted < ping->count)
			&& next_timeout(ping) == 0)
			send_echo(ping);
	}
}

static void	init_ping(t_ping *ping)
{
	memset(ping, 0, sizeof(*ping));
	ping->datalen = DEFAULT_DATALEN;
	ping->linger = DEFAULT_LINGER;
	ping->sock = -1;
	ping->id = (uint16_t)(getpid() & 0xFFFF);
}

int	main(int argc, char **argv)
{
	t_ping				ping;
	int					status;
	struct sigaction	sa;

	init_ping(&ping);
	if (parse_args(&ping, argc, argv, &status) < 0)
		return (status);
	if (resolve_host(&ping) < 0 || open_socket(&ping) < 0)
	{
		if (ping.sock >= 0)
			close(ping.sock);
		return (EXIT_FAILURE);
	}
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = on_sigint;
	sigaction(SIGINT, &sa, NULL);
	printf("PING %s (%s): %zu data bytes", ping.host,
		inet_ntoa(ping.dst.sin_addr), ping.datalen);
	if (ping.verbose)
		printf(", id 0x%04x = %u", ping.id, ping.id);
	printf("\n");
	ping_loop(&ping);
	print_stats(&ping);
	close(ping.sock);
	return (ping.received ? EXIT_SUCCESS : EXIT_FAILURE);
}
