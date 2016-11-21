#include <curl/curl.h>
#include <sysexits.h>
#include <string.h>

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

	CURL *curl = curl_easy_init();
	if(!curl) {
		return EX_SOFTWARE;
	}

	// Do the work

	curl_easy_cleanup(curl);
	curl_global_cleanup();
	return EX_OK;
}
