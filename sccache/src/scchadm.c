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
#include <stdlib.h>
#include <stdio.h>

#include "cachestat.h"

int main( int argc, char *argv[] )
{
  int rc = 0;
  
  if ( argc != 2 ) {
    printf( "%s <cache-dir>\n", argv[ 0 ] );
    exit( 1 );
  }

  if ( !stat_open( argv[ 1 ] ) ) {
    printf( "sccache - statistics:\n hit:\t%lu\n fail:\t%lu\n",
	    theStats.hit,
	    theStats.fail );
  }
  
  stat_close( 0 );

  return rc;
}
