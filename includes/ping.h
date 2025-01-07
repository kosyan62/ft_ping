/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ping.h                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kosyan62 <kosyan62@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/13 23:01:42 by kosyan62          #+#    #+#             */
/*   Updated: 2024/12/13 23:01:42 by kosyan62         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_PING_H
#define FT_PING_H

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

/*
 * Booleans are usefull, so we define them here.
 */
#define bool unsigned char
#define true 1
#define false 0

/*
 * Options for ping, in the order they appear in the struct.
 */

typedef	struct 	s_options
{
	char				host[256];
	struct addrinfo		*dst_info;
	struct sockaddr		*dst_addr;
	size_t 				dst_addr_len;
	int					sock_fd;
	bool				verbose;
	bool				flood;
	unsigned	short	preload;
	bool				numeric;
	int					timeout;
	int					deadline;
	bool				direct;
	int					packetsize;
	unsigned	char	ttl;
//	char	*timestamp; TODO - implement timestamp options later
	long				count;
}				t_options;


struct addrinfo * get_addrinfo(const char *host);


#endif
