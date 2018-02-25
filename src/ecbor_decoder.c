/*
 * Copyright (c) 2018 Vasile Vilvoiu <vasi.vilvoiu@gmail.com>
 *
 * libecbor is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include "ecbor.h"
#include "ecbor_internal.h"

static ecbor_error_t
ecbor_initialize_decode_internal (ecbor_decode_context_t *context,
                                  const uint8_t *buffer,
                                  uint64_t buffer_size)
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
                         uint64_t buffer_size)
{
  ecbor_error_t rc =
    ecbor_initialize_decode_internal (context, buffer, buffer_size);

  if (rc != ECBOR_OK) {
    return rc;
  }

  context->mode = ECBOR_MODE_DECODE;
  context->nodes = NULL;
  context->node_capacity = 0;
  context->n_nodes = 0;
  
  return ECBOR_OK;
}

ecbor_error_t
ecbor_initialize_decode_streamed (ecbor_decode_context_t *context,
                                  const uint8_t *buffer,
                                  uint64_t buffer_size)
{
  ecbor_error_t rc =
    ecbor_initialize_decode_internal (context, buffer, buffer_size);

  if (rc != ECBOR_OK) {
    return rc;
  }
  
  context->mode = ECBOR_MODE_DECODE_STREAMED;
  context->nodes = NULL;
  context->node_capacity = 0;
  context->n_nodes = 0;
  
  return ECBOR_OK;
}

ecbor_error_t
ecbor_initialize_decode_tree (ecbor_decode_context_t *context,
                              const uint8_t *buffer,
                              uint64_t buffer_size,
                              ecbor_node_t *node_buffer,
                              uint64_t node_capacity)
{
  ecbor_error_t rc =
    ecbor_initialize_decode_internal (context, buffer, buffer_size);

  if (rc != ECBOR_OK) {
    return rc;
  }
  
  if (!node_buffer) {
    return ECBOR_ERR_NULL_ITEM_BUFFER;
  }
  
  context->mode = ECBOR_MODE_DECODE_TREE;
  context->nodes = node_buffer;
  context->node_capacity = node_capacity;
  context->n_nodes = 0;
  
  return ECBOR_OK;
}

static inline ecbor_error_t
ecbor_decode_uint (ecbor_decode_context_t *context,
                   uint64_t *value,
                   uint64_t *size,
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
        (*value) = *((uint16_t *) context->in_position);
        break;

      case ECBOR_ADDITIONAL_4BYTE:
        (*value) = *((uint32_t *) context->in_position);
        break;

      case ECBOR_ADDITIONAL_8BYTE:
        (*value) = *((uint64_t *) context->in_position);
        break;

      default:
        return ECBOR_ERR_INVALID_ADDITIONAL;
    }
    
    /* advance buffer */
    context->in_position += (*size);
    context->bytes_left -= (*size);
  }
  
  return ECBOR_OK;
}

static ecbor_error_t
ecbor_decode_next_internal (ecbor_decode_context_t *context,
                            ecbor_item_t *item,
                            int8_t is_chunk,
                            ecbor_major_type_t chunk_mtype)
{
  unsigned int additional;

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
  item->major_type = ECBOR_MT_UNDEFINED;
  item->size = 0;
  item->n_chunks = 0;
  item->is_indefinite = false;
  
  /* extract major type (most significant three bits) and additional info */
  item->major_type = (*context->in_position >> 5) & 0x07;
  additional = (*context->in_position & 0x1f);
  context->in_position ++; context->bytes_left --;
  
  /* check mandatory major type (in case we are reading string chunks);
     we do not want to continue parsing a malformed indefinite string and
     potentially explode the stack with subsequent calls */
  if (is_chunk && chunk_mtype != item->major_type) {
    if (item->major_type == ECBOR_MT_SPECIAL
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
  switch (item->major_type) {

    /*
     * Integer types
     */
    case ECBOR_MT_UINT:
      return ecbor_decode_uint (context, &item->value.uinteger, &item->size,
                                additional);
    
    case ECBOR_MT_NINT:
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
    case ECBOR_MT_BSTR:
    case ECBOR_MT_STR:
      /* keep buffer pointer from current pointer */
      item->value.string = context->in_position;

      /* discriminate between definite and indefinite strings; we do not treat
       * indefinite strings as we do indefinite maps and arrays, in the sense
       * that we do not allow the user to manually walk each chunk
       */
      if (additional == ECBOR_ADDITIONAL_INDEFINITE) {
        ecbor_item_t chunk;
        ecbor_error_t rc;
        
        /* mark accordingly */
        item->is_indefinite = true;
        
        /* do not allow nested indefinite length strings */
        if (is_chunk) {
          return ECBOR_ERR_NESTET_INDEFINITE_STRING;
        }

        /* indefinite lenght string; read through all blocks to compute size */
        item->value.string = context->in_position;
        
        while (true) {
          /* read next chunk */
          rc = ecbor_decode_next_internal (context, &chunk, true,
                                           item->major_type);
          if (rc != ECBOR_OK) {
            if (rc == ECBOR_END_OF_INDEFINITE) {
              /* stop code found, break from loop */
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
          item->size += chunk.size;
          item->n_chunks ++;
        }
      } else {
        uint64_t foo;

        /* read size of buffer */
        ecbor_error_t rc = ecbor_decode_uint (context, &item->size, &foo,
                                              additional);
        if (rc != ECBOR_OK) {
          return rc;
        }

        /* advance */
        if (context->bytes_left < item->size) {
          return ECBOR_ERR_INVALID_END_OF_BUFFER;
        }
        context->in_position += item->size;
        context->bytes_left -= item->size;
      }
      break;

    /*
     * Arrays and maps
     */
    case ECBOR_MT_ARRAY:
    case ECBOR_MT_MAP:
      /* keep buffer pointer from current pointer */
      item->value.items = context->in_position;

      /* discriminate between definite and indefinite maps and arrays */
      if (additional == ECBOR_ADDITIONAL_INDEFINITE) {
        /* mark accordingly */
        item->is_indefinite = true;

        if (context->mode != ECBOR_MODE_DECODE_STREAMED) {
          /* we have an indefinite map or array and we're not in streamed mode;
             we have to walk children to compute size and advance to next item */
          ecbor_item_t child;
          ecbor_error_t rc;

          while (true) {
            /* read next chunk */
            rc = ecbor_decode_next_internal (context, &child, false,
                                             ECBOR_MT_UNDEFINED);
            if (rc != ECBOR_OK) {
              if (rc == ECBOR_END_OF_INDEFINITE) {
                /* stop code found, break from loop */
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
            item->n_chunks ++;
          }
          
          if ((item->major_type == ECBOR_MT_MAP) && (item->n_chunks % 2 != 0)) {
            /* incomplete key-value pair; we expect maps to have even number of
               items */
            return ECBOR_ERR_INVALID_KEY_VALUE_PAIR;
          }
        }
      } else {
        uint64_t foo;

        /* read size of map or array */
        ecbor_error_t rc = ecbor_decode_uint (context, &item->n_chunks, &foo,
                                              additional);
        if (rc != ECBOR_OK) {
          return rc;
        }

        if (item->major_type == ECBOR_MT_MAP) {
          /* we keep the total number of items in n_chunks, yet the map has the
             number of key-value pairs encoded in the length */
          item->n_chunks *= 2;
        }

        if (context->mode != ECBOR_MODE_DECODE_STREAMED) {
          ecbor_item_t child;
          ecbor_error_t rc;
          uint64_t child_no;

          /* not in streamed mode; compute size so we can advance */
          for (child_no = 0; child_no < item->n_chunks; child_no ++) {
            /* read next child */
            rc = ecbor_decode_next_internal (context, &child, false,
                                             ECBOR_MT_UNDEFINED);
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

    case ECBOR_MT_TAG:
      return ECBOR_ERR_CURRENTLY_NOT_SUPPORTED;
      break;

    case ECBOR_MT_SPECIAL:
      if (additional == ECBOR_ADDITIONAL_INDEFINITE) {
        /* stop code */
        return ECBOR_END_OF_INDEFINITE;
      } else {
        return ECBOR_ERR_CURRENTLY_NOT_SUPPORTED;
      }
      break;
    
    default:
      return ECBOR_ERR_UNKNOWN;
  }
  
  return ECBOR_OK;
}

ecbor_error_t
ecbor_decode (ecbor_decode_context_t *context, ecbor_item_t *item)
{
  ecbor_error_t rc = ECBOR_OK;
  ecbor_node_t *curr_prev = NULL, *curr_parent = NULL, *curr_node = NULL;
  
  if (!context) {
    return ECBOR_ERR_NULL_CONTEXT;
  }
  if (!item) {
    return ECBOR_ERR_NULL_ITEM;
  }

  if (context->mode == ECBOR_MODE_DECODE
      || context->mode == ECBOR_MODE_DECODE_STREAMED) {
    /* we just get the next item */
    return ecbor_decode_next_internal (context, item, false, ECBOR_MT_UNDEFINED);
  }
  
  /*
   * decode whole tree
   */
  context->n_nodes = 0;
  
  /* step into streamed mode; some of the semantic checks will be done here */
  context->mode = ECBOR_MODE_DECODE_STREAMED;

  /* consume until something happens */
  while (true) {

    /* allocate new node */
    if (context->n_nodes >= context->node_capacity) {
      rc = ECBOR_ERR_END_OF_NODE_BUFFER;
      goto end;
    }

    curr_prev = curr_node;
    curr_node = &context->nodes[context->n_nodes];
    context->n_nodes ++;
    
    if (curr_prev) {
      curr_prev->next = curr_node;
      curr_node->index = curr_prev->index + 1;
    } else {
      curr_node->index = 0;
    }

    /* consume next item */
    rc = ecbor_decode_next_internal (context, &curr_node->item, false,
                                     ECBOR_MT_UNDEFINED);

    /* handle end of indefinite */
    if (rc == ECBOR_END_OF_INDEFINITE) {
      if ((!curr_parent)
          || (curr_parent->item.major_type != ECBOR_MT_MAP
              && curr_parent->item.major_type != ECBOR_MT_ARRAY)
          || (!curr_parent->item.is_indefinite)) {
        /* we are not in an indefinite map or array */
        rc = ECBOR_ERR_INVALID_STOP_CODE;
        goto end;
      }

      /* jump up one level */
      curr_node = curr_parent;
      curr_parent = curr_parent->parent;
      
      /* ignore this item, reuse it later */
      context->n_nodes --;
      continue;
    }
    
    /* handle new children */
    if (curr_node->item.major_type == ECBOR_MT_MAP
        || curr_node->item.major_type == ECBOR_MT_ARRAY) {
      /* TODO */
    }
    
    /* handle end of definite maps and arrays */
    /* TODO */
    
    /* check return code */
    if (rc == ECBOR_END_OF_BUFFER) {
      if (!curr_parent) {
        /* we are not allowed to reach the end unless the item is top-level */
        rc = ECBOR_ERR_INVALID_END_OF_BUFFER;
        goto end;
      }
    } else if (rc != ECBOR_OK) {
      goto end;
    }
  }

end:
  /* return to tree mode and return error code */
  context->mode = ECBOR_MODE_DECODE_TREE;
  return rc;
}