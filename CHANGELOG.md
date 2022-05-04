# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.2] - 2022-05-04
### Fixed
- Fixed compilation warnings
- Fixed call to printf without format string
- Fixed hardcoded install locations in CMake

## [1.0.1] - 2021-11-28
### Added
- Unit tests for encoder.
- APIs for retrieving encoded buffer size (`ecbor_get_encoded_buffer_size()` and `ECBOR_GET_ENCODED_BUFFER_SIZE`).

### Changed
- `ecbor_uint()` API now receives `uint64_t` argument. This should be backwards compatible with old signature.
- `ecbor_str()` and `ecbor_bstr()` APIs now take `const` pointers.

### Fixed
- Fixed `ecbor_memcpy` routine.
- Correctly updating item counter when encoding arrays and maps (thank you, ivan-baldin).
- Fix header encoding for FP64 values.
- Initialize `is_indefinite` flags for applicable item types in their builder functions.
- Fix chaining of map children.
