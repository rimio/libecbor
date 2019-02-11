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

Unit tests can be run by performing:

```
cd test/
./runtests.sh
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

### Decoder

