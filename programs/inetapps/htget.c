/*
 *	HTGET
 *
 *	Get a document via HTTP
 *
 *	This program was compiled under Borland C++ 3.1 in C mode for the small
 *	model. You will require the WATTCP libraries. A copy is included in the
 *	distribution. For the sources of WATTCP, ask archie where the archives
 *	are.
 *
 *	Please send bug fixes, ports and enhancements to the author for
 *	incorporation in newer versions.
 *
 *	Copyright (C) 1997  Ken Yap (ken@syd.dit.csiro.au)
 *
 *	This program is free software; you can redistribute it and/or modify it
 *	under the terms of the Artistic License, a copy of which is included
 *	with this distribution.
 *
 *	THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR
 *	IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 *	WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#define	BUFFERSIZE	2048

#define	PROGRAM		"HTGET"
#define	VERSION		"1.02"
#define	AUTHOR		"Copyright (C) 1998 Ken Yap (ken@syd.dit.csiro.au)"

#define	HTTPVER		"HTTP/1.[01]"
#define	HTTPVER10	"HTTP/1.0"
#define	HTTPVER11	"HTTP/1.1"

#define	strn(s)		s, sizeof(s)-1

int		output;
int		headeronly = 0;
int		verbose = 0;
int		ifmodifiedsince = 0;
char		*outputfile = 0;
FILE		*of;
char		*password = 0;
char		basis_64[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
char		buffer[BUFFERSIZE];
struct stat	statbuf;
struct tm	*mtime;
char		*dayname[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
char		*monthname[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
			"Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
FILE *sock;

void base64encode(char *in, char *out)
{
	int		c1, c2, c3;

	while (*in != '\0')
	{
		c1 = *in++;
		if (*in == '\0')
			c2 = c3 = 0;
		else
		{
			c2 = *in++;
			if (*in == '\0')
				c3 = 0;
			else
				c3 = *in++;
		}
		*out++ = basis_64[c1>>2];
		*out++ = basis_64[((c1 & 0x3) << 4) | ((c2 & 0xF0) >> 4)];
		if (c2 == 0 && c3 == 0)
		{
			*out++ = '=';
			*out++ = '=';
		}
		else if (c3 == 0)
		{
			*out++ = basis_64[((c2 & 0xF) << 2) | ((c3 & 0xC0) >> 6)];
			*out++ = '=';
		}
		else
		{
			*out++ = basis_64[((c2 & 0xF) << 2) | ((c3 & 0xC0) >> 6)];
			*out++ = basis_64[c3 & 0x3F];
		}
	}
	*out = '\0';
}

/* WATTCP's sock_gets doesn't do the right thing */
int sock_getline(FILE *sock, char *buffer, int len)
{
	int		i;
	char		ch;

	for (i = 0, --len; i <= len && (ch = fgetc(sock)) != EOF; )
	{
		if (ch == '\n')
			break;
		else if (ch != '\r')
		{
			*buffer++ = ch;
			++i;
		}
	}
	*buffer = '\0';
	return (i);
}

long header(char *path)
{
	int		i, len, response;
	long		contentlength;
	char		*s;

	fflush(sock);
	contentlength = 0x7fffffffL;	/* largest int, in case no CL header */
	if ((len = sock_getline(sock, buffer, sizeof(buffer))) <= 0)
	{
		(void)fprintf(stderr, "EOF from server\n");
		return (-1L);
	}
	if (strncmp(s = buffer, strn(HTTPVER10)) != 0 &&
		strncmp(s, strn(HTTPVER11)) != 0)	/* no HTTP/1.[01]? */
	{
		(void)fprintf(stderr, "Not " HTTPVER " server\n");
		return (-1L);
	}
	s += sizeof(HTTPVER10)-1;		/* either 10 or 11 will do */
	if ((i = strspn(s, " \t")) <= 0)	/* no whitespace? */
	{
		(void)fprintf(stderr, "Malformed " HTTPVER " line\n");
		return (-1L);
	}
	s += i;
	response = 500;
	(void)sscanf(s, "%3d", &response);
	if (response == 401)
	{
		(void)fprintf(stderr, "%s: authorisation required, use -p ident:password\n", path);
		return (-1L);
	}
	else if (response != 200 && response != 301 && response != 302
		&& response != 304)
	{
		(void)fprintf(stderr, "%s: %s\n", path, s);
		contentlength = -1L;
	}
	if (headeronly)
	{
		(void)write(output, buffer, len);
		/* headers are to be read, so we write a DOS CR-NL */
		(void)write(output, "\n", 1);
	}
	/* eat up the other header lines here */
	while ((len = sock_getline(sock, buffer, sizeof(buffer))) > 0)
	{
		if (headeronly)
		{
			(void)write(output, buffer, len);
			(void)write(output, "\n", 1);
		}
		if (strncasecmp(s = buffer, strn("Content-Length:")) == 0)
		{
			s += sizeof("Content-Length:")-1;
			contentlength = atol(s);
		}
		else if (strncasecmp(buffer, strn("Location:")) == 0)
		{
			if (response == 301 || response == 302)
				fprintf(stderr, "At %s\n", buffer);
		}
		else if (strchr(" \t", buffer[0]) != 0)
			fprintf(stderr, "Warning: continuation line encountered\n");
	}
	return (response == 304 ? 0L : contentlength);
}

char conbuf[100];

int htget(char *host, int port, char *path)
{
	int		status = 0;
	int		connected = 0;
	int		completed = 0;
	int		i;
	long		length, contentlength;

#ifdef	DEBUG
	(void)fprintf(stderr, PROGRAM ": %s:%d%s\n", host, port, path);
#endif
	if (verbose)
		(void)fprintf(stderr, "Contacting %s\n", host);
	sprintf(conbuf, "/dev/tcp/%s:%u", host, port);
	sock = fopen(conbuf, "w+");
	if (!sock) {
		perror("Unable to connect");
		exit(1);
	}
	connected = 1;
	completed = 1;
	if (verbose)
		(void)fprintf(stderr, "Sending HTTP GET/HEAD request\n");
	fprintf(sock,
		"%s %s " HTTPVER10 "\r\n"
		"User-Agent: " PROGRAM "-WiNGS/" VERSION "\r\n",
		headeronly ? "HEAD" : "GET",
		path);
#ifdef	DEBUG
	fprintf(stderr,
		"%s %s " HTTPVER10 "\r\n"
		"User-Agent: " PROGRAM "-WiNGS/" VERSION "\r\n",
		headeronly ? "HEAD" : "GET",
		path);
#endif
	fprintf(sock, "Host: %s\r\n", host);
	if (password != 0)
	{
		base64encode(password, buffer);
#ifdef	DEBUG
		(void)fprintf(stderr, "%s => %s\n", password, buffer);
#endif
		fprintf(sock, "Authorization: Basic %s\r\n", buffer);
	}
	if (ifmodifiedsince != 0)
	{
		fprintf(sock, "If-Modified-Since: %s, %02d %s %04d %02d:%02d:%02d GMT\r\n",
			dayname[mtime->tm_wday], mtime->tm_mday,
			monthname[mtime->tm_mon], mtime->tm_year + 1900,
			mtime->tm_hour, mtime->tm_min, mtime->tm_sec);
#ifdef	DEBUG
		(void)fprintf(stderr, "If-Modified-Since: %s, %02d %s %04d %02d:%02d:%02d GMT\n",
			dayname[mtime->tm_wday], mtime->tm_mday,
			monthname[mtime->tm_mon], mtime->tm_year + 1900,
			mtime->tm_hour, mtime->tm_min, mtime->tm_sec);
#endif
	}
	fprintf(sock, "\r\n");
	if ((contentlength = header(path)) > 0L && !headeronly)
	{
		/*
		   we wait until the last moment to open the output file
		   if any specified so that we won't overwrite the file
		   in case of error in contacting server
		*/
		if (outputfile != 0)
		{
			if ((of = fopen(outputfile, "w")) == NULL)
			{
				perror(outputfile);
				goto close_up;
			}
			output = fileno(of);
		}
		length = 0L;
		while ((i = fread(buffer, 1, sizeof(buffer), sock)) > 0)
		{
			(void)write(output, buffer, i);
			length += i;
			if (verbose && contentlength != 0x7fffffffL)
				(void)fprintf(stderr, "Received %ld/%ld\r",
					length, contentlength);
		}
		if (verbose)
			(void)fprintf(stderr, "\n");
		if (contentlength != 0x7fffffffL && length != contentlength)
			(void)fprintf(stderr, PROGRAM ": Warning, actual length = %ld, content length = %ld\n",
				length, contentlength);
	}
close_up:
	fclose(sock);
sock_err:
	if (status == -1)
		(void)fprintf(stderr, PROGRAM ": %s reset connection\n", host);
	if (!connected)
		(void)fprintf(stderr, PROGRAM ": Could not get connected\n");
	/* 0 if nothing went wrong */
	return(!completed || contentlength < 0L);
}

void usage(void)
{
	(void)fprintf(stderr, PROGRAM " " VERSION " " AUTHOR "\n");
	(void)fprintf(stderr, PROGRAM " comes with ABSOLUTELY NO WARRANTY.\n");
	(void)fprintf(stderr, "This is Postcardware, and you are welcome to redistribute it\n");
	(void)fprintf(stderr, " under certain conditions; see the file Artistic for details.\n");
	(void)fprintf(stderr, "Usage: " PROGRAM " [-h] [-m] [-o file] [-p ident:password] [-v] URL\n");
	(void)fprintf(stderr, " -h  get header only\n");
	(void)fprintf(stderr, " -m  fetch only if newer than file in -o\n");
	(void)fprintf(stderr, " -o file  save output in file\n");
	(void)fprintf(stderr, " -p ident:password  send authorisation\n");
	(void)fprintf(stderr, " -v  show some progress messages\n");
}

int main(int argc, char **argv)
{
	char		*host, *path, *s, *proxy;
	int		i, port = 80;
	int		status;
	extern int	optind;
	extern char	*optarg;

	while ((i = getopt(argc, argv, "hmo:p:v")) >= 0)
	{
		switch (i)
		{
		case 'h':
			headeronly = 1;
			break;
		case 'm':
			ifmodifiedsince = 1;
			break;
		case 'o':
			outputfile = optarg;
			break;
		case 'p':
			password = optarg;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			usage();
			return (1);
		}
	}
	argc -= optind;
	argv += optind;
	if (argc <= 0)
	{
		usage();
		return (1);
	}
	if((proxy = getenv("HTTP_PROXY")) == 0)
	{
		if (strncasecmp(argv[0], strn("http://")) == 0)
			argv[0] += sizeof("http://")-1;
		/* separate out the path component */
		if ((path = strchr(argv[0], '/')) == NULL)
		{
			host = argv[0];
			path = "/";	     /* top directory */
		}
		else
		{
			if ((host = malloc(path - argv[0] + 1)) == NULL)
			{
				(void)fprintf(stderr, PROGRAM ": Out of memory\n");
				return (1);
			}
			host[0] = '\0';
			strncat(host, argv[0], path - argv[0]);
		}
	}
	else
	{
		host = proxy;
		path = argv[0];
	}
	/* do we have a port number? */
	if ((s = strchr(host, ':')) != NULL)
	{
		*s++ = '\0';
		port = atoi(s);
	}
	if (ifmodifiedsince != 0)
	{
		/* allow only if no -h and -o file specified and file exists */
		if (headeronly || outputfile == 0 || stat(outputfile, &statbuf) < 0)
			ifmodifiedsince = 0;
		else if (verbose)
		{
			mtime = gmtime(&statbuf.st_mtime);
			(void)fprintf(stderr, "%s last modified %s",
				outputfile, asctime(mtime));
		}
	}
	/* ensure no mangling of \n in binary documents */
	output = fileno(stdout);
	status = htget(host, port, path);
	close(output);
	return (status);
}
