/*
 * Copyright (c) 2018 Vasile Vilvoiu <vasi.vilvoiu@gmail.com>
 *
 * libecbor is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include "ecbor.h"

extern ecbor_major_type_t
ecbor_get_type (ecbor_item_t *item)
{
  if (!item) {
    return ECBOR_MT_UNDEFINED;
  }
  if (item->major_type < ECBOR_MT_UINT
      || item->major_type > ECBOR_MT_SPECIAL) {
    return ECBOR_MT_UNDEFINED;
  }
  return item->major_type;
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
  
  switch (item->major_type) {
    case ECBOR_MT_NINT:
      *((int64_t *)value) = item->value.integer;
      break;

    case ECBOR_MT_UINT:
      *((uint64_t *)value) = item->value.uinteger;
      break;

    case ECBOR_MT_BSTR:
    case ECBOR_MT_STR:
      *((uint8_t **)value) = (uint8_t *) item->value.string;
      break;

    case ECBOR_MT_TAG:
      *((uint64_t *)value) = item->value.tag.tag_value;
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
  
  switch (item->major_type) {
    case ECBOR_MT_BSTR:
    case ECBOR_MT_STR:
      *length = item->size;
      break;
    
    case ECBOR_MT_ARRAY:
      *length = item->n_chunks;
      break;

    case ECBOR_MT_MAP:
      *length = item->n_chunks / 2;
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
  if (array->major_type != ECBOR_MT_ARRAY) {
    return ECBOR_ERR_INVALID_TYPE;
  }
  if (array->n_chunks <= index) {
    return ECBOR_ERR_INDEX_OUT_OF_BOUNDS;
  }
  if (!item) {
    return ECBOR_ERR_NULL_ITEM;
  }
  
  rc = ecbor_initialize_decode (&context, array->value.items, array->size);
  if (rc != ECBOR_OK) {
    return rc;
  }
  
  for (i = 0; i < index+1; i ++) {
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
  if (map->major_type != ECBOR_MT_ARRAY) {
    return ECBOR_ERR_INVALID_TYPE;
  }
  if (map->n_chunks <= (index * 2)) {
    return ECBOR_ERR_INDEX_OUT_OF_BOUNDS;
  }
  if (!key || !value) {
    return ECBOR_ERR_NULL_ITEM;
  }
  
  rc = ecbor_initialize_decode (&context, map->value.items, map->size);
  if (rc != ECBOR_OK) {
    return rc;
  }
  
  for (i = 0; i < index; i ++) {
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
  if (tag->major_type != ECBOR_MT_ARRAY) {
    return ECBOR_ERR_INVALID_TYPE;
  }
  if (!item) {
    return ECBOR_ERR_NULL_VALUE;
  }

  rc = ecbor_initialize_decode (&context, tag->value.items, tag->size);
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