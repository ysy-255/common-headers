#ifndef INT_HPP
#define INT_HPP

#include <cstdint>

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

#define U8MAX UINT8_MAX // 255
#define U16MAX UINT16_MAX // 65535
#define U32MAX UINT32_MAX // 4294967295 > 4.29*10⁹
#define U64MAX UINT64_MAX // 18446744073709551615 > 1.84*10¹⁹

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

#define I8MIN INT8_MIN // -128
#define I16MIN INT16_MIN // -32768
#define I32MIN INT32_MIN // -2147483648 < -2.14*10⁹
#define I64MIN INT64_MIN // -9223372036854775808 < -9.22*10¹⁸

#define I8MAX INT8_MAX // 127
#define I16MAX INT16_MAX // 32767
#define I32MAX INT32_MAX // 2147483647 > 2.14*10⁹
#define I64MAX INT64_MAX // 9223372036854775807 > 9.22*10¹⁸


#ifdef __SIZEOF_INT128__
using u128 = __uint128_t;
using i128 = __int128_t;
#define U128MAX u128(u128(0) - 1) // 340282366920938463463374607431768211455 > 3.40*10³⁸
#define I128MAX i128(U128MAX >> 1) // 170141183460469231731687303715884105727 > 1.70*10³⁸
#define I128MIN i128(-I128MAX - 1) // -170141183460469231731687303715884105728 < -1.70*10³⁸
#endif



#include <cstddef>
using std::size_t;
using std::byte;


#endif
