
/** \file util.h
 * \brief 
 *
 */

#ifndef _UTIL_CC_H_
#define _UTIL_CC_H_

#include <stdint.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

  size_t strlcpy( char *, const char *, size_t );
  size_t strlcat( char *, const char *, size_t );
  int mkfile( char *, size_t );
  int parse_uuid( char *, uint8_t * );
  int format_uuid( char *, uint8_t * );
  void trap_error_signals();
  void recursive_remove( char * );

#ifdef __cplusplus
}
#endif

#endif

/*
 * vim:autoindent
 * vim:expandtab
 */
