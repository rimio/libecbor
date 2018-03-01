/*
 * Copyright (c) 2018 Vasile Vilvoiu <vasi.vilvoiu@gmail.com>
 *
 * libecbor is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#ifndef _ECBOR_H_
#define _ECBOR_H_

#include <stdint.h>
#include <stddef.h>

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
  ECBOR_ERR_NULL_VALUE                      = 14,
  ECBOR_ERR_NULL_ARRAY                      = 15,
  ECBOR_ERR_NULL_MAP                        = 16,
  
  ECBOR_ERR_NULL_ITEM                       = 20,
  
  ECBOR_ERR_WRONG_MODE                      = 30,

  /* bounds errors */
  ECBOR_ERR_INVALID_END_OF_BUFFER           = 50,
  ECBOR_ERR_END_OF_ITEM_BUFFER              = 51,
  ECBOR_ERR_EMPTY_ITEM_BUFFER               = 52,
  ECBOR_ERR_INDEX_OUT_OF_BOUNDS             = 53,
  ECBOR_ERR_WONT_RETURN_INDEFINITE          = 54,
  ECBOR_ERR_WONT_RETURN_DEFINITE            = 55,
  ECBOR_ERR_VALUE_OVERFLOW                  = 56,
  
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
  /* This is used when, for some reason, no other type could be resolved;
     Usually denotes an error occured. */
  ECBOR_TYPE_NONE       = -1,
  
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
  ECBOR_TYPE_BOOL       = 11,
  ECBOR_TYPE_NULL       = 12,
  ECBOR_TYPE_UNDEFINED  = 13,

  /* Last type, used for bounds checking */
  ECBOR_TYPE_LAST       = ECBOR_TYPE_UNDEFINED
} ecbor_type_t;

/*
 * CBOR Item
 */
typedef struct ecbor_item ecbor_item_t;
struct ecbor_item {
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
      size_t n_chunks;
    } string;
    const uint8_t *items;
  } value;
  
  /* storage size (serialized) of item, in bytes */
  size_t size;
  
  /* length of value; can mean different things:
   *   - payload (value) size, in bytes, for ints and strings
   *   - number of items in arrays and maps
   */
  size_t length;
  
  /* non-zero if size is indefinite (for strings, maps and arrays) */
  uint8_t is_indefinite;
  
  /* tree links and metainformation (only populated in tree decoder mode) */
  ecbor_item_t *parent;
  ecbor_item_t *child; /* first child */
  ecbor_item_t *next; /* next in array or map */
  ecbor_item_t *prev; /* ditto */
  size_t index; /* index in array or map */
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
  size_t bytes_left;
} ecbor_encode_context_t;
 
typedef struct {
  /* mode of operation for this context */
  ecbor_mode_t mode;
  
  /* input buffer position */
  const uint8_t *in_position;
  
  /* remaining bytes */
  size_t bytes_left;

  /* item buffer */
  ecbor_item_t *items;
  
  /* capacity of item buffer (in items) */
  size_t item_capacity;
  
  /* number of used items so far */
  size_t n_items;
} ecbor_decode_context_t;


/*
 * Initialization routines
 */
extern ecbor_error_t
ecbor_initialize_decode (ecbor_decode_context_t *context,
                         const uint8_t *buffer,
                         size_t buffer_size);

extern ecbor_error_t
ecbor_initialize_decode_streamed (ecbor_decode_context_t *context,
                                  const uint8_t *buffer,
                                  size_t buffer_size);

extern ecbor_error_t
ecbor_initialize_decode_tree (ecbor_decode_context_t *context,
                              const uint8_t *buffer,
                              size_t buffer_size,
                              ecbor_item_t *item_buffer,
                              size_t item_capacity);


/*
 * Encoding routines
 */


/*
 * Decoding routines
 */
extern ecbor_error_t
ecbor_decode (ecbor_decode_context_t *context, ecbor_item_t *item);

extern ecbor_error_t
ecbor_decode_tree (ecbor_decode_context_t *context, ecbor_item_t **root);

/*
 * Tree API
 */

/*
 * Strict API
 */

/* Metadata */
extern ecbor_type_t
ecbor_get_type (ecbor_item_t *item);

extern ecbor_error_t
ecbor_get_length (ecbor_item_t *item, size_t *length);


/* Child items */
extern ecbor_error_t
ecbor_get_array_item (ecbor_item_t *array, size_t index,
                      ecbor_item_t *arr_item);

extern ecbor_error_t
ecbor_get_array_item_ptr (ecbor_item_t *array, size_t index,
                          ecbor_item_t **item);

extern ecbor_error_t
ecbor_get_map_item (ecbor_item_t *map, size_t index, ecbor_item_t *key,
                    ecbor_item_t *value);

extern ecbor_error_t
ecbor_get_map_item_ptr (ecbor_item_t *map, size_t index, ecbor_item_t **key,
                        ecbor_item_t **value);

extern ecbor_error_t
ecbor_get_tag_item (ecbor_item_t *tag, ecbor_item_t *item);

extern ecbor_error_t
ecbor_get_tag_item_ptr (ecbor_item_t *tag, ecbor_item_t **item);


/* Ints */
extern ecbor_error_t
ecbor_get_uint8 (ecbor_item_t *item, uint8_t *value);

extern ecbor_error_t
ecbor_get_uint16 (ecbor_item_t *item, uint16_t *value);

extern ecbor_error_t
ecbor_get_uint32 (ecbor_item_t *item, uint32_t *value);

extern ecbor_error_t
ecbor_get_uint64 (ecbor_item_t *item, uint64_t *value);


extern ecbor_error_t
ecbor_get_int8 (ecbor_item_t *item, int8_t *value);

extern ecbor_error_t
ecbor_get_int16 (ecbor_item_t *item, int16_t *value);

extern ecbor_error_t
ecbor_get_int32 (ecbor_item_t *item, int32_t *value);

extern ecbor_error_t
ecbor_get_int64 (ecbor_item_t *item, int64_t *value);


/* Strings */
extern ecbor_error_t
ecbor_get_str (ecbor_item_t *str, char **value);

extern ecbor_error_t
ecbor_get_str_chunk_count (ecbor_item_t *str, size_t *count);

extern ecbor_error_t
ecbor_get_str_chunk (ecbor_item_t *str, size_t index, ecbor_item_t *chunk);


extern ecbor_error_t
ecbor_get_bstr (ecbor_item_t *str, uint8_t **value);

extern ecbor_error_t
ecbor_get_bstr_chunk_count (ecbor_item_t *str, size_t *count);

extern ecbor_error_t
ecbor_get_bstr_chunk (ecbor_item_t *str, size_t index, ecbor_item_t *chunk);


/* Tag */
extern ecbor_error_t
ecbor_get_tag_value (ecbor_item_t *tag, uint64_t *tag_value);


/* Floating point */
extern ecbor_error_t
ecbor_get_fp32 (ecbor_item_t *item, float *value);

extern ecbor_error_t
ecbor_get_fp64 (ecbor_item_t *item, double *value);


/* Boolean */
extern ecbor_error_t
ecbor_get_bool (ecbor_item_t *item, uint8_t *value);


/*
 * Inline API
 */
#define ECBOR_GET_TYPE(i) \
  ((i)->type)
#define ECBOR_GET_LENGTH(i) \
  ((i)->type == ECBOR_TYPE_MAP ? (i)->length / 2 : (i)->length)

#define ECBOR_GET_INT(i) \
  ((i)->value.integer)
#define ECBOR_GET_UINT(i) \
  ((i)->value.uinteger)

#define ECBOR_GET_STRING(i) \
  ((i)->value.string)

#define ECBOR_GET_FP32(i) \
  ((i)->value.fp32)
#define ECBOR_GET_FP64(i) \
  ((i)->value.fp64)

#define ECBOR_GET_BOOL(i) \
  ((i)->value.uinteger)

#define ECBOR_GET_TAG_VALUE(i) \
  ((i)->value.tag.tag_value)
#define ECBOR_GET_TAG_ITEM(i) \
  ((i)->child)

#define ECBOR_IS_INDEFINITE(i) \
  ((i)->is_indefinite)
#define ECBOR_IS_DEFINITE(i) \
  (!(i)->is_indefinite)
#define ECBOR_IS_NINT(i) \
  ((i)->type == ECBOR_TYPE_NINT)
#define ECBOR_IS_UINT(i) \
  ((i)->type == ECBOR_TYPE_UINT)
#define ECBOR_IS_INTEGER(i) \
  (ECBOR_IS_NINT(i) || ECBOR_IS_UINT(i))
#define ECBOR_IS_BSTR(i) \
  ((i)->type == ECBOR_TYPE_BSTR)
#define ECBOR_IS_STR(i) \
  ((i)->type == ECBOR_TYPE_STR)
#define ECBOR_IS_ARRAY(i) \
  ((i)->type == ECBOR_TYPE_ARRAY)
#define ECBOR_IS_MAP(i) \
  ((i)->type == ECBOR_TYPE_MAP)
#define ECBOR_IS_TAG(i) \
  ((i)->type == ECBOR_TYPE_TAG)
#define ECBOR_IS_FP32(i) \
  ((i)->type == ECBOR_TYPE_FP32)
#define ECBOR_IS_FLOAT(i) \
  ECBOR_IS_FP32(i)
#define ECBOR_IS_FP64(i) \
  ((i)->type == ECBOR_TYPE_FP64)
#define ECBOR_IS_DOUBLE(i) \
  ECBOR_IS_FP64(i)
#define ECBOR_IS_BOOL(i) \
  ((i)->type == ECBOR_TYPE_BOOL)
#define ECBOR_IS_NULL(i) \
  ((i)->type == ECBOR_TYPE_NULL)
#define ECBOR_IS_UNDEFINED(i) \
  ((i)->type == ECBOR_TYPE_UNDEFINED)

#endif