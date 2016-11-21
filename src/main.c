#include <curl/curl.h>
#include <sysexits.h>
#include <string.h>
#include <err.h>

static int usage(void) {
	errx(EX_USAGE, "usage: metar <station_id ...>");

	// Never reached
	return EX_USAGE;
}

int main(int argc, const char * const argv[]) {
	union url {
		char entirety[67];
		struct parts {
			char fiddlybits[58];
			char station[4];
			char extension[4];
			char terminator;
		} parts;
	} url = {"ftp://tgftp.nws.noaa.gov/data/observations/metar/stations/XXXX.TXT"};

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
