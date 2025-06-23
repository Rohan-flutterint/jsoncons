### jsoncons::msgpack::decode_msgpack

```cpp
#include <jsoncons_ext/msgpack/msgpack.hpp>

template <typename T,typename BytesLike>
T decode_msgpack(const BytesLike& source,
    const msgpack_decode_options& options = msgpack_decode_options());           (1) (since 0.152.0)

template <typename T>
T decode_msgpack(std::istream& is,
    const msgpack_decode_options& options = msgpack_decode_options());           (2)

template <typename T,typename BytesLike,typename Alloc,typename TempAlloc>
T decode_msgpack(const allocator_set<Alloc,TempAlloc>& alloc_set,
    const BytesLike& source,
    const msgpack_decode_options& options = msgpack_decode_options());           (3) (since 0.171.0)

template <typename T,typename Alloc,typename TempAlloc>
T decode_msgpack(const allocator_set<Alloc,TempAlloc>& alloc_set,
    std::istream& is,
    const msgpack_decode_options& options = msgpack_decode_options());           (4) (since 0.171.0)

template <typename T,typename InputIt>
T decode_msgpack(InputIt first, InputIt last,
    const msgpack_decode_options& options = msgpack_decode_options());           (5) (since 0.153.0)

template <typename T,typename BytesLike>
read_result<T> try_decode_msgpack(const BytesLike& source,
    const msgpack_decode_options& options = msgpack_decode_options());           (6) (since 1.4.0)

template <typename T>
read_result<T> try_decode_msgpack(std::istream& is,
    const msgpack_decode_options& options = msgpack_decode_options());           (7) (since 1.4.0)

template <typename T,typename BytesLike,typename Alloc,typename TempAlloc>
read_result<T> try_decode_msgpack(const allocator_set<Alloc,TempAlloc>& alloc_set,
    const BytesLike& source,
    const msgpack_decode_options& options = msgpack_decode_options());           (8) (since 1.4.0)

template <typename T,typename Alloc,typename TempAlloc>
read_result<T> try_decode_msgpack(const allocator_set<Alloc,TempAlloc>& alloc_set,
    std::istream& is,
    const msgpack_decode_options& options = msgpack_decode_options());           (9) (since 1.4.0)

template <typename T,typename InputIt>
read_result<T> try_decode_msgpack(InputIt first, InputIt last,
    const msgpack_decode_options& options = msgpack_decode_options());           (10) (since 1.4.0)
```

Decodes a [MessagePack](http://msgpack.org/index.html) data format into a C++ data structure.

(1) Reads MessagePack data from a contiguous byte sequence provided by `source` into a type T, using the specified (or defaulted) [options](msgpack_options.md). 
Type `BytesLike` must be a container that has member functions `data()` and `size()`, 
and member type `value_type` with size exactly 8 bits (since 0.152.0.)
Any of the values types `int8_t`, `uint8_t`, `char`, `unsigned char` and `std::byte` (since C++17) are allowed.
Type 'T' must be an instantiation of [basic_json](../corelib/basic_json.md) 
or support jsoncons reflection traits.

(2) Reads MessagePack data from a binary stream into a type T, using the specified (or defaulted) [options](msgpack_options.md). 
Type 'T' must be an instantiation of [basic_json](../corelib/basic_json.md) 
or support jsoncons reflection traits.

(3)-(4) are identical to (1)-(2) except an [allocator_set](allocator_set.md) is passed as an additional argument and
provides allocators for result data and temporary allocations.

(5) Reads MessagePack data from the range [`first`,`last`) into a type T, using the specified (or defaulted) [options](msgpack_options.md). 
Type 'T' must be an instantiation of [basic_json](../corelib/basic_json.md) 
or support jsoncons reflection traits.

(6)-(10) Non-throwing versions of (1)-(5)

#### Return value

(1)-(5) Deserialized value

(6)-(10) [read_result<T>](../corelib/read_result.md)

#### Exceptions

(1)-(5) Throw [ser_error](../corelib/ser_error.md) if read fails.

Any overload may throw `std::bad_alloc` if memory allocation fails.

### See also

[encode_msgpack](encode_msgpack.md) encodes a json value to the [MessagePack](http://msgpack.org/index.html) data format.


