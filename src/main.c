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

/* For TAFs:            "http://tgftp.nws.noaa.gov/data/forecasts/taf/stations/" */
#define URL_FIDDLYBITS  "http://tgftp.nws.noaa.gov/data/observations/metar/stations/"
#define URL_STATION     "XXXX"
#define URL_EXTENSION   ".TXT"

#define URL_TEMPLATE    URL_FIDDLYBITS URL_STATION URL_EXTENSION

/* This unholy union achieves the magic of a type that can be constructed using
 * a compound initializer with a string literal inside, but have the template
 * section easily modifiable at runtime. It is unfornately statically sized to
 * match only one template string at a time.
 */
union url {
	char entirety[sizeof(URL_TEMPLATE)];
	struct parts {
		char fiddlybits[sizeof(URL_FIDDLYBITS) - 1];
		char station   [sizeof(URL_STATION)    - 1];
		char extension [sizeof(URL_EXTENSION)  - 1];
		char terminator;
	} parts;
};

#define STATION_ID_LEN          (sizeof(URL_STATION) - 1)
#define DEFAULT_STATION_PREFIX  'K'
#define HTTP_RESPONSE_NOT_FOUND 404

static bool setStation(union url *url, const char *station) {
	size_t len = strlen(station);
	size_t i;

	if(len != STATION_ID_LEN &&
	   len != STATION_ID_LEN - 1) {
		warnx("Station ID must be either three or four characters long.");
		return false;
	}

	/* Transfer the station id from end to beginning */
	for(i = 1; i <= len; i++) {
		if(!isalnum(station[len - i])) {
			warnx("Station ID must contain only alphanumeric characters.");
			return false;
		}
		url->parts.station[STATION_ID_LEN - i] = (char)toupper(station[len - i]);
	}

	/* If it is a character short, prepend the 'K' */
	if (len == STATION_ID_LEN - 1) {
		url->parts.station[0] = DEFAULT_STATION_PREFIX;
	}
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

	union url url = {URL_TEMPLATE};

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
		if(!setStation(&url, argv[arg])) {
			continue;
		}

		curl_easy_setopt(curl, CURLOPT_URL, url.entirety);
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
