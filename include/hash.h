#include <cstddef>
#include <string>
#include "MurmurHash3.h"

namespace nbtlru
{

class murmur_bin_hash_x64_128_xor_shift_to_64 
{
    static_assert(sizeof(uint64_t) == sizeof(::std::size_t));
public:
    ::std::size_t operator()(::std::span<const ::std::byte> buffer) const noexcept
    {
        return operator()(buffer, 0);
    }

    ::std::size_t operator()(::std::span<const ::std::byte> buffer, uint32_t hints) const noexcept
    {
        uint64_t h[2]{};
        MurmurHash3_x64_128(buffer.data(), static_cast<int>(buffer.size()), hints, h);
        return h[0] ^ (h[1] << 1);
    }

    ::std::size_t operator()(::std::string_view buffer) const noexcept
    {
        return operator()(buffer, 0);
    }

    ::std::size_t operator()(::std::string_view buffer, uint32_t hints) const noexcept
    {
        uint64_t h[2]{};
        MurmurHash3_x64_128(buffer.data(), static_cast<int>(buffer.size()), hints, h);
        return h[0] ^ (h[1] << 1);
    }
};

template<typename>
class murmur_hash_x64_128_xor_shift_to_64
{
};

template<::std::integral Int>
class murmur_hash_x64_128_xor_shift_to_64<Int>
{
public:
    ::std::size_t operator()(Int i) const noexcept
    {
        struct buffer_t 
        { 
            ::std::byte buffer[sizeof(Int)];
        };
        const auto val = ::std::bit_cast<buffer_t>(i);
        return murmur_bin_hash_x64_128_xor_shift_to_64{}(val.buffer);
    }
};

template<>
class murmur_hash_x64_128_xor_shift_to_64<::std::string_view>
{
public:
    ::std::size_t operator()(::std::string_view str) const noexcept
    {
        return murmur_bin_hash_x64_128_xor_shift_to_64{}(str);
    }
};

} // namespace nbtlru
