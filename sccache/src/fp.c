/* sccache - The Simple Compile Cache
 * Copyright (C) 2003 Marcus Perlick
 * mailto: riffraff@users.sf.net
 *    
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA. */
#include "fp.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "md4.h"

/* Do this for compatibility. 4.3+BSD has the MAP_FILE macro, others
 * not. */
#ifndef MAP_FILE
#define MAP_FILE 0
#endif

void
fingerprint( MD4_CTX *ctx, const char *filename )
{
  struct stat stbuf;
  char *mem;
  int fd = open( filename, O_RDONLY );

  if ( fd < 0 ) {
    perror( "fingerprint:open" );
    exit( 1 );
  }

  if ( fstat( fd, &stbuf ) < 0 ) {
    perror( "fingerprint:fstat" );
    exit( 1 );
  }

  if ( (mem = mmap( 0, 
                    stbuf.st_size,
                    PROT_READ,
                    MAP_FILE | MAP_SHARED,
                    fd,
                    0 ))
       == (caddr_t)-1 )
    {
      perror( "fingerprint:mmap" );
      exit( 1 );
    }

  MD4Update( ctx, mem, stbuf.st_size );

  munmap( mem, stbuf.st_size );
  close( fd );
}

void
make_fpstring( char str[ 33 ], unsigned char digest[ 16 ] )
{
  static const char nibbles[] = { '0', '1', '2', '3', '4', '5', '6', '7',
                                  '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

  unsigned char *end = digest + 16;
  while ( digest < end ) {
    *str = nibbles[ *digest & 0xf ];
    str++;
    *str = nibbles[ *digest >> 4 ];
    str++;

    digest++;
  }

  *str = '\0';
}
