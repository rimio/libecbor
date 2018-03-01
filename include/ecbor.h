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
  ECBOR_ERR_NULL_NODE_BUFFER                = 13,
  ECBOR_ERR_NULL_VALUE                      = 14,
  ECBOR_ERR_NULL_ARRAY                      = 15,
  ECBOR_ERR_NULL_MAP                        = 16,
  
  ECBOR_ERR_NULL_ITEM                       = 20,
  ECBOR_ERR_NULL_NODE                       = 21,
  
  ECBOR_ERR_WRONG_MODE                      = 30,

  /* bounds errors */
  ECBOR_ERR_INVALID_END_OF_BUFFER           = 50,
  ECBOR_ERR_END_OF_NODE_BUFFER              = 51,
  ECBOR_ERR_EMPTY_NODE_BUFFER               = 52,
  ECBOR_ERR_INDEX_OUT_OF_BOUNDS             = 53,
  ECBOR_ERR_WONT_RETURN_INDEFINITE          = 54,
  
  /* semantic errors */
  ECBOR_ERR_CURRENTLY_NOT_SUPPORTED         = 100,
  ECBOR_ERR_INVALID_ADDITIONAL              = 101,
  ECBOR_ERR_INVALID_CHUNK_MAJOR_TYPE        = 102,
  ECBOR_ERR_NESTET_INDEFINITE_STRING        = 103,
  ECBOR_ERR_INVALID_KEY_VALUE_PAIR          = 104,
  ECBOR_ERR_INVALID_STOP_CODE               = 105,
  ECBOR_ERR_INVALID_TYPE                    = 106,
  
  /* control codes */
  ECBOR_END_OF_BUFFER                       = 200,
  ECBOR_END_OF_INDEFINITE                   = 201,
} ecbor_error_t;

/*
 * Implementation limits
 */

/*
 * CBOR types
 */
typedef enum {
  /* Unknown type */
  ECBOR_TYPE_UNDEFINED  = -1,
  
  /* First type, used for bounds checking */
  ECBOR_TYPE_FIRST      = 0,

  /* Following types have a correspondent as a major type, as per RFC */
  ECBOR_TYPE_UINT       = 0,
  ECBOR_TYPE_NINT       = 1,
  ECBOR_TYPE_BSTR       = 2,
  ECBOR_TYPE_STR        = 3,
  ECBOR_TYPE_ARRAY      = 4,
  ECBOR_TYPE_MAP        = 5,
  ECBOR_TYPE_TAG        = 6,
  /* Major type #7 is not exposed, but translated in one of the following.
   * We keep value 7 reserved so there is no confusion. */
  
  /* Following types are translated from major type #7 */
  ECBOR_TYPE_FP16       = 8,
  ECBOR_TYPE_FP32       = 9,
  ECBOR_TYPE_FP64       = 10,

  /* Last type, used for bounds checking */
  ECBOR_TYPE_LAST       = 10
} ecbor_type_t;

/*
 * CBOR Item
 */
typedef struct {
  /* item type */
  ecbor_type_t type;
  
  /* value */
  union {
    uint64_t uinteger;
    int64_t integer;
    float fp32;
    double fp64;
    struct {
      uint64_t tag_value;
      const uint8_t *child;
    } tag;
    struct {
      const uint8_t *str;
      uint64_t n_chunks;
    } string;
    const uint8_t *items;
  } value;
  
  /* storage size (serialized) of item, in bytes */
  uint64_t size;
  
  /* length of value; can mean different things:
   *   - payload (value) size, in bytes, for ints and strings
   *   - number of items in arrays and maps
   */
  uint64_t length;
  
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
  ecbor_node_t *child; /* first child */
  ecbor_node_t *next; /* next in array or map */
  ecbor_node_t *prev; /* ditto */
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
 * Encoding routines
 */


/*
 * Decoding routines
 */
extern ecbor_error_t
ecbor_decode (ecbor_decode_context_t *context, ecbor_item_t *item);

extern ecbor_error_t
ecbor_decode_tree (ecbor_decode_context_t *context);

/*
 * Tree API
 */
extern ecbor_error_t
ecbor_get_root (ecbor_decode_context_t *context, ecbor_node_t **root);

/*
 * Strict API
 */
extern ecbor_type_t
ecbor_get_type (ecbor_item_t *item);

extern ecbor_error_t
ecbor_get_value (ecbor_item_t *item, void *value);

extern ecbor_error_t
ecbor_get_length (ecbor_item_t *item, uint64_t *length);

extern ecbor_error_t
ecbor_get_array_item (ecbor_item_t *array, uint64_t index,
                      ecbor_item_t *arr_item);

extern ecbor_error_t
ecbor_get_map_item (ecbor_item_t *map, uint64_t index, ecbor_item_t *key,
                    ecbor_item_t *value);

extern ecbor_error_t
ecbor_get_tag_item (ecbor_item_t *tag, ecbor_item_t *arr_item);


/*
 * Inline API
 */
#define ECBOR_GET_TYPE(i) \
  ((i).type)

#define ECBOR_IS_INDEFINITE(i) \
  ((i).is_indefinite)

#define ECBOR_IS_NINT(i) \
  ((i).type == ECBOR_TYPE_NINT)
#define ECBOR_IS_UINT(i) \
  ((i).type == ECBOR_TYPE_UINT)
#define ECBOR_IS_INTEGER(i) \
  (ECBOR_IS_NINT(i) || ECBOR_IS_UINT(i))
#define ECBOR_IS_BSTR(i) \
  ((i).type == ECBOR_TYPE_BSTR)
#define ECBOR_IS_STR(i) \
  ((i).type == ECBOR_TYPE_STR)
#define ECBOR_IS_ARRAY(i) \
  ((i).type == ECBOR_TYPE_ARRAY)
#define ECBOR_IS_MAP(i) \
  ((i).type == ECBOR_TYPE_MAP)
#define ECBOR_IS_TAG(i) \
  ((i).type == ECBOR_TYPE_TAG)
#define ECBOR_IS_FP32(i) \
  ((i).type == ECBOR_TYPE_FP32)
#define ECBOR_IS_FLOAT(i) \
  ECBOR_IS_FP32(i)
#define ECBOR_IS_FP64(i) \
  ((i).type == ECBOR_TYPE_FP64)
#define ECBOR_IS_DOUBLE(i) \
  ECBOR_IS_FP64(i)

#define ECBOR_GET_INT(i) \
  ((i).value.integer)
#define ECBOR_GET_UINT(i) \
  ((i).value.uinteger)
#define ECBOR_GET_BSTR(i) \
  ((i).value.string)
#define ECBOR_GET_STR(i) \
  ((i).value.string)
#define ECBOR_GET_TAG(i) \
  ((i).value.tag.tag_value)

#endif