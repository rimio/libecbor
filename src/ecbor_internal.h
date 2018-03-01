/*
 * Copyright (c) 2018 Vasile Vilvoiu <vasi.vilvoiu@gmail.com>
 *
 * libecbor is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#ifndef _ECBOR_INTERNAL_H_
#define _ECBOR_INTERNAL_H_

#include <stdint.h>
#include <stddef.h>

/* Don't rely on <stdbool.h> for this */
#define false 0
#define true 1

/* CBOR major type 7 must not be exposed to user, but translated to other types */
enum {
  ECBOR_TYPE_SPECIAL = 7
};

/* Additional value meanings */
enum {
  ECBOR_ADDITIONAL_1BYTE              = 24,
  ECBOR_ADDITIONAL_2BYTE              = 25,
  ECBOR_ADDITIONAL_4BYTE              = 26,
  ECBOR_ADDITIONAL_8BYTE              = 27,
  
  ECBOR_ADDITIONAL_INDEFINITE         = 31
};

/* Simple value meanings */
enum {
  ECBOR_SIMPLE_FALSE                  = 20,
  ECBOR_SIMPLE_TRUE                   = 21,
  ECBOR_SIMPLE_NULL                   = 22,
  ECBOR_SIMPLE_UNDEFINED              = 23
};

/* Internal checks. Most of these macros rely on the function returning an
   error code, and a 'rc' value being declared locally */
#define ECBOR_INTERNAL_CHECK_ITEM_PTR(i)  \
  {                                       \
    if (!(i)) {                           \
      return ECBOR_ERR_NULL_ITEM;         \
    }                                     \
  }

#define ECBOR_INTERNAL_CHECK_VALUE_PTR(v) \
  {                                       \
    if (!(v)) {                           \
      return ECBOR_ERR_NULL_VALUE;        \
    }                                     \
  }

#define ECBOR_INTERNAL_CHECK_TYPE(t1, ref)  \
  {                                         \
    if ((t1) != (ref)) {                    \
      return ECBOR_ERR_INVALID_TYPE;        \
    }                                       \
  }

#define ECBOR_INTERNAL_CHECK_BOUNDS(index, limit) \
  {                                               \
    if ((index) >= (limit)) {                     \
      return ECBOR_ERR_INDEX_OUT_OF_BOUNDS;       \
    }                                             \
  }

/*
 * Endianness
 */
extern uint16_t
ecbor_uint16_from_big_endian (uint16_t value);

extern uint32_t
ecbor_uint32_from_big_endian (uint32_t value);

extern uint64_t
ecbor_uint64_from_big_endian (uint64_t value);

extern float
ecbor_fp32_from_big_endian (float value);

extern double
ecbor_fp64_from_big_endian (double value);

#endif