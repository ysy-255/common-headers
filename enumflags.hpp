#include <bitset>
#include <cstddef>
#include <type_traits>
#include <initializer_list>
#include <utility>

using std::bitset;
using std::size_t;

template<class E>
concept enum_type = std::is_enum_v<E>;

// 値が `0` から `Count - 1` までの連番である enum 型 `E` のフラグを管理するクラス
template<enum_type E, E Count>
class EnumFlags{
private:
	static constexpr size_t N = static_cast<size_t>(std::to_underlying(Count));
	static constexpr size_t index (const E e) {return static_cast<size_t>(std::to_underlying(e));}
	bitset<N> flags;
public:
	constexpr EnumFlags() = default;
	constexpr EnumFlags (const E e) { flags.set(index(e)); }
	constexpr EnumFlags (const std::initializer_list<E> es) {
		for (const E e : es) flags.set(index(e));
	}
	[[nodiscard]]
	static constexpr size_t size() noexcept { return N; }
	constexpr void set (const E e) { flags.set(index(e)); }
	constexpr void set (const std::initializer_list<E> es) {
		for (const E e : es) flags.set(index(e));
	}
	constexpr void clear (const E e) { flags.reset(index(e)); }
	constexpr void clear (const std::initializer_list<E> es) {
		for (const E e : es) flags.reset(index(e));
	}
	[[nodiscard]] constexpr bool is_set (const E e) const{ return flags.test(index(e)); }
	[[nodiscard]] constexpr bool any_set() const{ return flags.any(); }
	[[nodiscard]] constexpr bool any_set (const std::initializer_list<E> es) const{
		for (const E e : es) if (flags.test(index(e))) return true;
		return false;
	}
	[[nodiscard]] constexpr bool all_set() const{ return flags.all(); }
	[[nodiscard]] constexpr bool all_set (const std::initializer_list<E> es) const{
		for (const E e : es) if (!flags.test(index(e))) return false;
		return true;
	}
	[[nodiscard]] constexpr bool is_clear (const E e) const{ return !flags.test(index(e)); }
	[[nodiscard]] constexpr bool any_clear() const{ return !flags.all(); }
	[[nodiscard]] constexpr bool any_clear (const std::initializer_list<E> es) const{
		for (const E e : es) if (!flags.test(index(e))) return true;
		return false;
	}
	[[nodiscard]] constexpr bool all_clear() const{ return flags.none(); }
	[[nodiscard]] constexpr bool all_clear (const std::initializer_list<E> es) const{
		for (const E e : es) if (flags.test(index(e))) return false;
		return true;
	}
	[[nodiscard]] constexpr EnumFlags operator| (const EnumFlags& other) const{
		EnumFlags res;
		res.flags = flags | other.flags;
		return res;
	}
	[[nodiscard]] constexpr EnumFlags operator& (const EnumFlags& other) const{
		EnumFlags res;
		res.flags = flags & other.flags;
		return res;
	}
	[[nodiscard]] constexpr EnumFlags operator^ (const EnumFlags& other) const{
		EnumFlags res;
		res.flags = flags ^ other.flags;
		return res;
	}
	[[nodiscard]] constexpr EnumFlags operator~() const{
		EnumFlags res;
		res.flags = ~flags;
		return res;
	}
	constexpr EnumFlags& operator|= (const EnumFlags& other) {
		flags |= other.flags;
		return *this;
	}
	constexpr EnumFlags& operator&= (const EnumFlags& other) {
		flags &= other.flags;
		return *this;
	}
	constexpr EnumFlags& operator^= (const EnumFlags& other) {
		flags ^= other.flags;
		return *this;
	}
};
