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

#include <curl/curl.h>
#include <sysexits.h> /* EX_USAGE, EX_OK */
#include <err.h>      /* errx, warnx */
#include <ctype.h>    /* toupper, isalnum */
#include <string.h>   /* strlen */
#include <stdbool.h>
#include <assert.h>

#define URL_PREFIX_TAF     "http://tgftp.nws.noaa.gov/data/forecasts/taf/stations/"
#define URL_PREFIX_DECODED "http://tgftp.nws.noaa.gov/data/observations/metar/decoded/"
#define URL_PREFIX_METAR   "http://tgftp.nws.noaa.gov/data/observations/metar/stations/"
#define URL_EXTENSION      ".TXT"

#define STATION_ID_LEN          (4)
#define DEFAULT_STATION_PREFIX  'K'
#define HTTP_RESPONSE_NOT_FOUND 404

/* This value must be larger than the largest possible URL produced. This is
 * easy to ensure by simply keeping the prefix sizeof (the first one) set to
 * the largest prefix possible. */
#define URL_BUFFER_LEN     (sizeof(URL_PREFIX_METAR) + STATION_ID_LEN + sizeof(URL_EXTENSION))

enum urlType {
	METAR,
	TAF,
	Decoded
};

static bool formURL(char *buf, size_t bufLen, enum urlType type, const char *station) {
	size_t stationLen = strlen(station);
	size_t i, written;

	/* Ensure the station is a valid length */
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
	for(i = 1; i <= stationLen; i++) {
		if(!isalnum(station[stationLen - i])) {
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

static size_t printData(void *contents, size_t size, size_t nmemb, void *userp) {
	(void)userp;
	printf("%s", contents);
	return size * nmemb;
}

static int __attribute__((noreturn)) usage(void) {
	errx(EX_USAGE, "usage: metar <station_id ...>");
}

int main(int argc, const char * const argv[]) {
	CURL *curl;
	CURLcode res;
	int arg;
	long response;
	char url[URL_BUFFER_LEN];

	if (argc < 2) {
		warnx("At least one argument is required");
		usage();
		return EX_USAGE;
	}

	curl = curl_easy_init();
	if (!curl) {
		return EX_SOFTWARE;
	}

	for (arg = 1; arg < argc; arg++) {
		if(!formURL(url, sizeof(url), METAR, argv[arg])) {
			continue;
		}

		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, printData);
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
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
	}

	curl_easy_cleanup(curl);
	curl_global_cleanup();
	return EX_OK;
}
