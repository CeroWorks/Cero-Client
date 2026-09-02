#ifndef CERO_PLATFORM_DEFINES_H
#define CERO_PLATFORM_DEFINES_H

#define _POSIX_C_SOURCE 200112L

#if defined(__FreeBSD__) || defined(__OpenBSD__) || \
      defined(__NetBSD__)   || defined(__DragonFly__)
  #ifndef __BSD_VISIBLE
    #define __BSD_VISIBLE 1
  #endif
  #ifndef _BSD_SOURCE
    #define _BSD_SOURCE 1
  #endif
  #ifndef _NETBSD_SOURCE
    #define _NETBSD_SOURCE 1
  #endif
#elif defined(__linux__)
  #ifndef _DEFAULT_SOURCE
    #define _DEFAULT_SOURCE 1
  #endif
#elif defined(__APPLE__)
  #ifndef _DARWIN_C_SOURCE
    #define _DARWIN_C_SOURCE 1
  #endif
#endif

#if defined(_WIN32)
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || \
      defined(__NetBSD__)   || defined(__DragonFly__)
  #define __BSD__ 1
#elif defined(__linux__)
#endif

#ifdef _WIN32
  #define PATH_SEP "\\"
#else
  #define PATH_SEP "/"
#endif

#endif /* CERO_PLATFORM_DEFINES_H */
