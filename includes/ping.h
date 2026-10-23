/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ping.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kosyan62 <kosyan62@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/13 23:01:42 by kosyan62          #+#    #+#             */
/*   Updated: 2026/10/23 12:00:00 by kosyan62         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PING_H
# define PING_H

# include <stddef.h>
# include <stdint.h>
# include <signal.h>
# include <sys/time.h>
# include <netinet/in.h>
# include <netinet/ip.h>
# include <netinet/ip_icmp.h>

# define ICMP_HDR_LEN		8
# define DEFAULT_DATALEN	56
# define MAX_DATALEN		(65535 - 20 - ICMP_HDR_LEN)
# define DEFAULT_LINGER		10
# define RECV_BUF_SIZE		65536

/* Exit code for usage errors, same as inetutils (EX_USAGE) */
# define EXIT_USAGE			64

typedef struct s_ping
{
	/* options */
	int					verbose;
	long				count;		/* -c: 0 means infinite */
	size_t				datalen;	/* -s: ICMP data bytes */
	int					ttl;		/* --ttl: 0 means system default */
	long				deadline;	/* -w: seconds, 0 means none */
	long				linger;		/* -W: seconds to wait for last reply */

	/* destination */
	char				*host;
	struct sockaddr_in	dst;
	int					sock;
	uint16_t			id;

	/* statistics */
	long				transmitted;
	long				received;
	double				tmin;
	double				tmax;
	double				tsum;
	double				tsum2;
	struct timeval		start;
	struct timeval		last_send;
}	t_ping;

extern volatile sig_atomic_t	g_stop;

/* options.c */
int		parse_args(t_ping *ping, int argc, char **argv, int *status);

/* socket.c */
int		resolve_host(t_ping *ping);
int		open_socket(t_ping *ping);

/* icmp_tools.c */
uint16_t	checksum(const void *data, size_t len);
int		send_echo(t_ping *ping);
void	handle_packet(t_ping *ping, unsigned char *buf, ssize_t len,
			struct sockaddr_in *from);

/* main.c */
double	tv_diff_ms(const struct timeval *a, const struct timeval *b);
void	print_stats(t_ping *ping);

#endif
