/*
 * Copyright (c) 2018 Vasile Vilvoiu <vasi.vilvoiu@gmail.com>
 *
 * libecbor is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#ifndef _ECBOR_H_
#define _ECBOR_H_

#include <stdint.h>

/*
 * Error codes
 */
typedef enum {
  /* no error */
  ECBOR_OK                                  = 0,
  ECBOR_ERR_UNKNOWN                         = 1,    /* shouldn't happen, hopefully */
  
  /* internal (i.e. misuse) errors */
  ECBOR_ERR_NULL_CONTEXT                    = 10,
  ECBOR_ERR_NULL_INPUT_BUFFER               = 11,
  ECBOR_ERR_NULL_OUTPUT_BUFFER              = 12,
  ECBOR_ERR_NULL_ITEM_BUFFER                = 13,
  
  ECBOR_ERR_NULL_ITEM                       = 20,
  
  ECBOR_ERR_WRONG_MODE                      = 30,
  
  /* bounds errors */
  ECBOR_ERR_INVALID_END_OF_BUFFER           = 50,
  ECBOR_ERR_END_OF_NODE_BUFFER              = 51,
  
  /* syntax errors */
  ECBOR_ERR_CURRENTLY_NOT_SUPPORTED         = 100,
  ECBOR_ERR_INVALID_ADDITIONAL              = 101,
  ECBOR_ERR_INVALID_CHUNK_MAJOR_TYPE        = 102,
  ECBOR_ERR_NESTET_INDEFINITE_STRING        = 103,
  ECBOR_ERR_INVALID_KEY_VALUE_PAIR          = 104,
  ECBOR_ERR_INVALID_STOP_CODE               = 105,
  
  /* control codes */
  ECBOR_END_OF_BUFFER                       = 200,
  ECBOR_END_OF_INDEFINITE                   = 201,
} ecbor_error_t;

/*
 * Implementation limits
 */

/*
 * CBOR major types
 */
typedef enum {
  ECBOR_MT_UNDEFINED  = -1,
  ECBOR_MT_UINT       = 0,
  ECBOR_MT_NINT       = 1,
  ECBOR_MT_BSTR       = 2,
  ECBOR_MT_STR        = 3,
  ECBOR_MT_ARRAY      = 4,
  ECBOR_MT_MAP        = 5,
  ECBOR_MT_TAG        = 6,
  ECBOR_MT_SPECIAL    = 7,
} ecbor_major_type_t;

/*
 * CBOR Item
 */
typedef struct {
  /* major type of item */
  ecbor_major_type_t major_type;
  
  /* value */
  union {
    uint64_t uinteger;
    int64_t integer;
    const uint8_t *string;
    const uint8_t *items;
  } value;
  
  /* storage size of value, in bytes */
  uint64_t size;
  
  /* number of chunks in value, in case it is indefinite */
  uint64_t n_chunks;
  
  /* non-zero if size is indefinite (for strings, maps and arrays) */
  uint8_t is_indefinite;
} ecbor_item_t;

/*
 * CBOR tree node
 */
typedef struct ecbor_node ecbor_node_t;
struct ecbor_node {
  ecbor_item_t item;
  ecbor_node_t *parent;
  ecbor_node_t *next; /* next in array or map */
  uint64_t index; /* index in array or map */
};

/*
 * Mode
 */
typedef enum {
  ECBOR_MODE_DECODE           = 0,
  ECBOR_MODE_DECODE_STREAMED  = 1,
  ECBOR_MODE_DECODE_TREE      = 2,
  ECBOR_MODE_ENCODE           = 3
} ecbor_mode_t;

/*
 * CBOR parsing context
 */
typedef struct {
  /* mode of operation for this context */
  ecbor_mode_t mode;
  
  /* output buffer position */
  uint8_t *out_position;
  
  /* remaining bytes */
  unsigned int bytes_left;
} ecbor_encode_context_t;
 
typedef struct {
  /* mode of operation for this context */
  ecbor_mode_t mode;
  
  /* input buffer position */
  const uint8_t *in_position;
  
  /* remaining bytes */
  uint64_t bytes_left;

  /* node buffer */
  ecbor_node_t *nodes;
  
  /* capacity of item buffer (in items) */
  unsigned int node_capacity;
  
  /* number of used items so far */
  unsigned int n_nodes;
} ecbor_decode_context_t;


/*
 * Initialization routines
 */
extern ecbor_error_t
ecbor_initialize_decode (ecbor_decode_context_t *context,
                         const uint8_t *buffer,
                         uint64_t buffer_size);

extern ecbor_error_t
ecbor_initialize_decode_streamed (ecbor_decode_context_t *context,
                                  const uint8_t *buffer,
                                  uint64_t buffer_size);

extern ecbor_error_t
ecbor_initialize_decode_tree (ecbor_decode_context_t *context,
                              const uint8_t *buffer,
                              uint64_t buffer_size,
                              ecbor_node_t *node_buffer,
                              uint64_t node_capacity);


/*
 * Decoding routines
 */
extern ecbor_error_t
ecbor_decode (ecbor_decode_context_t *context, ecbor_item_t *item);

/*
 * Encoding routines
 */

#endif