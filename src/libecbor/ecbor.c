/*
 * Copyright (c) 2018 Vasile Vilvoiu <vasi.vilvoiu@gmail.com>
 *
 * libecbor is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include "ecbor.h"
#include "ecbor_internal.h"

uint16_t
ecbor_uint16_from_big_endian (uint16_t value)
{
#if __BYTE_ORDER__ && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
  return value;
#elif __BYTE_ORDER__ && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
  /* TODO: ARM optimization */
  return ((value << 8) & 0xff00)
       | ((value >> 8) & 0x00ff);
#else
  #error "Endianness not supported!"
#endif
}

uint32_t
ecbor_uint32_from_big_endian (uint32_t value)
{
#if __BYTE_ORDER__ && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
  return value;
#elif __BYTE_ORDER__ && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
  /* TODO: ARM optimization */
  return ((value <<  8) & 0x00ff0000)
       | ((value >>  8) & 0x0000ff00)
       | ((value << 24) & 0xff000000)
       | ((value >> 24) & 0x000000ff);
#else
  #error "Endianness not supported!"
#endif
}

uint64_t
ecbor_uint64_from_big_endian (uint64_t value)
{
#if __BYTE_ORDER__ && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
  return value;
#elif __BYTE_ORDER__ && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
  /* TODO: ARM optimization */
  return ((value <<  8) & 0x000000ff00000000)
       | ((value >>  8) & 0x00000000ff000000)
       | ((value << 24) & 0x0000ff0000000000)
       | ((value >> 24) & 0x0000000000ff0000)
       | ((value << 40) & 0x00ff000000000000)
       | ((value >> 40) & 0x000000000000ff00)
       | ((value << 56) & 0xff00000000000000)
       | ((value >> 56) & 0x00000000000000ff);
#else
  #error "Endianness not supported!"
#endif
}

float
ecbor_fp32_from_big_endian (float value)
{
#if __BYTE_ORDER__ && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
  return value;
#elif __BYTE_ORDER__ && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
  /* while it looks like a lot of casts, we're doing this just so we follow
     aliasing rules; the compiler will optimize */
  uint8_t *base = (uint8_t *) &value;
  uint32_t r = ecbor_uint32_from_big_endian(*((uint32_t *) base));
  base = (uint8_t *) &r;
  return *((float *) base);
#else
  #error "Endianness not supported!"
#endif
}

double
ecbor_fp64_from_big_endian (double value)
{
#if __BYTE_ORDER__ && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
  return value;
#elif __BYTE_ORDER__ && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
  uint8_t *base = (uint8_t *) &value;
  uint64_t r = ecbor_uint64_from_big_endian(*((uint64_t *) base));
  base = (uint8_t *) &r;
  return *((double *) base);
#else
  #error "Endianness not supported!"
#endif
}

uint16_t
ecbor_uint16_to_big_endian (uint16_t value)
{
  return ecbor_uint16_from_big_endian (value);
}

uint32_t
ecbor_uint32_to_big_endian (uint32_t value)
{
  return ecbor_uint32_from_big_endian (value);
}

uint64_t
ecbor_uint64_to_big_endian (uint64_t value)
{
  return ecbor_uint64_from_big_endian (value);
}

float
ecbor_fp32_to_big_endian (float value)
{
  return ecbor_fp32_from_big_endian (value);
}

double
ecbor_fp64_to_big_endian (double value)
{
  return ecbor_fp64_from_big_endian (value);
}

ecbor_type_t
ecbor_get_type (ecbor_item_t *item)
{
  ECBOR_INTERNAL_CHECK_ITEM_PTR (item);
  if (item->type < ECBOR_TYPE_FIRST
      || item->type > ECBOR_TYPE_LAST) {
    return ECBOR_TYPE_NONE;
  }
  return item->type;
}

ecbor_error_t
ecbor_get_length (ecbor_item_t *item, size_t *length)
{
  ECBOR_INTERNAL_CHECK_ITEM_PTR (item);
  ECBOR_INTERNAL_CHECK_VALUE_PTR (length);
  
  switch (item->type) {
    case ECBOR_TYPE_BSTR:
    case ECBOR_TYPE_STR:
    case ECBOR_TYPE_ARRAY:
      *length = item->length;
      break;

    case ECBOR_TYPE_MAP:
      *length = item->length / 2;
      break;

    default:
      return ECBOR_ERR_INVALID_TYPE;
  }
  
  return ECBOR_OK;
}

ecbor_error_t
ecbor_get_array_item (ecbor_item_t *array, size_t index,
                      ecbor_item_t *item)
{
  ecbor_error_t rc;

  if (!array) {
    return ECBOR_ERR_NULL_ARRAY;
  }
  ECBOR_INTERNAL_CHECK_TYPE (array->type, ECBOR_TYPE_ARRAY);
  ECBOR_INTERNAL_CHECK_BOUNDS (index, array->length);
  ECBOR_INTERNAL_CHECK_ITEM_PTR (item);

  if (array->child) {
    /* parsed in tree mode */
    ecbor_item_t *item_ptr;
    rc = ecbor_get_array_item_ptr (array, index, &item_ptr);
    if (rc != ECBOR_OK) {
      return rc;
    }
    
    /* copy contents */
    (*item) = (*item_ptr);
  } else {
    /* parsed in normal mode, we must re-parse it */
    ecbor_decode_context_t context;
    size_t i;

    rc = ecbor_initialize_decode (&context, array->value.items, array->size);
    if (rc != ECBOR_OK) {
      return rc;
    }
    
    for (i = 0; i <= index; i ++) {
      rc = ecbor_decode (&context, item);
      if (rc != ECBOR_OK) {
        if (rc == ECBOR_END_OF_BUFFER) {
          return ECBOR_ERR_INVALID_END_OF_BUFFER;
        } else {
          return rc;
        }
      }
    }
  }
  
  return ECBOR_OK;
}

ecbor_error_t
ecbor_get_array_item_ptr (ecbor_item_t *array, size_t index,
                          ecbor_item_t **item)
{
  
  size_t i;

  if (!array) {
    return ECBOR_ERR_NULL_ARRAY;
  }
  ECBOR_INTERNAL_CHECK_TYPE (array->type, ECBOR_TYPE_ARRAY);
  ECBOR_INTERNAL_CHECK_BOUNDS (index, array->length);
  ECBOR_INTERNAL_CHECK_ITEM_PTR (item);
  if (!array->child) {
    return ECBOR_ERR_WRONG_MODE;
  }

  (*item) = array->child;
  for (i = 0; i < index; i ++) {
    (*item) = (*item)->next;
    if (!(*item)) {
      /* internal error, this should not have happened */
      return ECBOR_ERR_UNKNOWN;
    }
  }
  
  return ECBOR_OK;
}

ecbor_error_t
ecbor_get_map_item (ecbor_item_t *map, size_t index, ecbor_item_t *key,
                    ecbor_item_t *value)
{
  ecbor_error_t rc;

  if (!map) {
    return ECBOR_ERR_NULL_MAP;
  }

  ECBOR_INTERNAL_CHECK_TYPE (map->type, ECBOR_TYPE_MAP);
  ECBOR_INTERNAL_CHECK_BOUNDS ((index * 2), map->length);
  ECBOR_INTERNAL_CHECK_ITEM_PTR (key);
  ECBOR_INTERNAL_CHECK_ITEM_PTR (value);

  if (map->child) {
    /* parsed in tree mode */
    ecbor_item_t *k, *v;
    rc = ecbor_get_map_item_ptr (map, index, &k, &v);
    if (rc != ECBOR_OK) {
      return rc;
    }

    /* copy contents */
    (*key) = (*k);
    (*value) = (*v);
  } else {
    /* parsed in normal mode */
    ecbor_decode_context_t context;
    size_t i;

    rc = ecbor_initialize_decode (&context, map->value.items, map->size);
    if (rc != ECBOR_OK) {
      return rc;
    }
    
    for (i = 0; i <= index; i ++) {
      rc = ecbor_decode (&context, key);
      if (rc != ECBOR_OK) {
        if (rc == ECBOR_END_OF_BUFFER) {
          return ECBOR_ERR_INVALID_END_OF_BUFFER;
        } else {
          return rc;
        }
      }
      
      rc = ecbor_decode (&context, value);
      if (rc != ECBOR_OK) {
        if (rc == ECBOR_END_OF_BUFFER) {
          return ECBOR_ERR_INVALID_END_OF_BUFFER;
        } else {
          return rc;
        }
      }
    }
  }
  
  return ECBOR_OK;
}

ecbor_error_t
ecbor_get_map_item_ptr (ecbor_item_t *map, size_t index, ecbor_item_t **key,
                        ecbor_item_t **value)
{
  size_t i;

  if (!map) {
    return ECBOR_ERR_NULL_MAP;
  }

  ECBOR_INTERNAL_CHECK_TYPE (map->type, ECBOR_TYPE_MAP);
  ECBOR_INTERNAL_CHECK_BOUNDS ((index * 2), map->length);
  ECBOR_INTERNAL_CHECK_ITEM_PTR (key);
  ECBOR_INTERNAL_CHECK_ITEM_PTR (value);
  if (!map->child) {
    return ECBOR_ERR_WRONG_MODE;
  }

  (*key) = map->child;
  for (i = 0; i < index * 2; i ++) {
    (*key) = (*key)->next;
    if (!(*key)) {
      /* internal error, this should not have happened */
      return ECBOR_ERR_UNKNOWN;
    }
  }
  
  (*value) = (*key)->next;
  if (!(*value)) {
    /* internal error, this should not have happened */
    return ECBOR_ERR_UNKNOWN;
  }

  return ECBOR_OK;
}

ecbor_error_t
ecbor_get_tag_item (ecbor_item_t *tag, ecbor_item_t *item)
{
  ECBOR_INTERNAL_CHECK_ITEM_PTR (tag);
  ECBOR_INTERNAL_CHECK_VALUE_PTR (item);
  ECBOR_INTERNAL_CHECK_TYPE (tag->type, ECBOR_TYPE_TAG);

  if (tag->child) {
    /* parsed in tree mode; copy contents */
    (*item) = (*tag->child);
  } else {
    /* parsed in normal mode */
    ecbor_decode_context_t context;
    ecbor_error_t rc;

    rc = ecbor_initialize_decode (&context, tag->value.tag.child, tag->size);
    if (rc != ECBOR_OK) {
      return rc;
    }

    rc = ecbor_decode (&context, item);
    if (rc != ECBOR_OK) {
      if (rc == ECBOR_END_OF_BUFFER) {
        return ECBOR_ERR_INVALID_END_OF_BUFFER;
      } else {
        return rc;
      }
    }
  }
  
  return ECBOR_OK;
}

ecbor_error_t
ecbor_get_tag_item_ptr (ecbor_item_t *tag, ecbor_item_t **item)
{
  ECBOR_INTERNAL_CHECK_ITEM_PTR (tag);
  ECBOR_INTERNAL_CHECK_VALUE_PTR (item);
  ECBOR_INTERNAL_CHECK_TYPE (tag->type, ECBOR_TYPE_TAG);
  if (!tag->child) {
    return ECBOR_ERR_WRONG_MODE;
  }
  
  (*item) = tag->child;
  return ECBOR_OK;
}


#define ECBOR_GET_INTEGER_INTERNAL(item, value, etype, btype) \
{                                                             \
  ECBOR_INTERNAL_CHECK_ITEM_PTR (item);                       \
  ECBOR_INTERNAL_CHECK_VALUE_PTR (value);                     \
  ECBOR_INTERNAL_CHECK_TYPE ((item)->type, (etype));          \
  (*value) = (btype) item->value.uinteger;                    \
  return (item->size - 1 <= sizeof(btype)                     \
          ? ECBOR_OK                                          \
          : ECBOR_ERR_VALUE_OVERFLOW);                        \
}

ecbor_error_t
ecbor_get_uint8 (ecbor_item_t *item, uint8_t *value)
ECBOR_GET_INTEGER_INTERNAL (item, value, ECBOR_TYPE_UINT, uint8_t)

ecbor_error_t
ecbor_get_uint16 (ecbor_item_t *item, uint16_t *value)
ECBOR_GET_INTEGER_INTERNAL (item, value, ECBOR_TYPE_UINT, uint16_t)

ecbor_error_t
ecbor_get_uint32 (ecbor_item_t *item, uint32_t *value)
ECBOR_GET_INTEGER_INTERNAL (item, value, ECBOR_TYPE_UINT, uint32_t)

ecbor_error_t
ecbor_get_uint64 (ecbor_item_t *item, uint64_t *value)
ECBOR_GET_INTEGER_INTERNAL (item, value, ECBOR_TYPE_UINT, uint64_t)

ecbor_error_t
ecbor_get_int8 (ecbor_item_t *item, int8_t *value)
ECBOR_GET_INTEGER_INTERNAL (item, value, ECBOR_TYPE_NINT, int8_t)

ecbor_error_t
ecbor_get_int16 (ecbor_item_t *item, int16_t *value)
ECBOR_GET_INTEGER_INTERNAL (item, value, ECBOR_TYPE_NINT, int16_t)

ecbor_error_t
ecbor_get_int32 (ecbor_item_t *item, int32_t *value)
ECBOR_GET_INTEGER_INTERNAL (item, value, ECBOR_TYPE_NINT, int32_t)

ecbor_error_t
ecbor_get_int64 (ecbor_item_t *item, int64_t *value)
ECBOR_GET_INTEGER_INTERNAL (item, value, ECBOR_TYPE_NINT, int64_t)

#undef ECBOR_GET_INTEGER_INTERNAL


static __attribute__((noinline)) ecbor_error_t
ecbor_get_string_internal (ecbor_item_t *str, const uint8_t **value,
                           ecbor_type_t type)
{
  ECBOR_INTERNAL_CHECK_ITEM_PTR (str);
  ECBOR_INTERNAL_CHECK_VALUE_PTR (value);
  ECBOR_INTERNAL_CHECK_TYPE (str->type, type);

  if (str->is_indefinite) {
    return ECBOR_ERR_WONT_RETURN_INDEFINITE;
  }
  
  (*value) = (uint8_t *) str->value.string.str;
  
  return ECBOR_OK;
}

static __attribute__((noinline)) ecbor_error_t
ecbor_get_string_chunk_count_internal (ecbor_item_t *str, size_t *count,
                                       ecbor_type_t type)
{
  ECBOR_INTERNAL_CHECK_ITEM_PTR (str);
  ECBOR_INTERNAL_CHECK_VALUE_PTR (count);
  ECBOR_INTERNAL_CHECK_TYPE (str->type, type);

  if (!str->is_indefinite) {
    return ECBOR_ERR_WONT_RETURN_DEFINITE;
  }

  (*count) = str->value.string.n_chunks;

  return ECBOR_OK;
}

static __attribute__((noinline)) ecbor_error_t
ecbor_get_string_chunk_internal (ecbor_item_t *str, size_t index,
                                 ecbor_item_t *chunk, ecbor_type_t type)
{
  ecbor_decode_context_t context;
  ecbor_error_t rc;
  size_t i;

  ECBOR_INTERNAL_CHECK_ITEM_PTR (str);
  ECBOR_INTERNAL_CHECK_VALUE_PTR (chunk);
  ECBOR_INTERNAL_CHECK_TYPE (str->type, type);

  if (!str->is_indefinite) {
    return ECBOR_ERR_WONT_RETURN_DEFINITE;
  }
  ECBOR_INTERNAL_CHECK_BOUNDS (index, str->value.string.n_chunks)

  /* locate chunk */
  rc = ecbor_initialize_decode (&context, str->value.string.str, str->size);
  if (rc != ECBOR_OK) {
    return rc;
  }
  
  for (i = 0; i <= index; i ++) {
    rc = ecbor_decode (&context, chunk);
    if (rc != ECBOR_OK) {
      if (rc == ECBOR_END_OF_BUFFER) {
        return ECBOR_ERR_INVALID_END_OF_BUFFER;
      } else {
        return rc;
      }
    }
  }
  
  /* check chunk */
  if (chunk->type != type) {
    return ECBOR_ERR_INVALID_CHUNK_MAJOR_TYPE;
  }
  if (chunk->is_indefinite) {
    return ECBOR_ERR_NESTET_INDEFINITE_STRING;
  }
  
  return ECBOR_OK;
}

ecbor_error_t
ecbor_get_str (ecbor_item_t *str, const char **value)
{
  return ecbor_get_string_internal (str, (const uint8_t **)value,
                                    ECBOR_TYPE_STR);
}

ecbor_error_t
ecbor_get_str_chunk_count (ecbor_item_t *str, size_t *count)
{
  return ecbor_get_string_chunk_count_internal (str, count, ECBOR_TYPE_STR);
}

ecbor_error_t
ecbor_get_str_chunk (ecbor_item_t *str, size_t index, ecbor_item_t *chunk)
{
  return ecbor_get_string_chunk_internal (str, index, chunk, ECBOR_TYPE_STR);
}

ecbor_error_t
ecbor_get_bstr (ecbor_item_t *str, const uint8_t **value)
{
  return ecbor_get_string_internal (str, value, ECBOR_TYPE_BSTR);
}

ecbor_error_t
ecbor_get_bstr_chunk_count (ecbor_item_t *str, size_t *count)
{
  return ecbor_get_string_chunk_count_internal (str, count, ECBOR_TYPE_BSTR);
}

ecbor_error_t
ecbor_get_bstr_chunk (ecbor_item_t *str, size_t index, ecbor_item_t *chunk)
{
  return ecbor_get_string_chunk_internal (str, index, chunk, ECBOR_TYPE_BSTR);
}


ecbor_error_t
ecbor_get_tag_value (ecbor_item_t *tag, uint64_t *tag_value)
{
  ECBOR_INTERNAL_CHECK_ITEM_PTR (tag);
  ECBOR_INTERNAL_CHECK_VALUE_PTR (tag_value);
  ECBOR_INTERNAL_CHECK_TYPE (tag->type, ECBOR_TYPE_TAG);
  
  (*tag_value) = tag->value.tag.tag_value;
  return ECBOR_OK;
}


ecbor_error_t
ecbor_get_fp32 (ecbor_item_t *item, float *value)
{
  ECBOR_INTERNAL_CHECK_ITEM_PTR (item);
  ECBOR_INTERNAL_CHECK_VALUE_PTR (value);
  ECBOR_INTERNAL_CHECK_TYPE (item->type, ECBOR_TYPE_FP32);
  
  (*value) = item->value.fp32;
  return ECBOR_OK;
}

ecbor_error_t
ecbor_get_fp64 (ecbor_item_t *item, double *value)
{
  ECBOR_INTERNAL_CHECK_ITEM_PTR (item);
  ECBOR_INTERNAL_CHECK_VALUE_PTR (value);
  ECBOR_INTERNAL_CHECK_TYPE (item->type, ECBOR_TYPE_FP64);
  
  (*value) = item->value.fp64;
  return ECBOR_OK;
}


ecbor_error_t
ecbor_get_bool (ecbor_item_t *item, uint8_t *value)
{
  ECBOR_INTERNAL_CHECK_ITEM_PTR (item);
  ECBOR_INTERNAL_CHECK_VALUE_PTR (value);
  ECBOR_INTERNAL_CHECK_TYPE (item->type, ECBOR_TYPE_BOOL);
  
  (*value) = (uint8_t) item->value.uinteger;
  return ECBOR_OK;
}

void
ecbor_memcpy (void *dest, void *src, size_t num)
{
  /* TODO: Replace with memcpy when platform supports */
  /* TODO: Optimize */
  while (num > 4) {
    *((uint32_t *) dest) = *((uint32_t *) src);
    num -= 4;
  }
  while (num) {
    *((uint8_t *) dest) = *((uint8_t *) src);
    num --;
  }
}