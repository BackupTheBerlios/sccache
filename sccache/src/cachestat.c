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
#include "cachestat.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <alloca.h>

Statistic theStats = {
  0,
  0
};

static FILE* stats_file = NULL;

int
stat_open( const char* cache_dir )
{
  int rc = 0;
  char *stf = alloca( strlen( cache_dir ) + 7 );

  assert( stats_file == NULL );
  
  sprintf( stf, "%s/stats", cache_dir );

  if ( (stats_file = fopen( stf, "r+" )) == NULL ) {
    if ( (stats_file = fopen( stf, "w" )) == NULL ) {
      perror( "stat_open:fopen(\"w\")" );
      rc = 1;
    }
  }
  else if ( fread( &theStats, sizeof(theStats), 1, stats_file ) != 1 ) {
    rc = 1;
  }

  return rc;
}

void
stat_close( char write_back )
{
  if ( stats_file ) {
    if ( write_back ) {
      fseek( stats_file, 0, SEEK_SET );
      fwrite( &theStats, sizeof(theStats), 1, stats_file );
    }
    fclose( stats_file );
    stats_file = NULL;
  }
}
