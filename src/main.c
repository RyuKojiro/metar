#include <curl/curl.h>
#include <sysexits.h> // EX_USAGE, EX_OK
#include <err.h>      // errx, warnx
#include <ctype.h>    // toupper, isalnum
#include <string.h>   // strlen
#include <stdbool.h>

#define URL_FIDDLYBITS  "ftp://tgftp.nws.noaa.gov/data/observations/metar/stations/"
#define URL_STATION		"XXXX"
#define URL_EXTENSION   ".TXT"

#define URL_TEMPLATE    URL_FIDDLYBITS URL_STATION URL_EXTENSION
#define STATION_ID_LEN  4

union url {
	char entirety[sizeof(URL_TEMPLATE)];
	struct parts {
		char fiddlybits[sizeof(URL_FIDDLYBITS) - 1];
		char station   [sizeof(URL_STATION)    - 1];
		char extension [sizeof(URL_EXTENSION)  - 1];
		char terminator;
	} parts;
};

static int __attribute__((noreturn)) usage(void) {
	errx(EX_USAGE, "usage: metar <station_id ...>");
}

static bool setStation(union url *url, const char *station) {
	size_t len = strlen(station);
	if(len != STATION_ID_LEN &&
	   len != STATION_ID_LEN - 1) {
		warnx("Station ID must be either three or four characters long.");
		return false;
	}

	// Transfer the station id from end to beginning
	for(size_t i = 1; i <= len; i++) {
		if(!isalnum(station[len - i])) {
			warnx("Station ID must contain only alphanumeric characters.");
			return false;
		}
		url->parts.station[STATION_ID_LEN - i] = (char)toupper(station[len - i]);
	}

	// If it was only 3 characters, prepend the 'K'
	if (len == 3) {
		url->parts.station[0] = 'K';
	}
	return true;
}

static size_t printData(void *contents, size_t size, size_t nmemb, void *userp) {
	(void)userp;
	printf("%s", contents);
	return size * nmemb;
}

int main(int argc, const char * const argv[]) {
	union url url = {URL_TEMPLATE};
	if (argc < 2) {
		warnx("At least one argument is required");
		usage();
		return EX_USAGE;
	}

	CURL *curl = curl_easy_init();
	if (!curl) {
		return EX_SOFTWARE;
	}

	// Do the work
	for (int arg = 1; arg < argc; arg++) {
		if(!setStation(&url, argv[arg])) {
			continue;
		}

		curl_easy_setopt(curl, CURLOPT_URL, url.entirety);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, printData);
		CURLcode res = curl_easy_perform(curl);

		if (res != CURLE_OK) {
			warnx("Unable to fetch information for station ID \"%s\"", argv[arg]);
			continue;
		}
	}

	curl_easy_cleanup(curl);
	curl_global_cleanup();
	return EX_OK;
}
