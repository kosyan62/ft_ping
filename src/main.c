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
#include <sys/time.h>
#include <math.h>
#include <netinet/ip.h>
#include <signal.h>


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

_Bool ping_continue = 1;

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
		perror("Failed to create socket");
		return EXIT_FAILURE;
	}

	/* Setting TTL */
	ret = setsockopt(options->sock_fd, SOL_IP, IP_TTL, &options->ttl, sizeof(options->ttl));
	if (ret < 0)
	{
		perror("Failed to set TTL");
		return EXIT_FAILURE;
	}

	/* Setting timeout */
	tv_out.tv_sec = options->timeout / 1000;
	tv_out.tv_usec = (options->timeout % 1000) * 1000;
	if (options->verbose)
		printf("Timeout set to: %ld.%03ld seconds\n", tv_out.tv_sec, tv_out.tv_usec/1000);
	ret = setsockopt(options->sock_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv_out, sizeof tv_out);
	if (ret < 0)
	{
		perror("Failed to set timeout");
		return EXIT_FAILURE;
	}

	/* Getting the host addrinfo */
	local_dst_info = get_addrinfo(options->host);
	if (!local_dst_info)
	{
		return EXIT_FAILURE;
	}
	options->dst_info = local_dst_info;

	/* Looking for IPv4 address */
	while (local_dst_info)
	{
		if (local_dst_info->ai_family == AF_INET)
		{
			struct sockaddr_in *addr = (struct sockaddr_in *)local_dst_info->ai_addr;
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
	/* We moved the initial message to ft_ping for consistency with standard ping */
	return EXIT_SUCCESS;
}

void	fill_icmp_packet(void *buffer, int sequence, size_t packet_size)
{
	struct icmp *packet = (struct icmp *)buffer;
	packet->icmp_type = ICMP_ECHO;
	packet->icmp_code = 0;
	packet->icmp_id = htons(getpid() & 0xFFFF);  // Use network byte order for ID and truncate to 16 bits
	packet->icmp_seq = htons(sequence);          // Convert sequence to network byte order too

	/* Fill data portion with pattern (after 8-byte ICMP header) */
	unsigned char *data = (unsigned char *)buffer + 8;
	for (size_t i = 0; i < packet_size - 8; i++) {
		data[i] = i % 256;
	}

	packet->icmp_cksum = 0;
	packet->icmp_cksum = checksum((unsigned short *)buffer, packet_size);
}

void sigint_handler(int signum)
{
	if (signum == SIGINT)
	{
		ping_continue = 0;  // Set flag to stop pinging
	}
}

int ft_ping(char *host, t_options options)
{
	/* Sending and receiving */
	int ret;
	int sent = 0;
	int received = 0;
	struct sockaddr_in *dst_addr_in;
	struct sockaddr_in src_addr;
	socklen_t src_addr_len;
	char packet_buffer[options.packetsize];
	char recv_buffer[options.packetsize + 128]; /* Add extra space for IP header */
	struct timeval packet_start_time;
	struct timeval end_time;
	struct timeval program_start_time;
	struct timeval program_end_time;

	/* Statistics */
	double min_rtt = -1;  /* Use -1 as a flag value */
	double max_rtt = 0;
	double total_rtt = 0;
	double rtt_sum_squares = 0;

	dst_addr_in = (struct sockaddr_in *)options.dst_addr;
	src_addr_len = sizeof(src_addr);
	gettimeofday(&program_start_time, NULL);
	/* Set sigint handler*/
	signal(SIGINT, sigint_handler);  // Ignore SIGINT for now, we will handle it later
	/* Initial message */
	if (options.verbose) {
		printf("PING %s (%s): %d data bytes, id 0x%04x = %d\n",
			options.host,
			inet_ntoa(dst_addr_in->sin_addr),
			options.packetsize - 8,
			getpid() & 0xFFFF,
			getpid() & 0xFFFF);
	} else {
		printf("PING %s (%s): %d data bytes\n",
			options.host,
			inet_ntoa(dst_addr_in->sin_addr),
			options.packetsize - 8);
	}

	while (sent < options.count && ping_continue)
	{
		/* Fill packet */
		memset(packet_buffer, 0, options.packetsize);
		fill_icmp_packet(packet_buffer, sent, options.packetsize);

		/* Record start time for this packet */
		gettimeofday(&packet_start_time, NULL);

		/* Send packet */
		/* Verbose message moved to header */

		ret = (int) sendto(options.sock_fd, packet_buffer, options.packetsize, 0, options.dst_addr, options.dst_addr_len);
		if (ret < 0)
		{
			printf("Failed to send packet\n");
			return EXIT_FAILURE;
		}
		sent++;

		/* Receive packet - keep trying until we get a valid reply or timeout */
		src_addr_len = sizeof(src_addr);

		/* Track current sequence number we're looking for */
		int expected_seq = sent - 1;

		/* Set a time limit for receiving valid replies */
		struct timeval receive_start_time;
		gettimeofday(&receive_start_time, NULL);
		struct timeval current_time;
		bool valid_reply_received = false;
		double rtt_ms = 0;
		struct iphdr *ip_header = NULL;
		struct icmp *icmp_header = NULL;

		while (!valid_reply_received) {
			/* Check if we've been waiting too long (more than socket timeout) */
			gettimeofday(&current_time, NULL);
			long wait_time_ms = (current_time.tv_sec - receive_start_time.tv_sec) * 1000 +
				(current_time.tv_usec - receive_start_time.tv_usec) / 1000;
			if (wait_time_ms > options.timeout) {
				if (options.verbose)
					printf("Request timed out\n");
				break;
			}

			ret = recvfrom(options.sock_fd, recv_buffer, sizeof(recv_buffer), MSG_DONTWAIT, (struct sockaddr*)&src_addr, &src_addr_len);
			if (ret < 0)
			{
				if (errno == EAGAIN || errno == EWOULDBLOCK) {
					/* Timeout occurred, just exit the loop */
					if (options.verbose)
						printf("Request timed out\n");
					break;
				}
				else if ( errno == EINTR ) {
					/* Interrupted by signal, just leave the loop */
					ping_continue = 0;  // Stop pinging on signal
					if (options.verbose)
						printf("Request interrupted by signal\n");
				}
				else {
					perror("recvfrom");
					return EXIT_FAILURE;
				}
			}

			/* Calculate RTT */
			gettimeofday(&end_time, NULL);
			long time_diff_us = (end_time.tv_sec - packet_start_time.tv_sec) * 1000000 +
								(end_time.tv_usec - packet_start_time.tv_usec);
			rtt_ms = time_diff_us / 1000.0; /* Convert to milliseconds with decimal precision */

			/* Parse the IP and ICMP headers */
			ip_header = (struct iphdr *)recv_buffer;
			icmp_header = (struct icmp *)(recv_buffer + (ip_header->ihl * 4));

			/* Check if this is a valid reply for our request */
			if (icmp_header->icmp_type == ICMP_ECHOREPLY &&
				ntohs(icmp_header->icmp_id) == (getpid() & 0xFFFF)) {

				/* Check if the sequence number matches what we expect */
				if (ntohs(icmp_header->icmp_seq) == expected_seq) {
					valid_reply_received = true;
				} else {
					/* If sequence doesn't match, we continue looking */
					continue;
				}
			} else {
				/* Continue to wait for a valid reply */
				continue;
			}
		}

		/* If no valid reply was received, continue to next packet */
		if (!valid_reply_received) {
			if (!options.flood)
				sleep(1); /* Wait a bit before sending the next packet */
			continue;
		}

		/* Successfully received a valid reply */
		received++;

		/* Update statistics */
		if (min_rtt < 0 || rtt_ms < min_rtt)
			min_rtt = rtt_ms;
		if (rtt_ms > max_rtt)
			max_rtt = rtt_ms;
		total_rtt += rtt_ms;
		rtt_sum_squares += rtt_ms * rtt_ms;

		/* Output in standard ping format */
		printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n",
			ret - (ip_header->ihl * 4), /* Subtract IP header length */
			inet_ntoa(src_addr.sin_addr),
			ntohs(icmp_header->icmp_seq),
			ip_header->ttl,
			rtt_ms);

		/* Wait a bit before sending the next packet */
		if (!options.flood)
			sleep(1);
	}

	/* Calculate program duration */
	gettimeofday(&program_end_time, NULL);
	long total_time_ms = (program_end_time.tv_sec - program_start_time.tv_sec) * 1000 +
					(program_end_time.tv_usec - program_start_time.tv_usec) / 1000;

	/* Calculate statistics */
	double avg_rtt = received > 0 ? total_rtt / received : 0;
	double stddev = 0;
	if (received > 1) {
		stddev = sqrt((rtt_sum_squares - (total_rtt * total_rtt) / received) / (received - 1));
	}

	/* Output statistics */
	printf("--- %s ping statistics ---\n", host);
	printf("%d packets transmitted, %d packets received, %d%% packet loss\n",
		sent, received, received > 0 ? (sent - received) * 100 / sent : 100);

	if (received > 0) {
		printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n",
			min_rtt, avg_rtt, max_rtt, stddev);
	}

	/* Return 1 if we didn't receive any packets, 0 if we got at least one response */
	return (received == 0) ? EXIT_FAILURE : EXIT_SUCCESS;
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
	parsed_options.packetsize = 64;  // 56 data bytes + 8 ICMP header bytes = 64
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
        {"help", no_argument, 0, '?'}, \
        {"version", no_argument, 0, 'V'}, \
        {0, 0, 0, 0}
		};
		/* getopt_long stores the option index here. */
		int	options_index = 0;

		option = getopt_long(argc, argv, "vfl:nW:w:p:rs:t:T:c:?V", long_options, &options_index);
		/* Detect the end of the options. */
		if (option == -1)
			break ;
		else if (option == '?')
		{
			/* Display help information */
			printf("Usage: ft_ping [-v] [-c count] [-i interval] [-W timeout] host\n");
			printf("Send ICMP ECHO_REQUEST packets to network hosts\n\n");
			printf("Options:\n");
			printf("  -v             verbose output\n");
			printf("  -c count       stop after <count> replies\n");
			printf("  -f             flood ping\n");
			printf("  -l preload     send <preload> number of packets as fast as possible\n");
			printf("  -n             numeric output only\n");
			printf("  -w deadline    timeout in seconds before ping exits\n");
			printf("  -W timeout     time to wait for response\n");
			printf("  -r             bypass routing tables\n");
			printf("  -s packetsize  set number of data bytes to send\n");
			printf("  -t ttl         set time to live\n");
			printf("  -?             display this help and exit\n");
			printf("  -V             output version information and exit\n");
			return EXIT_SUCCESS;
		}
		else if (option == 'V')
		{
			/* Display version information */
			printf("ft_ping (custom implementation) 1.0\n");
			printf("Based on inetutils ping\n");
			return EXIT_SUCCESS;
		}
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