/*
 * Copyright (c) 2018 Vasile Vilvoiu <vasi.vilvoiu@gmail.com>
 *
 * libecbor is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#ifndef _ECBOR_INTERNAL_H_
#define _ECBOR_INTERNAL_H_

#include <stdint.h>

/* Don't rely on <stddef.h> for this */
#define NULL 0
#define false 0
#define true 1

/* Additional value meanings */
enum {
  ECBOR_ADDITIONAL_1BYTE              = 24,
  ECBOR_ADDITIONAL_2BYTE              = 25,
  ECBOR_ADDITIONAL_4BYTE              = 26,
  ECBOR_ADDITIONAL_8BYTE              = 27,
  
  ECBOR_ADDITIONAL_INDEFINITE         = 31
};

/*
 * Endianness
 */
extern uint16_t
ecbor_uint16_from_big_endian (uint16_t value);

extern uint32_t
ecbor_uint32_from_big_endian (uint32_t value);

extern uint64_t
ecbor_uint64_from_big_endian (uint64_t value);

#endif