#include <curl/curl.h>
#include <sysexits.h> // EX_USAGE, EX_OK
#include <err.h>      // errx, warnx
#include <ctype.h>    // toupper, isalnum
#include <assert.h>   // assert

#define URL_TEMPLATE "ftp://tgftp.nws.noaa.gov/data/observations/metar/stations/XXXX.TXT"

union url {
	char entirety[67];
	struct parts {
		char fiddlybits[58];
		char station[4];
		char extension[4];
		char terminator;
	} parts;
};

static int usage(void) {
	errx(EX_USAGE, "usage: metar <station_id ...>");

	// Never reached
	return EX_USAGE;
}

static void setStation(union url *url, const char *station) {
	for(int i = 0; i < 4; i++) {
		assert(isalnum(station[i]));
		url->parts.station[i] = toupper(station[i]);
	}
}

static size_t printData(void *contents, size_t size, size_t nmemb, void *userp) {
	printf("%s", contents);
	return size * nmemb;
}

int main(int argc, const char * const argv[]) {
	union url url = {URL_TEMPLATE};
	if (argc < 2) {
		warnx("At least one argument is required");
		return usage();
	}

	CURL *curl = curl_easy_init();
	if (!curl) {
		return EX_SOFTWARE;
	}

	// Do the work
	for (int arg = 1; arg < argc; arg++) {
		setStation(&url, argv[arg]);
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
