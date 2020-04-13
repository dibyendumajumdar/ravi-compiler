/*
* This version is part of the Ravi Compiler project.
* Copyright (C) 2017 Dibyendu Majumdar
*/

#ifndef ravicomp_PORT_H
#define ravicomp_PORT_H

#ifndef _WIN32
#include <unistd.h>
#include <alloca.h>
#else
#include <io.h>
#include <malloc.h>
#endif

#if defined(_WIN32) && defined(_MSC_VER)
#include <stdlib.h>
#ifndef __cplusplus
#define inline __inline
#endif
#define __alignof__ __alignof
#endif

#ifdef __GNUC__
#define FORMAT_ATTR(pos) __attribute__((__format__(__printf__, pos, pos + 1)))
#define NORETURN_ATTR __attribute__((__noreturn__))
#define SENTINEL_ATTR __attribute__((__sentinel__))
#else
#define FORMAT_ATTR(pos)
#define NORETURN_ATTR
#define SENTINEL_ATTR
#endif

#endif
