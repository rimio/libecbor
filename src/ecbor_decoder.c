/*
 * Copyright (c) 2018 Vasile Vilvoiu <vasi.vilvoiu@gmail.com>
 *
 * libecbor is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include "ecbor.h"
#include "ecbor_internal.h"

static ecbor_item_t null_item = {
  .type = ECBOR_TYPE_NONE,
  .value = {
    .tag = {
      .tag_value = 0,
      .child = NULL
    }
  },
  .size = 0,
  .length = 0,
  .is_indefinite = 0,
  .parent = NULL,
  .child = NULL,
  .next = NULL,
  .index = 0
};

static ecbor_error_t
ecbor_initialize_decode_internal (ecbor_decode_context_t *context,
                                  const uint8_t *buffer,
                                  size_t buffer_size)
{
  if (!context) {
    return ECBOR_ERR_NULL_CONTEXT;
  }
  if (!buffer) {
    return ECBOR_ERR_NULL_INPUT_BUFFER;
  }

  context->in_position = buffer;
  context->bytes_left = buffer_size;
  
  return ECBOR_OK;
}

ecbor_error_t
ecbor_initialize_decode (ecbor_decode_context_t *context,
                         const uint8_t *buffer,
                         size_t buffer_size)
{
  ecbor_error_t rc =
    ecbor_initialize_decode_internal (context, buffer, buffer_size);

  if (rc != ECBOR_OK) {
    return rc;
  }

  context->mode = ECBOR_MODE_DECODE;
  context->items = NULL;
  context->item_capacity = 0;
  context->n_items = 0;
  
  return ECBOR_OK;
}

ecbor_error_t
ecbor_initialize_decode_streamed (ecbor_decode_context_t *context,
                                  const uint8_t *buffer,
                                  size_t buffer_size)
{
  ecbor_error_t rc =
    ecbor_initialize_decode_internal (context, buffer, buffer_size);

  if (rc != ECBOR_OK) {
    return rc;
  }
  
  context->mode = ECBOR_MODE_DECODE_STREAMED;
  context->items = NULL;
  context->item_capacity = 0;
  context->n_items = 0;
  
  return ECBOR_OK;
}

ecbor_error_t
ecbor_initialize_decode_tree (ecbor_decode_context_t *context,
                              const uint8_t *buffer,
                              size_t buffer_size,
                              ecbor_item_t *item_buffer,
                              size_t item_capacity)
{
  ecbor_error_t rc =
    ecbor_initialize_decode_internal (context, buffer, buffer_size);

  if (rc != ECBOR_OK) {
    return rc;
  }
  
  if (!item_buffer) {
    return ECBOR_ERR_NULL_ITEM_BUFFER;
  }
  
  context->mode = ECBOR_MODE_DECODE_TREE;
  context->items = item_buffer;
  context->item_capacity = item_capacity;
  context->n_items = 0;
  
  return ECBOR_OK;
}

static inline ecbor_error_t
ecbor_decode_uint (ecbor_decode_context_t *context,
                   uint64_t *value,
                   size_t *size,
                   uint8_t additional)
{
  if (additional < 24) {
    /* value stored in additional information */
    (*value) = additional;
    (*size) = 1;
  } else {
    /* compute storage size */
    (*size) = (0x1 << (additional - ECBOR_ADDITIONAL_1BYTE));
    if (context->bytes_left < (*size)) {
      return ECBOR_ERR_INVALID_END_OF_BUFFER;
    }

    /* read actual value */
    switch (additional) {
      case ECBOR_ADDITIONAL_1BYTE:
        (*value) = *((uint8_t *) context->in_position);
        break;

      case ECBOR_ADDITIONAL_2BYTE:
        (*value) =
          ecbor_uint16_from_big_endian (*((uint16_t *) context->in_position));
        break;

      case ECBOR_ADDITIONAL_4BYTE:
        (*value) =
          ecbor_uint32_from_big_endian (*((uint32_t *) context->in_position));
        break;

      case ECBOR_ADDITIONAL_8BYTE:
        (*value) =
          ecbor_uint64_from_big_endian (*((uint64_t *) context->in_position));
        break;

      default:
        return ECBOR_ERR_INVALID_ADDITIONAL;
    }
    
    /* advance buffer */
    context->in_position += (*size);
    context->bytes_left -= (*size);
    
    /* meter the first byte */
    (*size) ++;
  }
  
  return ECBOR_OK;
}

static inline ecbor_error_t
ecbor_decode_fp32 (ecbor_decode_context_t *context,
                   float *value,
                   size_t *size)
{
  /* compute storage size */
  (*size) = sizeof (float);
  if (context->bytes_left < (*size)) {
    return ECBOR_ERR_INVALID_END_OF_BUFFER;
  }

  /* read actual value */
  (*value) =
    ecbor_fp32_from_big_endian (*((float *) context->in_position));
  
  /* advance buffer */
  context->in_position += (*size);
  context->bytes_left -= (*size);
  
  /* meter the first byte */
  (*size) ++;
  
  return ECBOR_OK;
}

static inline ecbor_error_t
ecbor_decode_fp64 (ecbor_decode_context_t *context,
                   double *value,
                   size_t *size)
{
  /* compute storage size */
  (*size) = sizeof (double);
  if (context->bytes_left < (*size)) {
    return ECBOR_ERR_INVALID_END_OF_BUFFER;
  }

  /* read actual value */
  (*value) =
    ecbor_fp64_from_big_endian (*((double *) context->in_position));

  /* advance buffer */
  context->in_position += (*size);
  context->bytes_left -= (*size);
  
  /* meter the first byte */
  (*size) ++;
  
  return ECBOR_OK;
}

static inline ecbor_error_t
ecbor_decode_simple_value (ecbor_item_t *item)
{
  switch (item->value.uinteger) {
    case ECBOR_SIMPLE_FALSE:
    case ECBOR_SIMPLE_TRUE:
      item->type = ECBOR_TYPE_BOOL;
      item->value.uinteger =
        (item->value.uinteger == ECBOR_SIMPLE_FALSE ? 0 : 1);
      break;
    
    case ECBOR_SIMPLE_NULL:
      item->type = ECBOR_TYPE_NULL;
      break;

    case ECBOR_SIMPLE_UNDEFINED:
      item->type = ECBOR_TYPE_UNDEFINED;
      break;
    
    default:
      return ECBOR_ERR_CURRENTLY_NOT_SUPPORTED;
  }
  
  return ECBOR_OK;
}

static __attribute__((noinline)) ecbor_error_t
ecbor_decode_next_internal (ecbor_decode_context_t *context,
                            ecbor_item_t *item,
                            int8_t is_chunk,
                            ecbor_type_t chunk_mtype)
{
  uint8_t additional;

  if (context->bytes_left == 0) {
    return ECBOR_END_OF_BUFFER;
  }
  if (context->mode != ECBOR_MODE_DECODE
      && context->mode != ECBOR_MODE_DECODE_STREAMED) {
    /* only allow known modes of operation; junk in <mode> will generate
       undefined behaviour */
    return ECBOR_ERR_WRONG_MODE;
  }
  
  /* clear item, just so we do not leave garbage on partial read */
  (*item) = null_item;
  
  /* extract major type (most significant three bits) and additional info */
  item->type = (*context->in_position >> 5) & 0x07;
  additional = (*context->in_position & 0x1f);
  context->in_position ++; context->bytes_left --;
  
  /* check mandatory major type (in case we are reading string chunks);
     we do not want to continue parsing a malformed indefinite string and
     potentially explode the stack with subsequent calls */
  if (is_chunk && chunk_mtype != item->type) {
    if (item->type == (ecbor_type_t) ECBOR_TYPE_SPECIAL
        && additional == ECBOR_ADDITIONAL_INDEFINITE) {
      /* this is a valid stop code, pass it directly; note that this branch is
         only taken when inside an indefinite string */
      return ECBOR_END_OF_INDEFINITE;
    } else {
      /* this is not a stop code, and the item has the wrong major type */
      return ECBOR_ERR_INVALID_CHUNK_MAJOR_TYPE;
    }
  }
  
  /* handle type */
  switch (item->type) {

    /*
     * Integer types
     */
    case ECBOR_TYPE_UINT:
      return ecbor_decode_uint (context, &item->value.uinteger, &item->size,
                                additional);
    
    case ECBOR_TYPE_NINT:
      {
        /* read negative value as unsigned */
        ecbor_error_t rc = ecbor_decode_uint (context, &item->value.uinteger,
                                              &item->size, additional);
        if (rc != ECBOR_OK) {
          return rc;
        }

        /* parse as negative */
        item->value.integer = (-1) - item->value.uinteger;
      }
      break;

    /*
     * String types
     */
    case ECBOR_TYPE_BSTR:
    case ECBOR_TYPE_STR:
      /* keep buffer pointer from current pointer */
      item->value.string.str = context->in_position;
      item->value.string.n_chunks = 0;

      /* discriminate between definite and indefinite strings; we do not treat
       * indefinite strings as we do indefinite maps and arrays, in the sense
       * that we do not allow the user to manually walk each chunk
       */
      if (additional == ECBOR_ADDITIONAL_INDEFINITE) {
        ecbor_item_t chunk;
        ecbor_error_t rc;
        
        /* mark accordingly */
        item->is_indefinite = true;
        item->size = 1; /* already processed first byte */
        
        /* do not allow nested indefinite length strings */
        if (is_chunk) {
          return ECBOR_ERR_NESTET_INDEFINITE_STRING;
        }

        /* indefinite lenght string; read through all blocks to compute size */
        item->value.string.str = context->in_position;
        
        while (true) {
          /* read next chunk */
          rc = ecbor_decode_next_internal (context, &chunk, true,
                                           item->type);
          if (rc != ECBOR_OK) {
            if (rc == ECBOR_END_OF_INDEFINITE) {
              /* stop code found, break from loop */
              item->size += chunk.size; /* meter stop code as well */
              break;
            } else if (rc == ECBOR_END_OF_BUFFER) {
              /* treat a valid end of buffer as invalid since we did not yet
                 find the stop code */
              return ECBOR_ERR_INVALID_END_OF_BUFFER;
            } else if (rc != ECBOR_OK) {
              /* other error */
              return rc;
            }
          }

          /* add chunk size and length to item */
          item->size += chunk.size;
          item->length += chunk.length;
          item->value.string.n_chunks ++;
        }
      } else {
        /* read size of buffer */
        uint64_t len;
        ecbor_error_t rc = ecbor_decode_uint (context, &len, &item->size,
                                              additional);
        if (rc != ECBOR_OK) {
          return rc;
        }
        /* if sizeof(size_t) < sizeof(uint64_t), and payload is >4GB, we're
           fucked */
        item->length = len;

        /* advance */
        if (context->bytes_left < item->length) {
          return ECBOR_ERR_INVALID_END_OF_BUFFER;
        }
        context->in_position += item->length;
        context->bytes_left -= item->length;
        
        /* meter length of string to size of item */
        item->size += item->length;
      }
      break;

    /*
     * Arrays and maps
     */
    case ECBOR_TYPE_ARRAY:
    case ECBOR_TYPE_MAP:
      /* discriminate between definite and indefinite maps and arrays */
      if (additional == ECBOR_ADDITIONAL_INDEFINITE) {
        /* mark accordingly */
        item->is_indefinite = true;
        item->size = 1; /* already processed first byte */
        
        /* keep buffer pointer from current pointer */
        item->value.items = context->in_position;

        if (context->mode != ECBOR_MODE_DECODE_STREAMED) {
          /* we have an indefinite map or array and we're not in streamed mode;
             we have to walk children to compute size and advance to next item */
          ecbor_item_t child;
          ecbor_error_t rc;

          while (true) {
            /* read next chunk */
            rc = ecbor_decode_next_internal (context, &child, false,
                                             ECBOR_TYPE_NONE);
            if (rc != ECBOR_OK) {
              if (rc == ECBOR_END_OF_INDEFINITE) {
                /* stop code found, break from loop */
                item->size += child.size; /* meter stop code as well */
                break;
              } else if (rc == ECBOR_END_OF_BUFFER) {
                /* treat a valid end of buffer as invalid since we did not yet
                   find the stop code */
                return ECBOR_ERR_INVALID_END_OF_BUFFER;
              } else if (rc != ECBOR_OK) {
                /* other error */
                return rc;
              }
            }

            /* add chunk size to item size */
            item->size += child.size;
            item->length ++;
          }
          
          if ((item->type == ECBOR_TYPE_MAP) && (item->length % 2 != 0)) {
            /* incomplete key-value pair; we expect maps to have even number of
               items */
            return ECBOR_ERR_INVALID_KEY_VALUE_PAIR;
          }
        }
      } else {
        uint64_t len;

        /* read size of map or array */
        ecbor_error_t rc = ecbor_decode_uint (context, &len,
                                              &item->size, additional);
        if (rc != ECBOR_OK) {
          return rc;
        }
        
        /* improbable to have a map larger than 4M, but if we do and size_t is
           less than 64bit, this will create problems */
        item->length = len;
        
        /* keep buffer pointer from current pointer */
        item->value.items = context->in_position;

        if (item->type == ECBOR_TYPE_MAP) {
          /* we keep the total number of items in length, yet the map has the
             number of key-value pairs encoded in the length */
          item->length *= 2;
        }

        if (context->mode != ECBOR_MODE_DECODE_STREAMED) {
          ecbor_item_t child;
          ecbor_error_t rc;
          size_t child_no;

          /* not in streamed mode; compute size so we can advance */
          for (child_no = 0; child_no < item->length; child_no ++) {
            /* read next child */
            rc = ecbor_decode_next_internal (context, &child, false,
                                             ECBOR_TYPE_NONE);
            if (rc != ECBOR_OK) {
              if (rc == ECBOR_END_OF_INDEFINITE) {
                /* stop code found, but none is expected */
                return ECBOR_ERR_INVALID_STOP_CODE;
              } else if (rc == ECBOR_END_OF_BUFFER) {
                /* treat a valid end of buffer as invalid since we did not yet
                   reach the end of the map or array */
                return ECBOR_ERR_INVALID_END_OF_BUFFER;
              } else if (rc != ECBOR_OK) {
                /* other error */
                return rc;
              }
            }

            /* add child size to item size */
            item->size += child.size;
          }
        }
      }
      break;

    case ECBOR_TYPE_TAG:
      {
        ecbor_error_t rc;

        /* decode tag */
        rc = ecbor_decode_uint (context, &item->value.tag.tag_value,
                                &item->size, additional);
        if (rc != ECBOR_OK) {
          return rc;
        }
        
        /* keep child pointer */
        item->value.tag.child = context->in_position;
        item->length = 1;
        
        if (context->mode != ECBOR_MODE_DECODE_STREAMED) {
          ecbor_item_t child;

          /* not in streamed mode; compute size so we can advance */
          rc = ecbor_decode_next_internal (context, &child, false,
                                           ECBOR_TYPE_NONE);
          if (rc != ECBOR_OK) {
            return rc;
          }

          /* add child size to item size */
          item->size += child.size;
        }
      }
      break;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
    case ECBOR_TYPE_SPECIAL:
#pragma GCC diagnostic pop
      if (additional == ECBOR_ADDITIONAL_INDEFINITE) {
        /* stop code */
        item->size = 1;
        return ECBOR_END_OF_INDEFINITE;
      } else if (additional <= ECBOR_ADDITIONAL_1BYTE) {
        ecbor_error_t rc;
        /* read integer simple value */
        rc = ecbor_decode_uint (context, &item->value.uinteger, &item->size,
                                additional);
        if (rc != ECBOR_OK) {
          return rc;
        }
        
        /* parse simple value */
        return ecbor_decode_simple_value (item);
      } else if (additional == ECBOR_ADDITIONAL_2BYTE) {
        /* currently we do not support half float */
        return ECBOR_ERR_CURRENTLY_NOT_SUPPORTED;
      } else if (additional == ECBOR_ADDITIONAL_4BYTE) {
        /* floating point 32bit */
        item->type = ECBOR_TYPE_FP32;
        return ecbor_decode_fp32 (context, &item->value.fp32, &item->size);
      } else if (additional == ECBOR_ADDITIONAL_8BYTE) {
        /* floating point 64bit */
        item->type = ECBOR_TYPE_FP64;
        return ecbor_decode_fp64 (context, &item->value.fp64, &item->size);
      } else {
        /* currently unassigned according to RFC */
        return ECBOR_ERR_CURRENTLY_NOT_SUPPORTED;
      }
      break;
    
    default:
      /* while this would theoretically be a case for invalid type, any 3 bit
         value should be a valid type, so this branch is an internal error and
         should never happen */
      return ECBOR_ERR_UNKNOWN;
  }
  
  return ECBOR_OK;
}

ecbor_error_t
ecbor_decode (ecbor_decode_context_t *context, ecbor_item_t *item)
{  
  if (!context) {
    return ECBOR_ERR_NULL_CONTEXT;
  }
  if (!item) {
    return ECBOR_ERR_NULL_ITEM;
  }

  if (context->mode != ECBOR_MODE_DECODE
      && context->mode != ECBOR_MODE_DECODE_STREAMED) {
    /* context is for wrong mode */
    return ECBOR_ERR_WRONG_MODE;
  }
  
  /* we just get the next item */
  return ecbor_decode_next_internal (context, item, false, ECBOR_TYPE_NONE);
}

extern ecbor_error_t
ecbor_decode_tree (ecbor_decode_context_t *context, ecbor_item_t **root)
{
  enum {
    CONSUME_NODE = 0,
    ANALYZE_STOP_CODE,
    LINK_FIRST_NODE,
    LINK_NODE,
    CHECK_END_OF_DEFINITE,
    END
  } state = CONSUME_NODE;
  uint8_t last_was_stop_code = 0;
  ecbor_error_t rc = ECBOR_OK;
  ecbor_item_t *curr_node = NULL, *new_node = NULL;
  
  if (!context) {
    return ECBOR_ERR_NULL_CONTEXT;
  }
  if (!root) {
    return ECBOR_ERR_NULL_ITEM;
  }
  if (context->mode != ECBOR_MODE_DECODE_TREE) {
    return ECBOR_ERR_WRONG_MODE;
  }
  
  /* initialization */
  context->n_items = 0;
  (*root) = NULL;
  
  /* step into streamed mode; some of the semantic checks will be done here */
  context->mode = ECBOR_MODE_DECODE_STREAMED;

  /* consume until end of buffer or error */
  while (state != END) {

    /* state change */
    switch (state) {
      case CONSUME_NODE:
        /* allocate new node */
        if (context->n_items >= context->item_capacity) {
          rc = ECBOR_ERR_END_OF_ITEM_BUFFER;
          goto end;
        }

        new_node = &context->items[context->n_items];
        context->n_items ++;

        /* consume next item */
        rc = ecbor_decode_next_internal (context, new_node, false,
                                         ECBOR_TYPE_NONE);
        if (rc == ECBOR_END_OF_INDEFINITE) {
          state = ANALYZE_STOP_CODE;
          context->n_items --;
          rc = ECBOR_OK;
        } else if (rc == ECBOR_END_OF_BUFFER) {
          /* check for bad end scenatios */
          if (!curr_node) {
            /* first node was stop code, bad end */
            rc = ECBOR_ERR_INVALID_END_OF_BUFFER;
            goto end;
          }
          if (curr_node->parent) {
            /* we're not top level, bad end */
            rc = ECBOR_ERR_INVALID_END_OF_BUFFER;
            goto end;
          }
          if (ECBOR_IS_TAG (curr_node) && !curr_node->child) {
            /* unfinished tag, bad end */
            rc = ECBOR_ERR_INVALID_END_OF_BUFFER;
            goto end;
          }
          if (ECBOR_IS_MAP (curr_node) || ECBOR_IS_ARRAY (curr_node)) {
            if (ECBOR_IS_DEFINITE (curr_node) && !curr_node->child
                && curr_node->length > 0) {
              /* unfinished definite array or map, bad end */
              rc = ECBOR_ERR_INVALID_END_OF_BUFFER;
              goto end;
            }
            if (ECBOR_IS_INDEFINITE (curr_node) && !last_was_stop_code) {
              /* unfinished indefinite array or map, bad end */
              rc = ECBOR_ERR_INVALID_END_OF_BUFFER;
              goto end;
            }
          }
          /* done! */
          state = END;
        } else if (rc == ECBOR_OK) {
          state = (curr_node ? LINK_NODE : LINK_FIRST_NODE);
        } else {
          /* some kind of error */
          goto end;
        }
        break;

      case ANALYZE_STOP_CODE:
        if ((!ECBOR_IS_MAP (curr_node) && !ECBOR_IS_ARRAY (curr_node))
            || ECBOR_IS_DEFINITE (curr_node)
            || (ECBOR_IS_INDEFINITE (curr_node) && last_was_stop_code)) {
          /* stop code refers to parent node */
          curr_node = curr_node->parent;
          if (!curr_node) {
            rc = ECBOR_ERR_UNKNOWN;
            goto end;
          }
        }

        if ((ECBOR_IS_MAP (curr_node) || ECBOR_IS_ARRAY (curr_node))
            && ECBOR_IS_INDEFINITE (curr_node)) {
          /* correct stop code */
          state = CHECK_END_OF_DEFINITE;
          /* set stop code flag; this needs to be reset when curr_node changes */
          last_was_stop_code = 1;
        } else {
          rc = ECBOR_ERR_INVALID_STOP_CODE;
          goto end;
        }
        break;

      case LINK_FIRST_NODE:
        /* first node, skip checks */
        curr_node = new_node;
        new_node->index = 0;
        state = CONSUME_NODE;
        break;

      case LINK_NODE:
        {
          uint8_t is_unfinished_tag =
            ECBOR_IS_TAG (curr_node) && !curr_node->child;
          uint8_t is_unfinished_array_or_map =
            (ECBOR_IS_ARRAY (curr_node) || ECBOR_IS_MAP (curr_node))
            && ((ECBOR_IS_DEFINITE (curr_node) && (curr_node->length > 0)
                 && !curr_node->child)
                || (ECBOR_IS_INDEFINITE (curr_node) && !last_was_stop_code));

          if (is_unfinished_tag || is_unfinished_array_or_map) {
            /* link as child */
            curr_node->child = new_node;
            new_node->parent = curr_node;
            new_node->index = 0;
          } else {
            /* link as sibling */
            curr_node->next = new_node;
            new_node->parent = curr_node->parent;
            new_node->index = curr_node->index + 1;
          }
          
          /* new current node */
          curr_node = new_node;
          last_was_stop_code = 0;
          
          /* check end of definite arrays and maps */
          state = CHECK_END_OF_DEFINITE;
        }
        break;

      case CHECK_END_OF_DEFINITE:
        {
          uint8_t is_unfinished_tag =
            ECBOR_IS_TAG (curr_node) && !curr_node->child;
          uint8_t is_unfinished_array_or_map =
            (ECBOR_IS_ARRAY (curr_node) || ECBOR_IS_MAP (curr_node))
            && ((ECBOR_IS_DEFINITE (curr_node) && (curr_node->length > 0)
                 && !curr_node->child)
                || (ECBOR_IS_INDEFINITE (curr_node) && !last_was_stop_code));

          if (!is_unfinished_tag && !is_unfinished_array_or_map) {
            while (curr_node->parent
                   && (((ECBOR_IS_ARRAY (curr_node->parent) || ECBOR_IS_MAP (curr_node->parent))
                        && ECBOR_IS_DEFINITE (curr_node->parent)
                        && (curr_node->parent->length == (curr_node->index + 1)))
                       ||
                        (ECBOR_IS_TAG (curr_node->parent)))) {
              /* up one level */
              curr_node = curr_node->parent;
              last_was_stop_code = 0;
            }
          }

          /* consume next */
          state = CONSUME_NODE;
        }
        break;

      default:
        rc = ECBOR_ERR_UNKNOWN;
        goto end;
    }
  }

end:
  /* return to tree mode and return error code */
  context->mode = ECBOR_MODE_DECODE_TREE;

  rc = (rc == ECBOR_END_OF_BUFFER ? ECBOR_OK : rc);
  if (rc != ECBOR_OK) {
    /* make sure we don't expose garbage to user */
    context->n_items = 0;
  }
  
  /* return root node */
  if (context->n_items > 0) {
    (*root) = &context->items[0];
  }

  return rc;
}