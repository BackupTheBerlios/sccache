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
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <utime.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "fp.h"
#include "cachestat.h"

#define APP "sccache"

#ifdef NDEBUG
# define DBG( msg )
# define DBGAV( msg, ... )
#else
# define DBG( msg ) printf( msg "\n" );fflush(stdout);
# define DBGAV( msg, ... ) printf( msg "\n", __VA_ARGS__ );fflush(stdout);
#endif

int
assure_dir( char *path, size_t len )
{
  struct stat stbuf;
  char *it = path + len;

  for ( it--; it > path; it-- ) {
    if ( *it == '/' ) {
      *it = '\0';
      if ( stat( path, &stbuf ) ) {
        if ( assure_dir( path, it - path ) ) {
          int ret = !mkdir( path, 0777 );
          *it = '/';
          return ret;
        }
        *it = '/';
        return 0;
      }
      else if ( S_ISDIR( stbuf.st_mode ) ) {
        *it = '/';
        return 1;
      }
      else {
        *it = '/';
        return 0;
      }
    }
  }

  return 1;
}

typedef struct {
  const char *cache;
  char *src;
  const char *obj;

  size_t pp_argc;
  char **pp_argv;
  size_t cc_argc;
  char **cc_argv;
  
  char *i_file;
  MD4_CTX i_ctx;
  unsigned char fp[ 16 ];

  char *c_obj;
} Session;

int
preprocess( Session *ses )
{
  int crc;
  int ofd;
  pid_t pid;
  char *tmp_file;
  size_t c_len = strlen( ses->cache ) + 12;
  
  ses->pp_argc += 2;
  ses->pp_argv = realloc( ses->pp_argv, ses->pp_argc * sizeof(char*) );
  ses->pp_argv[ ses->pp_argc - 2 ] = ses->src;
  ses->pp_argv[ ses->pp_argc - 1 ] = NULL;

  tmp_file = alloca( c_len );
  sprintf( tmp_file, "%s/tmp_XXXXXX", ses->cache );

  if ( (ofd = mkstemp( tmp_file )) == -1 ) {
    fprintf( stderr,
	     APP":cannot create temp-file:%s:%s.\n",
	     tmp_file,
	     strerror( errno ) );
    exit( -1 );
  }

  DBGAV( "Temp .i-file is %s.", tmp_file );

  if ( (pid = fork()) ) {
    close( ofd );
    waitpid( pid, &crc, 0 );
  }
  else {
    if ( dup2( ofd, STDOUT_FILENO ) == -1 ) {
      perror( APP":dup2" );
      exit( -1 );
    }
    close( ofd );

    execvp( ses->pp_argv[ 0 ], ses->pp_argv );

    perror( APP":execvp" );
    _exit( -1 );
  }  

  if ( crc ) {
    DBGAV( "Preprocessing failed. Remove %s.", tmp_file );
    unlink( tmp_file );
  }
  else {
    /* _MP_ <:todo:> do all checks in this block */
    ses->i_file = malloc( c_len += 2 );
    sprintf( ses->i_file, "%s.i", tmp_file );
    rename( tmp_file, ses->i_file );

    DBGAV( "Moved %s to %s.", tmp_file, ses->i_file );
  }

  return crc;
}

#define CACHE_DOESNT_HAVE_IT 0
#define CACHE_HAS_OUTPUT 1
#define CACHE_HAS_OBJECT 2

int
check_cache( Session* ses )
{
  struct stat stbuf;
  char fp_str[ 33 ];
  char *tmp;
  size_t c_obj_sz = strlen( ses->cache ) + 42;

  make_fpstring( fp_str, ses->fp );

  ses->c_obj = malloc( c_obj_sz );
  sprintf( ses->c_obj, "%s/%c/%c/%c/%s.o", 
           ses->cache,
           fp_str[ 0 ], fp_str[ 1 ], fp_str[ 2 ],
           fp_str );

  DBGAV( "Checking cache for %s.", ses->c_obj );

  if ( !stat( ses->c_obj, &stbuf ) ) {
    DBG( "Found file in cache." );
    return CACHE_HAS_OBJECT;
  }

  tmp = alloca( c_obj_sz + 4 );
  sprintf( tmp, "%s.err", ses->c_obj );

  DBGAV( "Object file is not in cache.\nChecking for error file %s.", tmp ); 

  if ( !stat( tmp, &stbuf ) ) {
    DBG( "Found error file." );
    return CACHE_HAS_OUTPUT;
  }

  memcpy( tmp + c_obj_sz - 1, ".out", 4 );

  DBGAV( "Error file is not in cache.\nChecking for output file %s.", tmp ); 

  if ( !stat( tmp, &stbuf ) ) {
    DBG( "Found output file." );
    return CACHE_HAS_OUTPUT;
  }

  DBGAV( "Found nothing for fingerprint %s.", fp_str );
  
  return CACHE_DOESNT_HAVE_IT;
}

int
compile_to_cache( Session* ses )
{
  int crc;
  pid_t pid;

  ses->cc_argc += 4;
  ses->cc_argv = realloc( ses->cc_argv, ses->cc_argc * sizeof(char*) );
  ses->cc_argv[ ses->cc_argc - 4 ] = "-o";
  ses->cc_argv[ ses->cc_argc - 3 ] = ses->c_obj;
  ses->cc_argv[ ses->cc_argc - 2 ] = ses->i_file;
  ses->cc_argv[ ses->cc_argc - 1 ] = NULL;

  if ( (pid = fork()) ) {
    waitpid( pid, &crc, 0 );
  }
  else {
    int fd;
    size_t c_obj_len = strlen( ses->c_obj );
    char *tmp = alloca( c_obj_len + 5 );

    assure_dir( ses->c_obj, c_obj_len );

    close( STDOUT_FILENO );
    close( STDERR_FILENO );

    sprintf( tmp, "%s.out", ses->c_obj );
    if( (fd = open( tmp, O_WRONLY | O_CREAT | O_TRUNC, 0664 )) < 0 ) {
      perror( APP":compile_to_cache:open" );
      _exit( -1 );
    }

    sprintf( tmp, "%s.err", ses->c_obj );
    if( (fd = open( tmp, O_WRONLY | O_CREAT | O_TRUNC, 0664 )) < 0 ) {
      perror( APP":compile_to_cache:open" );
      _exit( -1 );
    }

    execvp( ses->cc_argv[ 0 ], ses->cc_argv );

    perror( APP":execvp" );
    _exit( -1 );
  }

  unlink( ses->i_file );

#ifndef NDEBUG
  if ( crc ) {
    DBGAV( "Compile of %s failed.", ses->i_file );
  }
  else {
    DBGAV( "Compiled %s -> %s.", ses->i_file, ses->c_obj );
  }
#endif
  
  return crc;
}

void
get_from_cache( Session *ses )
{
  DBGAV( "Taking %s form cache as %s.", ses->c_obj, ses->obj );

  unlink( ses->obj );
  if ( link( ses->c_obj, ses->obj ) ) {
    perror( APP":get_from_cache" );
    exit( 1 );
  }
  utime( ses->obj, NULL );
}

void
copy_to_file( const char *file, FILE* os )
{
  char buffer[ 1024 ];
  size_t rdsz;
  FILE *is;

  if ( !(is = fopen( file, "r" )) ) {
    perror( APP":copy_to_file" );
    exit( 1 );
  }

  while ( (rdsz = fread( buffer, 1, 1024, is )) ) {
    fwrite( buffer, rdsz, 1, os );
  }

  fclose( is );
}

void
show_messages( Session *ses )
{
  struct stat stbuf;
  size_t len = strlen( ses->c_obj );
  char *tmp = alloca( len + 5 );
  
  sprintf( tmp, "%s.out", ses->c_obj );
  if ( !stat( tmp, &stbuf ) ) {
    DBGAV( "Writing %s to stdout.", tmp );
    copy_to_file( tmp, stdout );
  }

  memcpy( tmp + len, ".err", 4 );
  if ( !stat( tmp, &stbuf ) ) {
    DBGAV( "Writing %s to stderr.", tmp );
    copy_to_file( tmp, stderr );
  }
}

char**
tok_cmd( char *cmd, size_t *argc )
{
  char **ret = malloc( sizeof(char*) );
  char *tok;

  assert( cmd != NULL );

  *argc = 1;
  *ret = strtok( cmd, " \t" );
  while ( (tok = strtok( NULL, " \t" )) ) {
    ret = realloc( ret, (*argc + 1) * sizeof(char*) );
    ret[ (*argc)++ ] = tok;
  }

  return ret;
}

void
print_usage( void )
{
  printf( "sccache <cache-dir> <source> <object> <pp-cmd> <cc-cmd>\n" );
}

int
main( int argc, char *argv[] )
{
  int rc = 0, i;
  Session session;

  if ( argc != 6 ) {
    print_usage();
    exit( 1 );
  }

  DBGAV( "%s started for %s -> %s", argv[ 0 ], argv[ 2 ], argv[ 3 ] );

  MD4Init( &session.i_ctx );

  /* Now hash: source, pp-cmd, cc-cmd */
  MD4Update( &session.i_ctx, argv[ 2 ], strlen( argv[ i ] ) );
  MD4Update( &session.i_ctx, argv[ 4 ], strlen( argv[ i ] ) );
  MD4Update( &session.i_ctx, argv[ 5 ], strlen( argv[ i ] ) );
  
  session.cache = argv[ 1 ];
  session.src = argv[ 2 ];
  session.obj = argv[ 3 ];

  stat_open( session.cache );
  
  session.pp_argv = tok_cmd( argv[ 4 ], &session.pp_argc );
  session.cc_argv = tok_cmd( argv[ 5 ], &session.cc_argc );

  if ( preprocess( &session ) ) {
    exit( 1 );
  }

  fingerprint( &session.i_ctx, session.i_file );

  /* Before we call check_cache, we have to finalize the MD4 digest */
  MD4Final( session.fp, &session.i_ctx );
  switch ( check_cache( &session ) ) {
  case CACHE_DOESNT_HAVE_IT:
    theStats.fail++;
    if ( !compile_to_cache( &session ) ) {
      get_from_cache( &session );
    }
    else {
      rc = 1;
    }
    break;

  case CACHE_HAS_OBJECT:
    theStats.hit++;
    unlink( session.i_file );
    get_from_cache( &session );
    break;

  default:
    theStats.hit++;
    unlink( session.i_file );
    rc = 1;
  }
  
  show_messages( &session );

  stat_close( 1 );
  
  return rc;
}
