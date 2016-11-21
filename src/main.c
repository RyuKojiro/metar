#include <curl/curl.h>
#include <sysexits.h>

int main(int argc, const char * const argv[]) {
	curl_global_init(CURL_GLOBAL_DEFAULT);
	CURL *curl = curl_easy_init();
	if(!curl) {
		return EX_SOFTWARE;
	}

	// Do the work

	curl_easy_cleanup(curl);
	curl_global_cleanup();
	return EX_OK;
}
