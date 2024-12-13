/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kosyan62 <kosyan62@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/13 23:01:17 by kosyan62          #+#    #+#             */
/*   Updated: 2024/12/13 23:06:30 by kosyan62         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <getopt.h>
#include <stdio.h>

/*
 * List of available options:
 * v - verbose
 * f - prints dot on request and erase it on receive
 * l (preload) - preload send all packets immediately (for more than 3 use sudo) | 1 <= value <= 65536
 * n - Numeric output only. No attempt will be made to lookup symbolic names
 * 		for host addresses.
 * w (deadline) - timeout, in seconds, before ping exits regardless of how many
 * 		packets have been sent or received. | 0 <= value <= 2147483647
 * W (timeout) - timeout in seconds to wait for response. | Strange stuff - 2147483 is max value, uses first 3 bytes of integer maybe
 * p (pattern) - pad bytes to send | 16 bytes max
 * r - Bypass the normal routing tables and send directly to a host on an
 * 		attached interface
 * s (packetsize) - Specifies the number of data bytes to be sent. | 0 <= value <= 2147483647
 * t (ttl)- Set the IP Time to Live. | 0 <= value <= 255
 * T (timestamp) - Set the IP timestamp available values is
 * 		tsonly/tsandaddr/tsprespec.
 * c (count) - Stop after sending count ECHO_REQUEST packets. | In source code - 1 <= value <= LONG_MAX
 */

int	main(int argc, char **argv)
{
	int	option;

	while (1)
	{
		static struct option long_options[] = {\
        {"verbose", no_argument, 0, 'v'}, \
        {"flood", no_argument, 0, 'f'}, \
        {"preload", required_argument, 0, 'l'}, \
        {"numeric", no_argument, 0, 'n'}, \
        {"timeout", required_argument, 0, 'W'}, \
        {"deadline", required_argument, 0, 'w'}, \
        {"pattern", required_argument, 0, 'p'}, \
        {"direct", no_argument, 0, 'r'}, \
        {"packetsize", required_argument, 0, 's'}, \
        {"ttl", required_argument, 0, 't'}, \
        {"timestamp", required_argument, 0, 'T'}, \
        {"count", required_argument, 0, 'c'}, \
        {0, 0, 0, 0}
		};
		/* getopt_long stores the option index here. */
		int	options_index = 0;

		option = getopt_long(argc, argv, "vfl:nW:w:p:rs:t:T:c:", long_options, &options_index);
		/* Detect the end of the options. */
		if (option == -1)
			break ;
		printf("option %c ", option);
	}
}
