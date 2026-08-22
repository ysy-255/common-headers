#include <bitset>
#include <concepts>
#include <type_traits>
#include <initializer_list>
#include <utility>

#include "int.hpp" // size_t

using std::bitset;

template<class E>
concept enum_type = std::is_enum_v<E>;

template<enum_type E, size_t N>
class EnumBitset{
public:
	EnumBitset() = default;
	static constexpr size_t size() noexcept { return N; }
	static constexpr size_t index (const E e) noexcept { return static_cast<size_t>(std::to_underlying(e)); }
	bool is_set (const E e) const{ return flags.test(index(e)); }
	bool is_clear (const E e) const{ return !flags.test(index(e)); }
	void set (const E e) { flags.set(index(e)); }
	void sets (const std::initializer_list<E> es) {
		for (const E e : es) flags.set(index(e));
	}
	void clear (const E e) { flags.reset(index(e)); }
	void clears (const std::initializer_list<E> es) {
		for (const E e : es) flags.reset(index(e));
	}
	bool any_clear() const{ return !flags.all(); }
	bool any_clear (const std::initializer_list<E> es) const{
		for (const E e : es) if (!flags.test(index(e))) return true;
		return false;
	}
	bool all_clear() const{ return flags.none(); }
	bool all_clear (const std::initializer_list<E> es) const{
		for (const E e : es) if (flags.test(index(e))) return false;
		return true;
	}
	bool any_set() const{ return flags.any(); }
	bool any_set (const std::initializer_list<E> es) const{
		for (const E e : es) if (flags.test(index(e))) return true;
		return false;
	}
	bool all_set() const{ return flags.all(); }
	bool all_set (const std::initializer_list<E> es) const{
		for (const E e : es) if (!flags.test(index(e))) return false;
		return true;
	}
	EnumBitset operator| (const EnumBitset& other) const{
		EnumBitset res;
		res.flags = flags | other.flags;
		return res;
	}
	EnumBitset operator& (const EnumBitset& other) const{
		EnumBitset res;
		res.flags = flags & other.flags;
		return res;
	}
	EnumBitset operator^ (const EnumBitset& other) const{
		EnumBitset res;
		res.flags = flags ^ other.flags;
		return res;
	}
	EnumBitset operator~() const{
		EnumBitset res;
		res.flags = ~flags;
		return res;
	}
	EnumBitset& operator|= (const EnumBitset& other) {
		flags |= other.flags;
		return *this;
	}
	EnumBitset& operator&= (const EnumBitset& other) {
		flags &= other.flags;
		return *this;
	}
	EnumBitset& operator^= (const EnumBitset& other) {
		flags ^= other.flags;
		return *this;
	}
protected:
	bitset<N> flags;
};
