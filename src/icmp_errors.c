#include "../includes/ping.h"

bool is_icmp_error(int type)
{
	return (type == ICMP_DEST_UNREACH || type == ICMP_SOURCE_QUENCH
		|| type == ICMP_REDIRECT || type == ICMP_TIME_EXCEEDED
		|| type == ICMP_PARAMPROB);
}

static const char *icmp_description(int type, int code)
{
	static const char *unreach[] = {
		"Destination Net Unreachable", "Destination Host Unreachable",
		"Destination Protocol Unreachable", "Destination Port Unreachable",
		"Fragmentation needed and DF set", "Source Route Failed",
		"Destination Net Unknown", "Destination Host Unknown",
		"Source Host Isolated", "Destination Network Prohibited",
		"Destination Host Prohibited",
		"Destination Network Unreachable At This TOS",
		"Destination Host Unreachable At This TOS", "Packet Filtered",
		"Precedence Violation", "Precedence Cutoff"};
	static const char *redirect[] = {
		"Redirect Network", "Redirect Host",
		"Redirect Type of Service and Network",
		"Redirect Type of Service and Host"};

	if (type == ICMP_DEST_UNREACH && code >= 0 && code <= 15)
		return unreach[code];
	if (type == ICMP_REDIRECT && code >= 0 && code <= 3)
		return redirect[code];
	if (type == ICMP_TIME_EXCEEDED && code == 0)
		return "Time to live exceeded";
	if (type == ICMP_TIME_EXCEEDED && code == 1)
		return "Frag reassembly time exceeded";
	if (type == ICMP_SOURCE_QUENCH)
		return "Source Quench";
	if (type == ICMP_PARAMPROB)
		return "Parameter problem";
	return "Unknown ICMP type or code";
}

/*
 * Verbose dump of the original IP header and ICMP header carried
 * inside an ICMP error message (same layout as inetutils ping -v).
 */
static void dump_original(struct ip *ip)
{
	unsigned char *bytes = (unsigned char *)ip;
	unsigned char *inner = bytes + (ip->ip_hl << 2);

	printf("IP Hdr Dump:\n ");
	for (size_t i = 0; i < sizeof(struct ip); i++)
		printf("%02x%s", bytes[i], (i % 2) ? " " : "");
	printf("\nVr HL TOS  Len   ID Flg  off TTL Pro  cks      Src\tDst\tData\n");
	printf(" %1x  %1x  %02x %04x %04x", ip->ip_v, ip->ip_hl, ip->ip_tos,
		ntohs(ip->ip_len), ntohs(ip->ip_id));
	printf("   %1x %04x", (ntohs(ip->ip_off) & 0xe000) >> 13,
		ntohs(ip->ip_off) & 0x1fff);
	printf("  %02x  %02x %04x", ip->ip_ttl, ip->ip_p, ntohs(ip->ip_sum));
	printf(" %s ", inet_ntoa(ip->ip_src));
	printf(" %s \n", inet_ntoa(ip->ip_dst));
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
 * Returns true if the error is about one of our echo requests.
 */
bool print_icmp_error(struct icmp *icmp, int icmp_len, struct sockaddr_in *from,
	struct sockaddr_in *dst, bool verbose)
{
	struct ip *orig;
	struct icmp *orig_icmp;
	int orig_hlen;
	bool ours;

	if (icmp_len < 8 + (int)sizeof(struct ip))
		return false;
	orig = (struct ip *)((unsigned char *)icmp + 8);
	orig_hlen = orig->ip_hl << 2;
	if (orig_hlen < (int)sizeof(struct ip) || icmp_len < 8 + orig_hlen + 8)
		return false;
	orig_icmp = (struct icmp *)((unsigned char *)orig + orig_hlen);
	ours = orig->ip_p == IPPROTO_ICMP
		&& orig->ip_dst.s_addr == dst->sin_addr.s_addr
		&& orig_icmp->icmp_type == ICMP_ECHO
		&& ntohs(orig_icmp->icmp_id) == (getpid() & 0xFFFF);
	if (!ours && !verbose)
		return false;
	printf("%d bytes from %s: %s\n", icmp_len, inet_ntoa(from->sin_addr),
		icmp_description(icmp->icmp_type, icmp->icmp_code));
	if (verbose)
		dump_original(orig);
	return ours;
}
