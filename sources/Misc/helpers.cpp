#include "CCallback.h"
#include "Misc/helpers.h"

using namespace XtraLife;
using namespace XtraLife::Helpers;

cstring::cstring(const char *other) : buffer(NULL) {
	*this = other;
}

cstring::cstring(char *other, bool takeOwnership) : buffer(NULL) {
	if (takeOwnership) { *this <<= other; }
	else { *this = other; }
}

cstring::cstring(const cstring &other) : buffer(NULL) {
	*this = other;
}

/*cstring *cstring::cstring(const char *fomat, ...) {
	cstring *res = new cstring();
	return res;
}*/

cstring::cstring(cstring &&other) {
	buffer = other.buffer;
	other.buffer = NULL;
}

cstring& cstring::operator=(const char *other) {
	if (buffer) { free(buffer); buffer = NULL; }
	if (other) { buffer = strdup(other); }
	return *this;
}

cstring& cstring::operator=(cstring &other) {
	*this = other.buffer; return *this;
}

cstring& cstring::operator=(cstring &&other) {
	*this = NULL; buffer = other.buffer; other.buffer = NULL; return *this;
}

cstring::~cstring() {
	if (buffer) { free(buffer); buffer = NULL; }
}

cstring& cstring::operator<<=(char *other) {
	*this = NULL; buffer = other;
	return *this;
}

bool cstring::IsEqual(const char *other) const {
	return other != NULL && strcmp(buffer, other) == 0;
}

char *cstring::DetachOwnership() {
	char *result = buffer;
	buffer = NULL;
	return result;
}

cstring& XtraLife::Helpers::csprintf(cstring &dest, const char *format, ...) {
	va_list args;
	va_start (args, format);
	size_t requiredChars = vsnprintf(NULL, 0, format, args);
	va_start (args, format);
	char *buffer = new char[requiredChars + 1];
	vsnprintf(buffer, requiredChars+1, format, args);
	buffer[requiredChars] = 0;
	return dest <<= buffer;
}

cstring XtraLife::Helpers::csprintf(const char *format, ...) {
	va_list args;
	va_start (args, format);
	size_t requiredChars = vsnprintf(NULL, 0, format, args);
	va_start (args, format);
	char *buffer = new char[requiredChars + 1];
	vsnprintf(buffer, requiredChars+1, format, args);
	buffer[requiredChars] = 0;
	return cstring(buffer, true);
}

char *XtraLife::print_current_time(char *dest, size_t dest_size, const char *format) {
	time_t now = time(0);
	struct tm tstruct = *localtime(&now);
	strftime(dest, dest_size, format, &tstruct);
	return dest;
}

bool XtraLife::IsEqual(const char *str1, const char *str2) {
	return str1 == str2 || (str1 && str2 && !strcmp(str1, str2));
}

void safe::strcpy(char *dest, size_t max_size, const char *source) {
	while (*source && max_size-- > 1) {
		*dest++ = *source++;
	}
	*dest = '\0';
}

void safe::strcat(char *dest, size_t max_size, const char *source) {
	while (*dest && max_size > 1) {
		dest++, max_size--;
	}
	while (*source && max_size-- > 1) {
		*dest++ = *source++;
	}
	*dest = '\0';
}

void safe::sprintf(char *dest, size_t max_size, const char *format, ...) {
	va_list args;
	va_start (args, format);
	vsnprintf(dest, max_size, format, args);
	dest[max_size - 1] = '\0';
	va_end (args);
}

void safe::replace_string(char *o_string, size_t maxSize, const char *s_string, const char *r_string) {
	// to store the pointer returned from strstr
	char *ch = strstr(o_string, s_string);
	if (ch) {
		// a buffer variable to do all replace things
		char *buffer = (char*) malloc(strlen(o_string) + strlen(r_string) + 1);
		// copy all the content to buffer before the first occurrence of the search string
		strncpy(buffer, o_string, ch-o_string);

		// prepare the buffer for appending by adding a null to the end of it
		buffer[ch-o_string] = 0;

		// no need for safe functions here, we know the buffer is large enough
		::strcpy(buffer+(ch - o_string), r_string);
		::strcat(buffer+(ch - o_string), ch + strlen(s_string));

		// empty o_string for copying
		o_string[0] = 0;
		safe::strcpy(o_string, maxSize, buffer);
		free(buffer);

		// pass recursively to replace other occurrences
		replace_string(o_string, maxSize, s_string, r_string);
	}
}

#include "CClannishRESTProxy.h"

void DownloadDataURL(const char* aURL, CResultHandler* aHandler);
void DownloadDataURL(const char* aURL, CResultHandler* aHandler)
{
    CClannishRESTProxy::Instance()->DownloadData(aURL, MakeBridgeDelegate(aHandler));
}
