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

#include "../includes/ping.h"
#include <getopt.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/types.h>


/*
 * List of available (and some not implemented) options:
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
int parse_option(t_options *parsed_options, int option, char *argument)
{
	long value;
	double dvalue;
	if (option == 'v')
		parsed_options->verbose = true;
	else if (option == 'f')
		parsed_options->flood = true;
	else if (option == 'l')
	{
		value = strtol(argument, NULL, 10);
		if ((errno == ERANGE) || (value < 1) || (value > 65536))
		{
			printf("ft_ping: invalid argument '%s': out of range 1 <= value <= 65536\n",
				   argument);
			return -1;
		}
		parsed_options->preload = value;
	}
	else if (option == 'n')
		parsed_options->numeric = true;
	else if (option == 'w')
	{
		value = strtol(argument, NULL, 10);
		if ((errno == ERANGE) || (0 > value) || (value > 2147483647))
		{
			printf("ft_ping: invalid argument '%s': out of range 0 <= value <= 2147483647\n",
				   argument);
			return -1;
		}
		parsed_options->deadline = (int) value;
	}
	else if (option == 'W')
	{
		dvalue = strtod(argument, NULL);
		if ((errno == ERANGE) || (0.0 > dvalue) || (dvalue > 2147483))
		{
			printf("ft_ping: invalid argument '%s': bad linger time\n",
				   argument);
			return -1;
		}
		parsed_options->timeout = (int) dvalue * 1000;
	}
	else if (option == 'r')
		parsed_options->direct = true;
	else if (option == 's')
	{
		value = strtol(argument, NULL, 10);
		if ((errno == ERANGE) || (0 > value) || (value > 2147483647))
		{
			printf("ft_ping: invalid argument '%s': out of range 0 <= value <= 2147483647\n",
				   argument);
			return -1;
		}
		parsed_options->packetsize = (int) value;
	}
	else if (option == 't')
	{
		value = strtol(argument, NULL, 10);
		if ((errno == ERANGE) || (0 > value) || (value > 255))
		{
			printf("ft_ping: invalid argument '%s': out of range 0 <= value <= 255\n",
				   argument);
			return -1;
		}
		parsed_options->ttl = (unsigned char) value;
	}
	else if (option == 'c')
	{
		value = strtol(argument, NULL, 10);
		if ((errno == ERANGE) || (1 > value) || (value > 9223372036854775807))
		{
			printf("ft_ping: invalid argument '%s': out of range 1 <= value <= 9223372036854775807\n",
				   argument);
			return -1;
		}
		parsed_options->count = value;
	}
	else
	{
		printf("Invalid option '%c'\n", option);
		return -1;
	}
	return 0;
}

int do_the_job(char *host, t_options options)
{
	struct addrinfo *dst_info, *to_free;
	int sock_fd;
	char buffer[1024];
	long ret;

	sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sock_fd < 0)
	{
		printf("Failed to create socket\n");
		return EXIT_FAILURE;
	}
	dst_info = get_addrinfo(host);
	if (!dst_info)
		return EXIT_FAILURE;
	/* Looking for IPv4 address */
	to_free = dst_info;
	while (dst_info)
	{
		if (dst_info->ai_family == AF_INET)
		{
			break ;
		}
		dst_info = dst_info->ai_next;
	}
	if (!dst_info)
	{
		printf("ping: %s: Name or service not known\n", host);
		free_addrinfo(to_free);
		return EXIT_FAILURE;
	}
	bzero(buffer, 1024);
	memset(buffer, 0xff, 1024);
	ret = sendto(sock_fd, buffer, 1024, 0, dst_info->ai_addr, dst_info->ai_addrlen);
	printf("Sent: %ld\n", ret);
	ret = recvfrom(sock_fd, buffer, 1024, 0, dst_info->ai_addr, &dst_info->ai_addrlen);
	printf("Received: %ld %s\n", ret, buffer);
	free_addrinfo(to_free);
	return EXIT_SUCCESS;
}


int	main(int argc, char **argv)
{
	int	option;
	t_options parsed_options;

	/* Initialize options with zeroes */
	bzero(&parsed_options, sizeof(t_options));
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
		else if (option == '?')
			/* getopt_long already printed an error message. */
			return (1);
		else
		{
			if (parse_option(&parsed_options, option, optarg) == -1)
				return EXIT_FAILURE;
		}
	}
	if (optind == argc)
	{
		printf("ping: usage error: Destination address required\n");
		return EXIT_FAILURE;
	}
	// TODO - implement multiple hosts
	if (optind + 1 < argc)
	{
		printf("ping: usage error: Multiple destination addresses not supported\n");
		return EXIT_FAILURE;
	}
	printf("Host to work with: %s\n", argv[optind]);
	return  do_the_job(argv[optind], parsed_options);
}


