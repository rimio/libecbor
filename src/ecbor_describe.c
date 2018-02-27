/*
 * Copyright (c) 2018 Vasile Vilvoiu <vasi.vilvoiu@gmail.com>
 *
 * libecbor is free software; you can redistribute it and/or modify
 * it under the terms of the MIT license. See LICENSE for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <malloc.h>
#include <ecbor.h>

/*
 * Command line arguments
 */
static struct option long_options[] = {
  { "help", no_argument,       0, 'h' },
  { 0, 0, 0, 0 }
};

void print_help (void);
void print_ecbor_error (ecbor_error_t err);
ecbor_error_t print_ecbor_item (ecbor_item_t *item, int level);

/*
 * Print help
 */
void
print_help (void)
{
  printf ("Usage: ecbor-describe [options] <filename>\n");
  printf ("  options:\n");
  printf ("  -h, --help     Display this help message\n");
}

void
print_ecbor_error (ecbor_error_t err)
{
  printf ("ECBOR error %d\n", err);
}

ecbor_error_t
print_ecbor_item (ecbor_item_t *item, int level)
{
  ecbor_major_type_t mt;
  ecbor_error_t rc;
  int i;

  for(i = 0; i < level * 2; i ++) putchar(' ');

  mt = ecbor_get_type (item);
  switch (mt) {
    case ECBOR_MT_NINT:
      {
        int64_t val;
        
        rc = ecbor_get_value (item, (void *) &val);
        if (rc != ECBOR_OK) {
          print_ecbor_error (rc);
          return rc;
        }
        
        printf ("[NINT] value %d\n", (int)val);
      }
      break;

    case ECBOR_MT_UINT:
      {
        uint64_t val;
        
        rc = ecbor_get_value (item, (void *) &val);
        if (rc != ECBOR_OK) {
          print_ecbor_error (rc);
          return rc;
        }
        
        printf ("[UINT] value %u\n", (unsigned int) val);
      }
      break;
    
    case ECBOR_MT_ARRAY:
      {
        uint64_t len, i;

        rc = ecbor_get_length (item, &len);
        if (rc != ECBOR_OK) {
          print_ecbor_error (rc);
          return rc;
        }

        printf ("[ARRAY] len %u\n", (unsigned int) len);
        for (i = 0; i < len; i ++) {
          ecbor_item_t child;

          rc = ecbor_get_array_item (item, i, &child);
          if (rc != ECBOR_OK) {
            print_ecbor_error (rc);
            return rc;
          }
          
          print_ecbor_item (&child, level+1);
        }
      }
      break;

    default:
      printf ("[UNKNOWN]\n");
      break;
  }
  
  return ECBOR_OK;
}

/*
 * Program entry
 */
int
main(int argc, char **argv)
{
  char *filename = NULL;
  unsigned char *cbor = NULL;
  long int cbor_length = 0;

  /* parse arguments */
  while (1) {
    int option_index, c;

    c = getopt_long (argc, argv, "h", long_options, &option_index);
    if (c == -1) {
      break;
    }
    
    switch (c) {
      default:
        print_help ();
        return 0;
    }
  }
  
  if (optind != (argc-1)) {
    printf ("Expecting exactly one file name!\n");
    print_help ();
    return -1;
  } else {
    filename = strdup (argv[optind]);
  }
  
  /* load CBOR data from file */
  {
    FILE *fp;

    printf ("Reading CBOR from file '%s'\n", filename);
    fp = fopen (filename, "rb");
    if (!fp) {
      printf ("Error opening file!\n");
      return -1;
    }

    if (fseek (fp, 0L, SEEK_END)) {
      printf ("Error seeking end of file!\n");
      fclose (fp);
      return -1;
    }
    cbor_length = ftell(fp);
    if (cbor_length < 0) {
      printf ("Error determining input size!\n");
      fclose (fp);
      return -1;
    }
    if (fseek (fp, 0L, SEEK_SET)) {
      printf ("Error seeking beginning of file!\n");
      fclose (fp);
      return -1;
    }
    
    cbor = (unsigned char *) malloc (cbor_length);
    if (!cbor) {
      printf ("Error allocating %d bytes!\n", (int) cbor_length);
      fclose (fp);
      return -1;
    }
    
    if (fread (cbor, 1, cbor_length, fp) != (size_t) cbor_length) {
      printf ("Error reading %d bytes!\n", (int) cbor_length);
      fclose (fp);
      return -1;
    }
    
    fclose (fp);
    free (filename);
  }
  
  /* parse CBOR data */
  {
    ecbor_decode_context_t context;
    ecbor_item_t item;
    ecbor_error_t rc;

    rc = ecbor_initialize_decode (&context, cbor, cbor_length);
    if (rc != ECBOR_OK) {
      print_ecbor_error (rc);
      return -1;
    }
    
    printf ("CBOR objects:\n");
    
    while (1) {
      rc = ecbor_decode (&context, &item);
      if (rc != ECBOR_OK) {
        if (rc == ECBOR_END_OF_BUFFER) {
          break;
        }
        print_ecbor_error (rc);
        return -1;
      }
      
      print_ecbor_item (&item, 0);
    }
  }
  
  free (cbor);
  
  /* all ok */
  return 0;
}