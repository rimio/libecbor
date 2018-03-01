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

extern float
ecbor_fp32_from_big_endian (float value)
{
#if __BYTE_ORDER__ && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
  return value;
#elif __BYTE_ORDER__ && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
  uint32_t r = ecbor_uint32_from_big_endian(*((uint32_t *) &value));
  return *((float *) &r);
#else
  #error "Endianness not supported!"
#endif
}

extern double
ecbor_fp64_from_big_endian (double value)
{
#if __BYTE_ORDER__ && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
  return value;
#elif __BYTE_ORDER__ && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
  uint64_t r = ecbor_uint64_from_big_endian(*((uint64_t *) &value));
  return *((double *) &r);
#else
  #error "Endianness not supported!"
#endif
}

extern ecbor_type_t
ecbor_get_type (ecbor_item_t *item)
{
  if (!item) {
    return ECBOR_TYPE_UNDEFINED;
  }
  if (item->type < ECBOR_TYPE_FIRST
      || item->type > ECBOR_TYPE_LAST) {
    return ECBOR_TYPE_UNDEFINED;
  }
  return item->type;
}

extern ecbor_error_t
ecbor_get_value (ecbor_item_t *item, void *value)
{
  if (!item) {
    return ECBOR_ERR_NULL_ITEM;
  }
  if (!value) {
    return ECBOR_ERR_NULL_VALUE;
  }
  
  switch (item->type) {
    case ECBOR_TYPE_NINT:
      *((int64_t *)value) = item->value.integer;
      break;

    case ECBOR_TYPE_UINT:
      *((uint64_t *)value) = item->value.uinteger;
      break;

    case ECBOR_TYPE_BSTR:
    case ECBOR_TYPE_STR:
      if (item->is_indefinite) {
        return ECBOR_ERR_WONT_RETURN_INDEFINITE;
      }
      *((uint8_t **)value) = (uint8_t *) item->value.string.str;
      break;

    case ECBOR_TYPE_TAG:
      *((uint64_t *)value) = item->value.tag.tag_value;
      break;
    
    case ECBOR_TYPE_FP16:
      return ECBOR_ERR_CURRENTLY_NOT_SUPPORTED;

    case ECBOR_TYPE_FP32:
      *((float *)value) = item->value.fp32;
      break;

    case ECBOR_TYPE_FP64:
      *((double *)value) = item->value.fp64;
      break;
    
    case ECBOR_TYPE_BOOL:
      *((uint64_t *)value) = item->value.uinteger;
      break;

    default:
      return ECBOR_ERR_INVALID_TYPE;
  }
  
  return ECBOR_OK;
}

extern ecbor_error_t
ecbor_get_length (ecbor_item_t *item, uint64_t *length)
{
  if (!item) {
    return ECBOR_ERR_NULL_ITEM;
  }
  if (!length) {
    return ECBOR_ERR_NULL_VALUE;
  }
  
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

extern ecbor_error_t
ecbor_get_array_item (ecbor_item_t *array, uint64_t index,
                      ecbor_item_t *item)
{
  ecbor_decode_context_t context;
  ecbor_error_t rc;
  uint64_t i;

  if (!array) {
    return ECBOR_ERR_NULL_ARRAY;
  }
  if (array->type != ECBOR_TYPE_ARRAY) {
    return ECBOR_ERR_INVALID_TYPE;
  }
  if (array->length <= index) {
    return ECBOR_ERR_INDEX_OUT_OF_BOUNDS;
  }
  if (!item) {
    return ECBOR_ERR_NULL_ITEM;
  }
  
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
  
  return ECBOR_OK;
}

extern ecbor_error_t
ecbor_get_map_item (ecbor_item_t *map, uint64_t index, ecbor_item_t *key,
                    ecbor_item_t *value)
{
  ecbor_decode_context_t context;
  ecbor_error_t rc;
  uint64_t i;

  if (!map) {
    return ECBOR_ERR_NULL_MAP;
  }
  if (map->type != ECBOR_TYPE_MAP) {
    return ECBOR_ERR_INVALID_TYPE;
  }
  if (map->length <= (index * 2)) {
    return ECBOR_ERR_INDEX_OUT_OF_BOUNDS;
  }
  if (!key || !value) {
    return ECBOR_ERR_NULL_ITEM;
  }
  
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
  
  return ECBOR_OK;
}

extern ecbor_error_t
ecbor_get_tag_item (ecbor_item_t *tag, ecbor_item_t *item)
{
  ecbor_decode_context_t context;
  ecbor_error_t rc;

  if (!tag) {
    return ECBOR_ERR_NULL_ITEM;
  }
  if (tag->type != ECBOR_TYPE_TAG) {
    return ECBOR_ERR_INVALID_TYPE;
  }
  if (!item) {
    return ECBOR_ERR_NULL_VALUE;
  }

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
  
  return ECBOR_OK;
}

extern ecbor_error_t
ecbor_get_root (ecbor_decode_context_t *context, ecbor_node_t **root)
{
  if (!context) {
    return ECBOR_ERR_NULL_CONTEXT;
  }
  if (!root) {
    return ECBOR_ERR_NULL_NODE;
  }
  if (context->n_nodes <= 0) {
    return ECBOR_ERR_EMPTY_NODE_BUFFER;
  }
  
  *root = &context->nodes[0];
  return ECBOR_OK;
}