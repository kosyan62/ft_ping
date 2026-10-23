/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   icmp_tools.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kosyan62 <kosyan62@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2024/12/13 23:01:42 by kosyan62          #+#    #+#             */
/*   Updated: 2026/10/23 12:00:00 by kosyan62         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ping.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>

uint16_t	checksum(const void *data, size_t len)
{
	const uint16_t	*p;
	uint32_t		sum;

	p = data;
	sum = 0;
	while (len > 1)
	{
		sum += *p++;
		len -= 2;
	}
	if (len == 1)
		sum += *(const uint8_t *)p;
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += sum >> 16;
	return ((uint16_t)~sum);
}

/*
 * Echo request layout: 8 bytes ICMP header, then the data. If the data is
 * big enough, its first bytes hold the send time, so the RTT can be computed
 * from the reply itself. The rest is filled with an incrementing pattern.
 */
int	send_echo(t_ping *ping)
{
	static unsigned char	buf[ICMP_HDR_LEN + MAX_DATALEN];
	struct icmp				*icmp;
	size_t					len;
	size_t					i;

	len = ICMP_HDR_LEN + ping->datalen;
	memset(buf, 0, len);
	icmp = (struct icmp *)buf;
	icmp->icmp_type = ICMP_ECHO;
	icmp->icmp_code = 0;
	icmp->icmp_id = htons(ping->id);
	icmp->icmp_seq = htons((uint16_t)ping->transmitted);
	i = 0;
	while (i < ping->datalen)
	{
		buf[ICMP_HDR_LEN + i] = (unsigned char)i;
		i++;
	}
	gettimeofday(&ping->last_send, NULL);
	if (ping->datalen >= sizeof(struct timeval))
		memcpy(buf + ICMP_HDR_LEN, &ping->last_send, sizeof(struct timeval));
	icmp->icmp_cksum = checksum(buf, len);
	ping->transmitted++;
	if (sendto(ping->sock, buf, len, 0, (struct sockaddr *)&ping->dst,
			sizeof(ping->dst)) < 0)
	{
		fprintf(stderr, "ft_ping: sending packet: %s\n", strerror(errno));
		return (-1);
	}
	return (0);
}

static void	print_echo_reply(t_ping *ping, struct ip *ip, struct icmp *icmp,
				size_t icmp_len, struct sockaddr_in *from)
{
	struct timeval	sent;
	struct timeval	now;
	double			rtt;

	ping->received++;
	printf("%zu bytes from %s: icmp_seq=%u ttl=%d", icmp_len,
		inet_ntoa(from->sin_addr), ntohs(icmp->icmp_seq), ip->ip_ttl);
	if (ping->datalen >= sizeof(struct timeval)
		&& icmp_len >= ICMP_HDR_LEN + sizeof(struct timeval))
	{
		gettimeofday(&now, NULL);
		memcpy(&sent, (unsigned char *)icmp + ICMP_HDR_LEN, sizeof(sent));
		rtt = tv_diff_ms(&now, &sent);
		if (ping->received == 1 || rtt < ping->tmin)
			ping->tmin = rtt;
		if (rtt > ping->tmax)
			ping->tmax = rtt;
		ping->tsum += rtt;
		ping->tsum2 += rtt * rtt;
		printf(" time=%.3f ms", rtt);
	}
	printf("\n");
}

static const char	*icmp_description(int type, int code)
{
	static const char	*unreach[] = {
		"Destination Net Unreachable", "Destination Host Unreachable",
		"Destination Protocol Unreachable", "Destination Port Unreachable",
		"Fragmentation needed and DF set", "Source Route Failed",
		"Destination Net Unknown", "Destination Host Unknown",
		"Source Host Isolated", "Destination Network Prohibited",
		"Destination Host Prohibited",
		"Destination Network Unreachable At This TOS",
		"Destination Host Unreachable At This TOS", "Packet Filtered",
		"Precedence Violation", "Precedence Cutoff"};
	static const char	*redirect[] = {
		"Redirect Network", "Redirect Host",
		"Redirect Type of Service and Network",
		"Redirect Type of Service and Host"};

	if (type == ICMP_DEST_UNREACH && code >= 0 && code <= 15)
		return (unreach[code]);
	if (type == ICMP_REDIRECT && code >= 0 && code <= 3)
		return (redirect[code]);
	if (type == ICMP_TIME_EXCEEDED && code == 0)
		return ("Time to live exceeded");
	if (type == ICMP_TIME_EXCEEDED && code == 1)
		return ("Frag reassembly time exceeded");
	if (type == ICMP_SOURCE_QUENCH)
		return ("Source Quench");
	if (type == ICMP_PARAMPROB)
		return ("Parameter problem");
	return ("Unknown ICMP type or code");
}

/*
 * Verbose dump of the original IP header and ICMP header carried
 * inside an ICMP error message (same layout as inetutils ping -v).
 */
static void	dump_original(struct ip *ip)
{
	unsigned char	*bytes;
	unsigned char	*inner;
	size_t			i;

	bytes = (unsigned char *)ip;
	printf("IP Hdr Dump:\n ");
	i = 0;
	while (i < sizeof(struct ip))
	{
		printf("%02x%s", bytes[i], (i % 2) ? " " : "");
		i++;
	}
	printf("\nVr HL TOS  Len   ID Flg  off TTL Pro  cks      Src\tDst\tData\n");
	printf(" %1x  %1x  %02x %04x %04x", ip->ip_v, ip->ip_hl, ip->ip_tos,
		ntohs(ip->ip_len), ntohs(ip->ip_id));
	printf("   %1x %04x", (ntohs(ip->ip_off) & 0xe000) >> 13,
		ntohs(ip->ip_off) & 0x1fff);
	printf("  %02x  %02x %04x", ip->ip_ttl, ip->ip_p, ntohs(ip->ip_sum));
	printf(" %s ", inet_ntoa(ip->ip_src));
	printf(" %s \n", inet_ntoa(ip->ip_dst));
	inner = bytes + (ip->ip_hl << 2);
	if (ip->ip_p == IPPROTO_ICMP)
	{
		printf("ICMP: type %u, code %u, size %u", inner[0], inner[1],
			ntohs(ip->ip_len) - (ip->ip_hl << 2));
		if (inner[0] == ICMP_ECHO || inner[0] == ICMP_ECHOREPLY)
			printf(", id 0x%04x, seq 0x%04x", inner[4] << 8 | inner[5],
				inner[6] << 8 | inner[7]);
		printf("\n");
	}
}

/*
 * An ICMP error carries the IP header + first bytes of the packet that
 * caused it. Without -v only errors about our own echo requests are shown,
 * with -v every error is shown along with a dump of the original packet.
 */
static void	print_icmp_error(t_ping *ping, struct icmp *icmp, size_t icmp_len,
				struct sockaddr_in *from)
{
	struct ip	*orig;
	size_t		orig_hlen;
	struct icmp	*orig_icmp;
	int			ours;

	if (icmp_len < ICMP_HDR_LEN + sizeof(struct ip))
		return ;
	orig = (struct ip *)((unsigned char *)icmp + ICMP_HDR_LEN);
	orig_hlen = orig->ip_hl << 2;
	if (orig_hlen < sizeof(struct ip)
		|| icmp_len < ICMP_HDR_LEN + orig_hlen + ICMP_HDR_LEN)
		return ;
	orig_icmp = (struct icmp *)((unsigned char *)orig + orig_hlen);
	ours = orig->ip_p == IPPROTO_ICMP
		&& orig->ip_dst.s_addr == ping->dst.sin_addr.s_addr
		&& orig_icmp->icmp_type == ICMP_ECHO
		&& ntohs(orig_icmp->icmp_id) == ping->id;
	if (!ours && !ping->verbose)
		return ;
	printf("%zu bytes from %s: %s\n", icmp_len, inet_ntoa(from->sin_addr),
		icmp_description(icmp->icmp_type, icmp->icmp_code));
	if (ping->verbose)
		dump_original(orig);
}

void	handle_packet(t_ping *ping, unsigned char *buf, ssize_t len,
			struct sockaddr_in *from)
{
	struct ip	*ip;
	size_t		hlen;
	struct icmp	*icmp;
	size_t		icmp_len;

	ip = (struct ip *)buf;
	hlen = ip->ip_hl << 2;
	if (len < (ssize_t)sizeof(struct ip) || (size_t)len < hlen + ICMP_HDR_LEN)
	{
		if (ping->verbose)
			fprintf(stderr, "ft_ping: packet too short (%zd bytes) from %s\n",
				len, inet_ntoa(from->sin_addr));
		return ;
	}
	icmp = (struct icmp *)(buf + hlen);
	icmp_len = (size_t)len - hlen;
	if (checksum(icmp, icmp_len) != 0)
	{
		if (ping->verbose)
			fprintf(stderr, "ft_ping: checksum mismatch from %s\n",
				inet_ntoa(from->sin_addr));
		return ;
	}
	if (icmp->icmp_type == ICMP_ECHOREPLY)
	{
		if (ntohs(icmp->icmp_id) == ping->id)
			print_echo_reply(ping, ip, icmp, icmp_len, from);
	}
	else if (icmp->icmp_type == ICMP_DEST_UNREACH
		|| icmp->icmp_type == ICMP_SOURCE_QUENCH
		|| icmp->icmp_type == ICMP_REDIRECT
		|| icmp->icmp_type == ICMP_TIME_EXCEEDED
		|| icmp->icmp_type == ICMP_PARAMPROB)
		print_icmp_error(ping, icmp, icmp_len, from);
}
