/* 
 * MBDyn (C) is a multibody analysis code. 
 * http://www.mbdyn.org
 *
 * Copyright (C) 1996-2004
 *
 * Pierangelo Masarati	<masarati@aero.polimi.it>
 * Paolo Mantegazza	<mantegazza@aero.polimi.it>
 *
 * Dipartimento di Ingegneria Aerospaziale - Politecnico di Milano
 * via La Masa, 34 - 20156 Milano, Italy
 * http://www.aero.polimi.it
 *
 * Changing this copyright notice is forbidden.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation (version 2 of the License).
 * 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <mbconfig.h>           /* This goes first in every *.c,*.cc file */
#endif /* HAVE_CONFIG_H */

#include <myassert.h>
#include <stdlib.h>
#include <string.h>
#include <crypt.h>

char *
mbdyn_make_salt(char *salt, size_t saltlen, const char *salt_format)
{
	static char salt_charset[] =
		"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./";

	ASSERT(strlen(salt_charset) == 64);
	ASSERT(salt);
	ASSERT(saltlen > 2);

	char	buf[34];

#if defined(HAVE_DEV_RANDOM) || defined(HAVE_DEV_URANDOM)
	FILE *fin = NULL;

#if defined(HAVE_DEV_RANDOM)
	fin = fopen("/dev/random");
#elif defined(HAVE_DEV_URANDOM)
	fin = fopen("/dev/urandom");
#endif /* HAVE_DEV_RANDOM || HAVE_DEV_URANDOM */

	fread(buf, sizeof(buf) - 1, 1, fin);
	buf[sizeof(buf) - 1] = '\0';
	fclose(fin);

	for (unsigned int i = 0; i < sizeof(buf) - 1; i++) {
		buf[i] = salt_charset[buf[i] % (sizeof(salt_charset) - 1)];
	}
#else
	for (unsigned int i = 0; i < sizeof(buf) - 1; i++) {
		buf[i] = salt_charset[rand() % (sizeof(salt_charset) - 1)];
	}
#endif

	if (salt_format) {
		snprintf(salt, saltlen, salt_format, buf);
	} else {
		strncpy(salt, buf, saltlen);
	}

	return salt;
}

