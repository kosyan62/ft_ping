/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   options.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kosyan62 <kosyan62@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/23 12:00:00 by kosyan62          #+#    #+#             */
/*   Updated: 2026/10/23 12:00:00 by kosyan62         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ping.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <getopt.h>

#define OPT_TTL		256
#define OPT_HELP	257

static void	print_help(void)
{
	printf("Usage: ft_ping [OPTION...] HOST\n"
		"Send ICMP ECHO_REQUEST packets to network hosts.\n"
		"\n"
		"  -c, --count=NUMBER         stop after sending NUMBER packets\n"
		"  -s, --size=NUMBER          send NUMBER data octets\n"
		"      --ttl=N                specify N as time-to-live\n"
		"  -v, --verbose              verbose output\n"
		"  -w, --timeout=N            stop after N seconds\n"
		"  -W, --linger=N             number of seconds to wait for response\n"
		"  -?, --help                 give this help list\n");
}

static void	print_try_help(void)
{
	fprintf(stderr, "Try 'ft_ping --help' for more information.\n");
}

/*
 * Converts an option argument to a number the same way inetutils does:
 * the whole string must be a number in [allow_zero ? 0 : 1, max].
 */
static int	parse_number(const char *arg, unsigned long max, int allow_zero,
				long *out)
{
	char			*end;
	unsigned long	n;

	errno = 0;
	n = strtoul(arg, &end, 0);
	if (*arg == '\0' || *arg == '-' || *end != '\0')
	{
		fprintf(stderr, "ft_ping: invalid value (`%s' near `%s')\n",
			arg, *end ? end : arg);
		return (-1);
	}
	if (n == 0 && !allow_zero)
	{
		fprintf(stderr, "ft_ping: option value too small: %s\n", arg);
		return (-1);
	}
	if (errno == ERANGE || n > max)
	{
		fprintf(stderr, "ft_ping: option value too big: %s\n", arg);
		return (-1);
	}
	*out = (long)n;
	return (0);
}

static int	apply_option(t_ping *ping, int opt, const char *arg)
{
	long	n;

	if (opt == 'v')
		ping->verbose = 1;
	else if (opt == 'c')
		return (parse_number(arg, LONG_MAX, 1, &ping->count));
	else if (opt == 'w')
		return (parse_number(arg, INT_MAX, 0, &ping->deadline));
	else if (opt == 'W')
		return (parse_number(arg, INT_MAX, 0, &ping->linger));
	else if (opt == 's' || opt == OPT_TTL)
	{
		if (parse_number(arg, opt == 's' ? MAX_DATALEN : 255,
				opt == 's', &n) < 0)
			return (-1);
		if (opt == 's')
			ping->datalen = (size_t)n;
		else
			ping->ttl = (int)n;
	}
	return (0);
}

/*
 * getopt_long() returns '?' both for "-?" and for an invalid option.
 * On an invalid short option optopt holds the offending character,
 * on an invalid long option the current word starts with "--".
 * Returns the exit code of the program.
 */
static int	handle_question(char **argv)
{
	if (optopt != 0 || strncmp(argv[optind - 1], "--", 2) == 0)
	{
		print_try_help();
		return (EXIT_USAGE);
	}
	print_help();
	return (EXIT_SUCCESS);
}

/*
 * Returns -1 if the program has to stop, with the exit code in *status.
 */
static int	parse_options(t_ping *ping, int argc, char **argv, int *status)
{
	static const struct option	long_opts[] = {
	{"count", required_argument, 0, 'c'},
	{"size", required_argument, 0, 's'},
	{"ttl", required_argument, 0, OPT_TTL},
	{"verbose", no_argument, 0, 'v'},
	{"timeout", required_argument, 0, 'w'},
	{"linger", required_argument, 0, 'W'},
	{"help", no_argument, 0, OPT_HELP},
	{0, 0, 0, 0}
	};
	int							opt;

	/* getopt prints its errors with argv[0], keep them short */
	argv[0] = "ft_ping";
	while (1)
	{
		optopt = 0;
		opt = getopt_long(argc, argv, "vc:s:w:W:?", long_opts, NULL);
		if (opt == -1)
			return (0);
		*status = EXIT_SUCCESS;
		if (opt == OPT_HELP)
			print_help();
		else if (opt == '?')
			*status = handle_question(argv);
		else if (apply_option(ping, opt, optarg) == 0)
			continue ;
		else
			*status = EXIT_FAILURE;
		return (-1);
	}
}

/*
 * Returns -1 if the program has to stop, with the exit code in *status.
 */
int	parse_args(t_ping *ping, int argc, char **argv, int *status)
{
	if (parse_options(ping, argc, argv, status) < 0)
		return (-1);
	*status = EXIT_USAGE;
	if (optind >= argc)
		fprintf(stderr, "ft_ping: missing host operand\n");
	else if (optind + 1 < argc)
		fprintf(stderr, "ft_ping: only one host is supported\n");
	else
	{
		ping->host = argv[optind];
		return (0);
	}
	print_try_help();
	return (-1);
}
