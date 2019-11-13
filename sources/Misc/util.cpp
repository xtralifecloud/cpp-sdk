/*
 * util.c
 * Amazon SimpleDB C bindings
 *
 * Created by Peter Macko on 11/26/08.
 *
 * Copyright 2009
 *	  The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	notice, this list of conditions and the following disclaimer in the
 *	documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *	may be used to endorse or promote products derived from this software
 *	without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdlib.h>
#include "util.h"
#include "base64.h"
#include "helpers.h"

/**
 * Perform a Base64 encoding
 * 
 * @param input the input buffer
 * @param length the length of the input
 * @param output the output buffer
 * @param olength the length of the output buffer
 * @return the length of the encoded message, or 0 if the output buffer is too small
 */
size_t base64(const unsigned char *input, size_t length, char* output, size_t olength)
{
	return encode64((const char*) input, output, (int) length);
}


/**
 * Compute the number of digits of a number
 * 
 * @param num the number
 * @param base the base
 * @return the number of digits
 */
int digits(int num, int base)
{
	int d = 1;
	int n;
	for (n = num; n >= base; n /= base) d++;
	return d;
}

void make_basic_authentication_header(const char *username, const char *password, char *output, size_t maxChars) {
	char userpass[256], encodedUserpass[256];
	safe::sprintf(userpass, "%s:%s", username, password);
	if (base64((unsigned char*) userpass, strlen(userpass),
			   encodedUserpass, safe::charsIn(encodedUserpass)) > 0) {
		safe::strcpy(output, maxChars, "Basic ");
		safe::strcat(output, maxChars, encodedUserpass);
	} else {
		safe::strcpy(output, maxChars, "");
	}
}
