/*
 * Copyright (c) 2018 Vasile Vilvoiu <vasi.vilvoiu@gmail.com>
 *
 * libecbor is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include "ecbor.h"
#include "ecbor_internal.h"

ecbor_error_t
ecbor_initialize_encode (ecbor_encode_context_t *context,
                         uint8_t *buffer,
                         size_t buffer_size)
{
  ECBOR_INTERNAL_CHECK_CONTEXT_PTR (context);
  if (!buffer) {
    return ECBOR_ERR_NULL_OUTPUT_BUFFER;
  }
  
  context->base = buffer;
  context->out_position = buffer;
  context->bytes_left = buffer_size;
  context->mode = ECBOR_MODE_ENCODE;
  
  return ECBOR_OK;
}

ecbor_error_t
ecbor_initialize_encode_streamed (ecbor_encode_context_t *context,
                                  uint8_t *buffer,
                                  size_t buffer_size)
{
  ECBOR_INTERNAL_CHECK_CONTEXT_PTR (context);
  if (!buffer) {
    return ECBOR_ERR_NULL_OUTPUT_BUFFER;
  }
  
  context->base = buffer;
  context->out_position = buffer;
  context->bytes_left = buffer_size;
  context->mode = ECBOR_MODE_ENCODE_STREAMED;
  
  return ECBOR_OK;
}


ecbor_error_t
ecbor_get_encoded_buffer_size(const ecbor_encode_context_t *context, size_t *out_size)
{
  /* check parameters */
  if (context == NULL) {
    return ECBOR_ERR_NULL_CONTEXT;
  }
  if (out_size == NULL) {
    return ECBOR_ERR_NULL_PARAMETER;
  }
  /* check sanity */
  if (context->base > context->out_position) {
    return ECBOR_ERR_INVALID_END_OF_BUFFER;
  }
  /* return length */
  *out_size = context->out_position - context->base;
  return ECBOR_OK;
}

static ecbor_error_t
ecbor_encode_uint (ecbor_encode_context_t *context, uint8_t major_type,
                   uint64_t value)
{
  uint8_t size = 0;

  /* compute storage size */
  if (value <= ECBOR_ADDITIONAL_LAST_INTEGER) {
    size = 1;
  } else if (value <= 0xFF) {
    size = 2;
  } else if (value <= 0xFFFF) {
    size = 3;
  } else if (value <= 0xFFFFFFFF) {
    size = 5;
  } else {
    size = 9;
  }

  /* check buffer */
  if (context->bytes_left < size) {
    return ECBOR_ERR_INVALID_END_OF_BUFFER;
  }

  /* write value */
  if (value <= ECBOR_ADDITIONAL_LAST_INTEGER) {
    /* value coded in header */
    (*context->out_position) = ((major_type & 0x7) << 5) | (value & 0x1f);
    context->out_position ++;
    context->bytes_left --;
  } else if (value <= 0xFF) {
    /* 1 extra byte */
    (*context->out_position) =
      ((major_type & 0x7) << 5) | ECBOR_ADDITIONAL_1BYTE;
    context->out_position ++;
    (*context->out_position) = (uint8_t) value;
    context->out_position ++;
    context->bytes_left -= 2;
  } else if (value <= 0xFFFF) {
    /* 2 extra bytes */
    (*context->out_position) =
      ((major_type & 0x7) << 5) | ECBOR_ADDITIONAL_2BYTE;
    context->out_position ++;
    (*((uint16_t *)context->out_position)) =
      ecbor_uint16_to_big_endian ((uint16_t) value);
    context->out_position += 2;
    context->bytes_left -= 3;
  } else if (value <= 0xFFFFFFFF) {
    /* 4 extra bytes */
    (*context->out_position) =
      ((major_type & 0x7) << 5) | ECBOR_ADDITIONAL_4BYTE;
    context->out_position ++;
    (*((uint32_t *)context->out_position)) =
      ecbor_uint32_to_big_endian ((uint32_t) value);
    context->out_position += 4;
    context->bytes_left -= 5;
  } else {
    /* 8 extra bytes */
    (*context->out_position) =
      ((major_type & 0x7) << 5) | ECBOR_ADDITIONAL_8BYTE;
    context->out_position ++;
    (*((uint64_t *)context->out_position)) =
      ecbor_uint64_to_big_endian ((uint64_t) value);
    context->out_position += 8;
    context->bytes_left -= 9;
  }

  return ECBOR_OK;
}

static ecbor_error_t
ecbor_encode_header (ecbor_encode_context_t *context, uint8_t major_type,
                     uint8_t additional)
{
  if (context->bytes_left < 1) {
    return ECBOR_ERR_INVALID_END_OF_BUFFER;
  }

  (*context->out_position) =
    ((((uint8_t)major_type) & 0x7) << 5) | (additional & 0x1f);
  context->out_position ++;
  context->bytes_left --;
  
  return ECBOR_OK;
}

ecbor_error_t
ecbor_encode (ecbor_encode_context_t *context, ecbor_item_t *item)
{
  ecbor_error_t rc;

  ECBOR_INTERNAL_CHECK_CONTEXT_PTR (context);
  ECBOR_INTERNAL_CHECK_ITEM_PTR (item);

  if (context->bytes_left == 0) {
    return ECBOR_ERR_INVALID_END_OF_BUFFER;
  }
  if (context->mode != ECBOR_MODE_ENCODE
      && context->mode != ECBOR_MODE_ENCODE_STREAMED) {
    /* only allow known modes of operation; junk in <mode> will generate
       undefined behaviour */
    return ECBOR_ERR_WRONG_MODE;
  }

  switch (item->type) {
    case ECBOR_TYPE_UINT:
      rc = ecbor_encode_uint (context, ECBOR_TYPE_UINT, item->value.uinteger);
      if (rc != ECBOR_OK) {
        return rc;
      }
      break;
    
    case ECBOR_TYPE_NINT:
      {
        uint64_t value = (-1) - item->value.integer;
        rc = ecbor_encode_uint (context, ECBOR_TYPE_NINT, value);
        if (rc != ECBOR_OK) {
          return rc;
        }
      }
      break;

    case ECBOR_TYPE_BSTR:
    case ECBOR_TYPE_STR:
      if (item->is_indefinite) {
        if (context->mode == ECBOR_MODE_ENCODE) {
          /* we do not support indefinite strings in non-stream mode */
          return ECBOR_ERR_WONT_ENCODE_INDEFINITE;
        }

        /* write just header */
        rc = ecbor_encode_header (context, item->type,
                                  ECBOR_ADDITIONAL_INDEFINITE);
        if (rc != ECBOR_OK) {
          return rc;
        }
      } else {
        /* write header */
        rc = ecbor_encode_uint (context, item->type, item->length);
        if (rc != ECBOR_OK) {
          return rc;
        }

        if (context->mode == ECBOR_MODE_ENCODE && item->length > 0) {
          /* write string */
          if (!item->value.string.str) {
            return ECBOR_ERR_NULL_VALUE;
          }
          if (context->bytes_left < item->length) {
            return ECBOR_ERR_INVALID_END_OF_BUFFER;
          }

          ecbor_memcpy ((void *) context->out_position,
                        (void *) item->value.string.str,
                        item->length);
          context->out_position += item->length;
          context->bytes_left -= item->length;
        }
      }
      break;
    
    case ECBOR_TYPE_ARRAY:
    case ECBOR_TYPE_MAP:
      if (item->is_indefinite) {
        if (context->mode == ECBOR_MODE_ENCODE) {
          /* we do not support indefinite maps and arrays in non-stream mode */
          return ECBOR_ERR_WONT_ENCODE_INDEFINITE;
        }

        /* write just header */
        rc = ecbor_encode_header (context, item->type,
                                  ECBOR_ADDITIONAL_INDEFINITE);
        if (rc != ECBOR_OK) {
          return rc;
        }
      } else {
        size_t written_len;
        
        if (item->type == ECBOR_TYPE_MAP) {
          if (item->length % 2) {
            return ECBOR_ERR_INVALID_KEY_VALUE_PAIR;
          }
          written_len = item->length / 2;
        } else {
          written_len = item->length;
        }

        /* write header */
        rc = ecbor_encode_uint (context, item->type, written_len);
        if (rc != ECBOR_OK) {
          return rc;
        }

        if (context->mode == ECBOR_MODE_ENCODE && item->length > 0) {
          size_t remaining = item->length;
          ecbor_item_t *current = item->child;
          
          while (remaining--) {
            /* write item */
            if (!current) {
              return ECBOR_ERR_NULL_ITEM;
            }

            rc = ecbor_encode (context, current);
            if (rc != ECBOR_OK) {
              return rc;
            }

            current = current->next;
          }
        }
      }
      break;

    case ECBOR_TYPE_TAG:
      /* write header */
      rc = ecbor_encode_uint (context, item->type, item->value.tag.tag_value);
      if (rc != ECBOR_OK) {
        return rc;
      }

      if (context->mode == ECBOR_MODE_ENCODE) {
        /* write tagged item */
        ECBOR_INTERNAL_CHECK_ITEM_PTR (item->child);

        rc = ecbor_encode (context, item->child);
        if (rc != ECBOR_OK) {
          return rc;
        }
      }
      break;

    case ECBOR_TYPE_STOP_CODE:
      rc = ecbor_encode_header (context, ECBOR_TYPE_SPECIAL,
                                ECBOR_ADDITIONAL_INDEFINITE);
      if (rc != ECBOR_OK) {
        return rc;
      }
      break;

    case ECBOR_TYPE_FP32:
      {
        float value_bigend =
          ecbor_fp32_to_big_endian (item->value.fp32);

        /* write header */
        rc = ecbor_encode_header (context, ECBOR_TYPE_SPECIAL,
                                  ECBOR_ADDITIONAL_4BYTE);
        if (rc != ECBOR_OK) {
          return rc;
        }

        /* write value */
        if (context->bytes_left < sizeof (float)) {
          return ECBOR_ERR_INVALID_END_OF_BUFFER;
        }
        (*((float *)context->out_position)) = value_bigend;
        context->out_position += sizeof (float);
        context->bytes_left -= sizeof (float);
      }
      break;

    case ECBOR_TYPE_FP64:
      {
        double value_bigend =
          ecbor_fp64_to_big_endian (item->value.fp64);

        /* write header */
        rc = ecbor_encode_header (context, ECBOR_TYPE_SPECIAL,
                                  ECBOR_ADDITIONAL_8BYTE);
        if (rc != ECBOR_OK) {
          return rc;
        }

        /* write value */
        if (context->bytes_left < sizeof (double)) {
          return ECBOR_ERR_INVALID_END_OF_BUFFER;
        }
        (*((double *)context->out_position)) = value_bigend;
        context->out_position += sizeof (double);
        context->bytes_left -= sizeof (double);
      }
      break;

    case ECBOR_TYPE_BOOL:
      rc = ecbor_encode_header (context, ECBOR_TYPE_SPECIAL,
                                (item->value.integer ? ECBOR_SIMPLE_TRUE
                                                     : ECBOR_SIMPLE_FALSE));
      if (rc != ECBOR_OK) {
        return rc;
      }
      break;

    case ECBOR_TYPE_NULL:
      rc = ecbor_encode_header (context, ECBOR_TYPE_SPECIAL,
                                ECBOR_SIMPLE_NULL);
      if (rc != ECBOR_OK) {
        return rc;
      }
      break;

    case ECBOR_TYPE_UNDEFINED:
      rc = ecbor_encode_header (context, ECBOR_TYPE_SPECIAL,
                                ECBOR_SIMPLE_UNDEFINED);
      if (rc != ECBOR_OK) {
        return rc;
      }
      break;
    
    default:
      return ECBOR_ERR_INVALID_TYPE;
  }
  
  return ECBOR_OK;
}

ecbor_item_t
ecbor_int (int64_t value)
{
  ecbor_item_t r = null_item;
  if (value >= 0) {
    r.type = ECBOR_TYPE_UINT;
    r.value.uinteger = (uint64_t) value;
  } else {
    r.type = ECBOR_TYPE_NINT;
    r.value.integer = value;
  }
  return r;
}

ecbor_item_t
ecbor_uint (uint64_t value)
{
  ecbor_item_t r = null_item;
  r.type = ECBOR_TYPE_UINT;
  r.value.uinteger = value;
  return r;
}

ecbor_item_t
ecbor_bstr (const uint8_t *bstr, size_t length)
{
  ecbor_item_t r = null_item;
  r.type = ECBOR_TYPE_BSTR;
  r.value.string.str = bstr;
  r.length = length;
  return r;
}

ecbor_item_t
ecbor_str (const char *str, size_t length)
{
  ecbor_item_t r = null_item;
  r.type = ECBOR_TYPE_STR;
  r.value.string.str = (uint8_t *)str;
  r.length = length;
  return r;
}

ecbor_item_t
ecbor_tag (ecbor_item_t *child, uint64_t tag_value)
{
  ecbor_item_t r = null_item;
  r.type = ECBOR_TYPE_TAG;
  r.child = child;
  r.value.tag.tag_value = tag_value;
  return r;
}

ecbor_item_t
ecbor_fp32 (float value)
{
  ecbor_item_t r = null_item;
  r.type = ECBOR_TYPE_FP32;
  r.value.fp32 = value;
  return r;
}

ecbor_item_t
ecbor_fp64 (double value)
{
  ecbor_item_t r = null_item;
  r.type = ECBOR_TYPE_FP64;
  r.value.fp64 = value;
  return r;
}

ecbor_item_t
ecbor_bool (uint8_t value)
{
  ecbor_item_t r = null_item;
  r.type = ECBOR_TYPE_BOOL;
  r.value.integer = value;
  return r;
}

ecbor_item_t
ecbor_null (void)
{
  ecbor_item_t r = null_item;
  r.type = ECBOR_TYPE_NULL;
  return r;
}

ecbor_item_t
ecbor_undefined (void)
{
  ecbor_item_t r = null_item;
  r.type = ECBOR_TYPE_UNDEFINED;
  return r;
}

ecbor_item_t
ecbor_array_token (size_t length)
{
  ecbor_item_t r = null_item;
  r.type = ECBOR_TYPE_ARRAY;
  r.length = length;
  return r;
}

ecbor_item_t
ecbor_indefinite_array_token (void)
{
  ecbor_item_t r = null_item;
  r.type = ECBOR_TYPE_ARRAY;
  r.is_indefinite = true;
  return r;
}

ecbor_item_t
ecbor_map_token (size_t length)
{
  ecbor_item_t r = null_item;
  r.type = ECBOR_TYPE_MAP;
  r.length = length;
  return r;
}

ecbor_item_t
ecbor_indefinite_map_token (void)
{
  ecbor_item_t r = null_item;
  r.type = ECBOR_TYPE_MAP;
  r.is_indefinite = true;
  return r;
}

ecbor_item_t
ecbor_stop_code (void)
{
  ecbor_item_t r = null_item;
  r.type = ECBOR_TYPE_STOP_CODE;
  return r;
}


/* Array and map builders */
ecbor_error_t
ecbor_array (ecbor_item_t *array, ecbor_item_t *items, size_t length)
{
  size_t i;

  if (!array) {
    return ECBOR_ERR_NULL_ARRAY;
  }
  
  array->type = ECBOR_TYPE_ARRAY;
  array->length = length;
  array->child = items;
  
  if (length > 0) {
    ECBOR_INTERNAL_CHECK_ITEM_PTR (items);
    items->parent = array;

    for (i = 1; i < length; i ++) {
      items->next = (items + 1);
      items ++;
    }
  }
  
  return ECBOR_OK;
}

ecbor_error_t
ecbor_map (ecbor_item_t *map, ecbor_item_t *keys, ecbor_item_t *values,
           size_t length)
{
  size_t i;

  if (!map) {
    return ECBOR_ERR_NULL_MAP;
  }

  map->type = ECBOR_TYPE_MAP;
  map->length = length;

  if (length > 0) {
    ECBOR_INTERNAL_CHECK_ITEM_PTR (keys);
    ECBOR_INTERNAL_CHECK_ITEM_PTR (values);

    map->child = keys;
    keys->parent = map;
    keys->next = values;
    values->parent = map;
    keys ++;
    values ++;

    for (i = 1; i < length; i ++) {
      values->next = (keys + 1);
      keys ++;
      keys->next = (values + 1);
      values ++;
    }
  }

  return ECBOR_OK;
}
