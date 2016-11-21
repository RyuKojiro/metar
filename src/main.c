#include <curl/curl.h>
#include <sysexits.h> // EX_USAGE, EX_OK
#include <err.h>      // errx, warnx
#include <ctype.h>    // toupper, isalnum
#include <assert.h>   // assert

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


int main(int argc, const char * const argv[]) {
	union url url = {"ftp://tgftp.nws.noaa.gov/data/observations/metar/stations/XXXX.TXT"};
	if (argc < 2) {
		warnx("At least one argument is required");
		return usage();
	}

	CURL *curl = curl_easy_init();
	if(!curl) {
		return EX_SOFTWARE;
	}

	// Do the work

	curl_easy_cleanup(curl);
	curl_global_cleanup();
	return EX_OK;
}
