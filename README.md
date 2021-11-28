# libecbor

CBOR library with no dependencies, small memory footprint and code size, developed for both desktop and embedded applications.

## Compiling

Compiling the library:

```
cmake .
make
```

This will generate the following files:
* `lib/libecbor.so` - dynamic linking version
* `lib/libecbor.a` - static linking version
* `include/ecbor.h` - header file for library
* `ecbor-describe` - describe tool, loads CBOR contents from file and displays them

## Testing

Functional tests can be run with:

```
cd test/
./runtests.sh
```

Unit tests depend on `gtest` and must be explicitly built:

```
cmake . -DTESTING=ON
```

They can be run either via CMake:

```
ctest
```

or by directly running the unit tests executable, which provides more verbose output:

```
./bin/unittest
```

## Installation

Installing can be performed with:

```
cmake . -DCMAKE_INSTALL_PREFIX=<install_prefix>
make
make install
```

If `<install_prefix>` is omitted, then the default install path is used (usually `/usr/local`).

## Guide

### Memory usage

`libecbor` guarantees that no memory other than the buffers provided by the caller is used; it has no dependency on any memory manager or standard library, other than the standard type definitions. While this can be sometimes cumbersome, it is a desired trait for memory-constrained devices.

Moreover, both encoding and decoding have an *absolute upper bound* on stack usage, regardless of the depth or size of the CBOR object. Actual numbers depend on the compiler and flags.

### Error handling

Most `libecbor` API calls return an error code. It is a good practice to check it consistently, especially when building for embedded targets where debugging may be more difficult.

Usually you would look for a return code of `ECBOR_OK`. The full error list can be found in the header file `include/ecbor.h` under the `ecbor_error_t` enum.

### Encoder

Each encoding operation must use an encode context (`ecbor_encode_context_t`), usually defined on the stack:

```c
ecbor_encode_context_t context;
```

The encoder must be initialized either in *normal* encoding mode

```c
uint8_t buffer[BUFFER_SIZE];
ecbor_error_t rc = ecbor_initialize_encode(&context, buffer, BUFFER_SIZE);
```

or in *streamed* mode

```c
uint8_t buffer[BUFFER_SIZE];
ecbor_error_t rc = ecbor_initialize_encode_streamed(&context, buffer, BUFFER_SIZE);
```

While in *normal* mode the user can encode a whole item tree at once (including children), it lacks support for encoding indefinite strings, maps and arrays, since it is assumed the item is pre-built and the size is already known.

If support for indefinite items is mandatory then the *streamed* mode can be used, but the user must encode the children manually. Please be attentive that the desired CBOR structure is followed in this case.

`buffer` is simply an output buffer of size `BUFFER_SIZE` where the encoder will place the resulting CBOR object.

Once the context has been initialized, items can be encoded with

```c
ecbor_error_t rc = ecbor_encode (&context, &item);
```

where `item` is of type `ecbor_item_t`. If the item is too large for the context output buffer, then `ECBOR_ERR_INVALID_END_OF_BUFFER` will be returned.

Simple items can be built by using one of the following APIs:

```c
ecbor_item_t item = ecbor_int (uint_value);
ecbor_item_t item = ecbor_uint (int_value);

ecbor_item_t item = ecbor_fp32 (float_value);
ecbor_item_t item = ecbor_fp64 (double_value);

ecbor_item_t item = ecbor_bool (bool_or_int_value);

ecbor_item_t item = ecbor_bstr (bin_buffer_ptr, BIN_BUFFER_SIZE);
ecbor_item_t item = ecbor_str (c_str_ptr, strlen(c_str_ptr));
ecbor_item_t item = ecbor_tag (&child_item, tag_int_value);

ecbor_item_t item = ecbor_null ();
ecbor_item_t item = ecbor_undefined ();
```

Arrays can be created with

```c
ecbor_error_t rc = ecbor_array (&arr, array_items, item_count);
```

where `arr` is the output `ecbor_item_t`, `array_items` is a `ecbor_item_t*` pointing to the children and `item_count` is the size of `array_items`.

Maps can be created with

```c
ecbor_error_t rc = ecbor_map (&map, keys, values, keyval_count);
```

where `map` is the output `ecbor_item_t`, `keys` and `values` are `ecbor_item_t*`, and `keyval_count` is the size of `keys` and `values`. Note that the *i*-th key-value pair is composed of `keys[i]` and `values[i]`.

If using *streamed* encoding mode, then only the placeholder (token) items must be built:

```c
ecbor_item_t item = ecbor_array_token (item_count);
ecbor_item_t item = ecbor_indefinite_array_token ();

ecbor_item_t item = ecbor_map_token (keyval_count);
ecbor_item_t item = ecbor_indefinite_map_token ();

ecbor_item_t item = ecbor_stop_code ();
```

but the user must subsequently encode the correct amount of child items (for definite-size arrays and maps) or the stop code (for indefinite arrays and maps).

Once all the items have been encoded, the length of the output buffer can be obtained either by calling 
```c
size_t sz;
ecbor_error_t rc = ecbor_get_encoded_buffer_size(&context, &sz);
```

or by using the `ECBOR_GET_ENCODED_BUFFER_SIZE` macro.

### Decoder

Just like encoding, the decoding operation must use a decode context (`ecbor_decode_context_t`), usually defined on the stack:

```c
ecbor_decode_context_t context;
```

The decoder must be initialized either in *normal* decoding mode

```c
ecbor_error_t rc = ecbor_initialize_decode (&context, buffer, buffer_size);
```

in *streamed* mode

```c
ecbor_error_t rc = ecbor_initialize_decode_streamed (&context, buffer, buffer_size);
```

or in *tree* mode

```c
ecbor_item_t item_buffer[MAX_ITEMS];
ecbor_error_t rc = ecbor_initialize_decode_streamed (&context, buffer, buffer_size, item_buffer, MAX_ITEMS);
```

where `buffer` is the input `uint8_t*` buffer of size `buffer_size`, and `item_buffer` is an output item buffer.

In *streamed* mode, any decoding API call will parse and retrieve *one* item from the buffer, without children, in the order specified in the buffer. This is a *dumb* but fast mode where absolutely no semantic checking is performed. Linking parents to children is left to the user, hence this mode is recommended only for very simple scenarios.

In *normal* mode, any decoding API call will parse one *whole* item, including it's children, but will *not* store the parsed children. Children can later be retrieved with special API calls, but each of them will trigger a re-parsing of part of the subtree. This mode only uses one instance of *ecbor_item_t*, and has no limit on the size of the parsed CBOR, but may be heavy on CPU usage since no parsing results are cached.

In *tree* mode, any decoding API call will parse one *whole* item, including its children, and all parsing results are cached in `item_buffer`. User must make sure this buffer is sufficiently large to store all items. Subsequent API calls to retrieve children will simply explore the parsed tree.

**IMPORTANT:** String items will *not* copy the payload from the original CBOR buffer. Moreover, in *normal* mode children are being re-parsed from the CBOR buffer as well. Do *not* destroy this buffer until *all* manipulation on the CBOR items has been finished.

In *normal* and *streamed* mode, one item can be decoded with

```c
ecbor_item_t item;
ecbor_error_t rc = ecbor_decode (&context, &item);
```

while in *tree* mode, the root item can be decoded with

```c
ecbor_item_t *root = NULL;
ecbor_error_t rc = ecbor_decode_tree (&context, &root);
```

After the call, `root` will point to the root item from the `item_buffer`.

For item manipulation there are two alternative APIs that can be used.

### Decoder - strict API

To retrieve the type of an item:

```c
ecbor_type_t type = ecbor_get_type (&item);
```

To retrieve the values of numerics and booleans:

```c
uint8_t val;
ecbor_error_t rc = ecbor_get_uint8 (item, &val);

uint16_t val;
ecbor_error_t rc = ecbor_get_uint16 (item, &val);

uint32_t val;
ecbor_error_t rc = ecbor_get_uint32 (item, &val);

uint64_t val;
ecbor_error_t rc = ecbor_get_uint64 (item, &val);


int8_t val;
ecbor_error_t rc = ecbor_get_int8 (item, &val);

int16_t val;
ecbor_error_t rc = ecbor_get_int16 (item, &val);

int32_t val;
ecbor_error_t rc = ecbor_get_int32 (item, &val);

int64_t val;
ecbor_error_t rc = ecbor_get_int64 (item, &val);


float val;
ecbor_error_t rc = ecbor_get_fp32 (item, &val);

double val;
ecbor_error_t rc = ecbor_get_fp64 (item, &val);


uint8_t val;
ecbor_error_t rc = ecbor_get_bool (item, &val);
```

To retrieve the *tag value* of a tag item:

```c
uint64_t tag_val;
ecbor_error_t rc = ecbor_get_tag_value (&tag_item, &tag_val);
```

To retrieve the length of a binary string, string, array or map:

```c
size_t length = 0;
ecbor_error_t rc = ecbor_get_length (&item, &length);
```

To retrieve a child of a tag, array or map:

```c
ecbor_item_t arr_item;
ecbor_error_t rc = ecbor_get_array_item (&array, index, &arr_item);

ecbor_item_t *arr_item_ptr = NULL;
ecbor_error_t rc = ecbor_get_array_item_ptr (&array, index, &arr_item_ptr);


ecbor_item_t key, val;
ecbor_error_t rc = ecbor_get_map_item (&map, index, &key, &val);

ecbor_item_t *key_ptr = NULL, *val_ptr = NULL;
ecbor_error_t rc = ecbor_get_map_item_ptr (&map, index, &key_ptr, &val_ptr);


ecbor_item_t item;
ecbor_error_t rc = ecbor_get_tag_item (&tag, &item);

ecbor_item_t *item_ptr = NULL;
ecbor_error_t rc = ecbor_get_tag_item_ptr (&tag, &item_ptr);
```

**IMPORTANT:** Note that the `*_ptr` versions of the APIs work only in *tree* mode.

To retrieve the payload of definite strings and binary strings:

```c
const char *c_str = NULL;
ecbor_error_t rc = ecbor_get_str (&str_item, &c_str);

const uint8_t *buffer = NULL;
ecbor_error_t rc = ecbor_get_bstr (&bstr_item, &buffer);
```

For *indefinite* strings and binary strings, each chunk must be parsed individually (a chunk is a child item of the same type, but with *definite* length):

```c
size_t chunk_count;
ecbor_error_t rc = ecbor_get_str_chunk_count (&str_item, &chunk_count);

ecbor_item_t chunk;
ecbor_error_t rc = ecbor_get_str_chunk (&str_item, index, &chunk);


size_t chunk_count;
ecbor_error_t rc = ecbor_get_bstr_chunk_count (&bstr_item, &chunk_count);

ecbor_item_t chunk;
ecbor_error_t rc = ecbor_get_bstr_chunk (&bstr_item, index, &chunk);
```

### Decoder - fast API

The following macros help to identify the type of the item:

```c
ecbor_item_t item;

ecbor_type_t type = ECBOR_GET_TYPE(&item)

ECBOR_IS_INDEFINITE(&item)
ECBOR_IS_DEFINITE(&item)

ECBOR_IS_NINT(&item)
ECBOR_IS_UINT(&item)
ECBOR_IS_INTEGER(&item)
ECBOR_IS_BSTR(&item)
ECBOR_IS_STR(&item)
ECBOR_IS_ARRAY(&item)
ECBOR_IS_MAP(&item)
ECBOR_IS_TAG(&item)
ECBOR_IS_FP32(&item)
ECBOR_IS_FLOAT(&item)
ECBOR_IS_FP64(&item)
ECBOR_IS_DOUBLE(&item)
ECBOR_IS_BOOL(&item)
ECBOR_IS_NULL(&item)
ECBOR_IS_UNDEFINED(&item)
```

In order to retrieve the value:

```c
ecbor_item_t item;

int64_t val = ECBOR_GET_INT(&item)
uint64_t val = ECBOR_GET_UINT(&item)

const char *c_str = ECBOR_GET_STRING(&item)

float val = ECBOR_GET_FP32(&item)
doube val = ECBOR_GET_FP64(&item)

uint8_t bool_value = ECBOR_GET_BOOL(&item)

uint64_t tag_value = ECBOR_GET_TAG_VALUE(&item)
ecbor_item_t tag_item = ECBOR_GET_TAG_ITEM(&item)
```

And finally, in order to retrieve the length of arrays, maps, strings:

```c
ecbor_item_t item;
size_t len = ECBOR_GET_LENGTH(&item)
```
