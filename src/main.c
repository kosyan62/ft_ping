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
#include <unistd.h>
#include <netinet/ip_icmp.h>
#include <sys/time.h>
#include <arpa/inet.h>


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


unsigned short checksum(void *b, int len)
{
	unsigned short *buf = b;
	unsigned int sum = 0;
	unsigned short result;

	for (sum = 0; len > 1; len -= 2)
		sum += *buf++;
	if (len == 1)
		sum += *(unsigned char *)buf;
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	result = ~sum;
	return result;
}

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

int setup(t_options *options)
{
	int ret;
	struct timeval tv_out;
	struct addrinfo *local_dst_info;

	/* Creating socket */
	options->sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (options->sock_fd < 0)
	{
		printf("Failed to create socket\n");
		return EXIT_FAILURE;
	}

	/* Setting TTL */
	ret = setsockopt(options->sock_fd, SOL_IP, IP_TTL, &options->ttl, sizeof(options->ttl));
	if (ret < 0)
	{
		printf("Failed to set TTL\n");
		return EXIT_FAILURE;
	}

	/* Setting timeout */
	tv_out.tv_sec = options->timeout / 1000;
	tv_out.tv_usec = (options->timeout % 1000) * 1000;
	ret = setsockopt(options->sock_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv_out, sizeof tv_out);
	if (ret < 0)
	{
		printf("Failed to set timeout\n");
		return EXIT_FAILURE;
	}

	/* Getting the host addrinfo */
	local_dst_info = get_addrinfo(options->host);
	if (!local_dst_info)
		return EXIT_FAILURE;
	options->dst_info = local_dst_info;

	/* Looking for IPv4 address */
	while (local_dst_info)
	{
		if (local_dst_info->ai_family == AF_INET)
		{
			break ;
		}
		local_dst_info = local_dst_info->ai_next;
	}
	if (!local_dst_info)
	{
		printf("ping: %s: Name or service not known\n", options->host);
		freeaddrinfo(options->dst_info);
		return EXIT_FAILURE;
	}
	options->dst_addr = local_dst_info->ai_addr;
	options->dst_addr_len = local_dst_info->ai_addrlen;
	return EXIT_SUCCESS;
}

int ft_ping(char *host, t_options options)
{
	/* Create packet */
	struct icmphdr packet;
	packet.type = ICMP_ECHO;
	packet.code = 0;
	packet.checksum = 0;
	packet.un.echo.id = getpid();
	packet.un.echo.sequence = 1;
	packet.checksum = checksum((unsigned short *)&packet, sizeof(packet));

	/* Sending and receiving */
	int ret;
	int i;
	int sent = 0;
	int received = 0;
	struct sockaddr_in *dst_addr_in;
	struct sockaddr_in src_addr;
	socklen_t src_addr_len;
	char packet_buffer[options.packetsize];
	char recv_buffer[options.packetsize];
	struct timeval start_time;
	struct timeval end_time;
	long time_diff;
	long min_time = 0;
	long max_time = 0;
	long avg_time = 0;
	long total_time = 0;

	dst_addr_in = (struct sockaddr_in *)options.dst_addr;
	src_addr_len = sizeof(src_addr);
	gettimeofday(&start_time, NULL);

	printf("COUNT: %ld\n", options.count);
	while (sent < options.count)
	{
		/* Send packet */
		ret = sendto(options.sock_fd, &packet, sizeof(packet), 0, options.dst_addr, options.dst_addr_len);
		if (ret < 0)
		{
			printf("Failed to send packet\n");
			return EXIT_FAILURE;
		}
		sent++;
		/* Receive packet */
		ret = recvfrom(options.sock_fd, recv_buffer, options.packetsize, 0, (struct sockaddr *)&src_addr, &src_addr_len);
		if (ret < 0)
		{
			printf("Failed to receive packet\n");
			return EXIT_FAILURE;
		}
		printf("Received packet\n");
		struct iphdr *ip_header = (struct iphdr *)recv_buffer;
		struct icmphdr *icmp_header = (struct icmphdr *)(recv_buffer + ip_header->ihl * 4);

		gettimeofday(&end_time, NULL);
		time_diff = (end_time.tv_sec - start_time.tv_sec) * 1000 + (end_time.tv_usec - start_time.tv_usec) / 1000;
		if (time_diff < min_time || min_time == 0)
			min_time = time_diff;
		if (time_diff > max_time)
			max_time = time_diff;
		total_time += time_diff;
		if (icmp_header->type == ICMP_ECHOREPLY)
		{
			received++;
		}
		printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%ld ms type=%d code=%d\n", ret, inet_ntoa(src_addr.sin_addr), packet.un.echo.sequence, options.ttl, time_diff, icmp_header->type, icmp_header->code);
	}
	avg_time = total_time / sent;
	printf("--- %s ping statistics ---\n", host);
	printf("%d packets transmitted, %d received, %d%% packet loss, time %ldms\n", sent, received, (sent - received) * 100 / sent, total_time);


	return EXIT_SUCCESS;
}


int	main(int argc, char **argv)
{
	int ret;
	int	option;
	t_options parsed_options;

	/* Initialize options with defaults*/
	parsed_options.dst_info = NULL;
	parsed_options.verbose = false;
	parsed_options.flood = false;
	parsed_options.preload = 0;
	parsed_options.numeric = false;
	parsed_options.timeout = 1000;
	parsed_options.direct = false;
	parsed_options.packetsize = 56;
	parsed_options.ttl = 64;
	parsed_options.count = 3;

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
	if (getuid() != 0)
	{
		printf("ping: permission denied: Please run as root\n");
		return EXIT_FAILURE;
	}
	if (strlen(argv[optind]) > 255)
	{
		printf("ping: %s: Name or service not known (too long)\n", argv[optind]);
		return EXIT_FAILURE;
	}
	strcpy(parsed_options.host, argv[optind]);

	if (setup(&parsed_options) == EXIT_FAILURE)
		return EXIT_FAILURE;
	ret = ft_ping(argv[optind], parsed_options);
	freeaddrinfo(parsed_options.dst_info);
	if (parsed_options.sock_fd)
		close(parsed_options.sock_fd);
	return ret;
}

