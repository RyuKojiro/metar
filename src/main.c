/*
 * Copyright (c) 2016 Daniel Loffgren
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <assert.h>
#include <ctype.h>    /* toupper, isalnum */
#include <err.h>      /* errx, warnx */
#include <stdbool.h>
#include <stdlib.h>   /* exit */
#include <stdio.h>    /* printf, fprintf */
#include <string.h>   /* strlen */
#include <sysexits.h> /* EX_USAGE, EX_OK */
#include <unistd.h>   /* getopt */
#include <curl/curl.h>

#define URL_PREFIX_TAF     "http://tgftp.nws.noaa.gov/data/forecasts/taf/stations/"
#define URL_PREFIX_DECODED "http://tgftp.nws.noaa.gov/data/observations/metar/decoded/"
#define URL_PREFIX_METAR   "http://tgftp.nws.noaa.gov/data/observations/metar/stations/"
#define URL_EXTENSION      ".TXT"

#define STATION_ID_LEN          (4)
#define DEFAULT_STATION_PREFIX  'K' /* TODO: This should be localizable */
#define HTTP_RESPONSE_NOT_FOUND 404

/* The URL_BUFFER_LEN must be large enough to fit the largest producible URL.
 * This is easy to ensure by simply keeping the LONGEST_URL_PREFIX set to the
 * largest prefix string. */
#define LONGEST_URL_PREFIX URL_PREFIX_METAR
#define URL_BUFFER_LEN     (sizeof(LONGEST_URL_PREFIX) + STATION_ID_LEN + sizeof(URL_EXTENSION))

enum urlType {
	METAR,
	TAF,
	Decoded
};

/* Function Prototypes */

static bool
formURL(char *buf, size_t bufLen, enum urlType type, const char *station);

static size_t
printData(void *contents, size_t size, size_t nmemb, void *userp);

static int __attribute__((noreturn))
usage(void);

int
main(int argc, char * const argv[]);

/*
 * Create a URL from parts, namely the urlType and the station, and store
 * the result in the specified buffer.
 */
static bool
formURL(char *buf, size_t bufLen, enum urlType type, const char *station) {
	size_t stationLen, i, written;

	/* Ensure the station is a valid length */
	stationLen = strlen(station);
	if(stationLen != STATION_ID_LEN &&
	   stationLen != STATION_ID_LEN - 1) {
		warnx("Station ID must be either three or four characters long.");
		return false;
	}

	/* Copy the first part of the URL */
	switch (type) {
		case METAR: {
			strncpy(buf, URL_PREFIX_METAR, bufLen);
			written = sizeof(URL_PREFIX_METAR);
		} break;
		case TAF: {
			strncpy(buf, URL_PREFIX_TAF, bufLen);
			written = sizeof(URL_PREFIX_TAF);
		} break;
		case Decoded: {
			strncpy(buf, URL_PREFIX_DECODED, bufLen);
			written = sizeof(URL_PREFIX_DECODED);
		} break;
		default: {
			return false;
		}
	}

	/* Subtract one for the null terminator */
	written--;

	/* Transfer the station id from end to beginning */
	for (i = 1; i <= stationLen; i++) {
		if (!isalnum(station[stationLen - i])) {
			warnx("Station ID must contain only alphanumeric characters.");
			return false;
		}
		buf[(written + STATION_ID_LEN) - i] = (char)toupper(station[stationLen - i]);
	}

	/* If it is a character short, prepend the 'K' */
	if (stationLen == STATION_ID_LEN - 1) {
		buf[written] = DEFAULT_STATION_PREFIX;
	}
	written += STATION_ID_LEN;

	/* Append the extension. If the following assertion fails, then we were
	 * handed a buffer that was too short! */
	assert(bufLen >= written + sizeof(URL_EXTENSION));
	strncpy(buf + written, URL_EXTENSION, sizeof(URL_EXTENSION));

	return true;
}

/*
 * This is a libcurl callback that simply dumps the data received, byte-for-byte.
 */
static size_t
printData(void *contents, size_t size, size_t nmemb, void *userp) {
	(void)userp;

	/* NOAA automated information always has a newline at the end, as is required
	 * to be valid POSIX text. */
	printf("%s", (const char *)contents);
	return size * nmemb;
}

static int __attribute__((noreturn))
usage(void) {
	fprintf(stderr, "usage: metar [-dt] station_id [...]\n"
	                "\t-d Show decoded METAR output\n"
	                "\t-t Show TAFs where available\n"
	);
	exit(EX_USAGE);
}

int
main(int argc, char * const argv[]) {
	CURL *curl;
	CURLcode res;
	int arg;
	long response;
	char url[URL_BUFFER_LEN];
	int ch;
	bool decoded = false;
	bool tafs = false;

	while ((ch = getopt(argc, argv, "dt")) != -1) {
		switch (ch) {
			case 'd': {
				decoded = true;
			} break;
			case 't': {
				tafs = true;
			} break;
			case '?':
			default: {
				usage();
				/* NOTREACHED */
				return EX_USAGE;
			}
		}
	}
	argc -= optind;
	argv += optind;

	if (argc < 1) {
		warnx("At least one argument is required");
		usage();
		/* NOTREACHED */
		return EX_USAGE;
	}

	curl = curl_easy_init();
	if (!curl) {
		return EX_SOFTWARE;
	}

	/* Only set up the things that will be the same for every request once. */
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, printData);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);

	for (arg = 0; arg < argc; arg++) {
		if (!formURL(url, sizeof(url), decoded ? Decoded : METAR, argv[arg])) {
			continue;
		}

		curl_easy_setopt(curl, CURLOPT_URL, url);
		res = curl_easy_perform(curl);

		/* This is resilient to both FTP and HTTP */
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response);
		if (res == CURLE_REMOTE_FILE_NOT_FOUND || response == HTTP_RESPONSE_NOT_FOUND) {
			warnx("Station ID \"%s\" not found", argv[arg]);
			continue;
		}

		if (res != CURLE_OK) {
			warnx("%s", curl_easy_strerror(res));
			warnx("Unable to fetch information for station ID \"%s\"", argv[arg]);
			continue;
		}

		/* If -t was specified, attempt to fetch TAF, failing silently */
		if (tafs) {
			if(!formURL(url, sizeof(url), TAF, argv[arg])) {
				continue;
			}

			curl_easy_setopt(curl, CURLOPT_URL, url);
			curl_easy_perform(curl);
		}
	}

	curl_easy_cleanup(curl);
	curl_global_cleanup();
	return EX_OK;
}
