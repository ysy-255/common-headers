#include <vector>
#include <algorithm>

#include "int.hpp"

u8 bit_ceil_exp(u64 n){
	if(n == 0) return 0;
	return (((n & (n - 1)) == 0 ? 63 : 64) - __builtin_clzll(n));
}

// e: 単位元
template <typename T, T e>
class SegTree{
	private:
	inline T op(const T a, const T b) const{
		T res;
		/* -- ここを書き換える -- */
		res = a + b;

		return res;
	}

	u32 lg;
	size_t sz;
	std::vector<T> dt;

	SegTree() = delete;
	SegTree(const SegTree &) = delete;
	SegTree & operator=(const SegTree &) = delete;


	public:
	SegTree(const size_t _sz, const T init = e){
		if(_sz == 0) return;
		lg = bit_ceil_exp(_sz);
		sz = 1ULL << lg;
		dt.reserve(sz + sz);
		dt.resize(sz);
		dt.resize(sz + sz, init);
		update_all();
	}
	SegTree(const std::vector<T> & vec, const T init = e){
		if(vec.empty()) return;
		lg = bit_ceil_exp(vec.size());
		sz = 1ULL << lg;
		dt.resize(sz + sz, init);
		std::copy(vec.begin(), vec.end(), dt.begin() + sz);
		update_all();
	}

	void set(size_t dst, const T value){
		dst += sz;
		dt[dst] = value;
		while((dst >>= 1) > 0){
			dt[dst] = op(dt[dst + dst], dt[dst + dst + 1]);
		}
	}

	T at(size_t ofs) const{
		return dt[sz + ofs];
	}

	// [l, r]
	T range0(size_t l, size_t r) const{
		l += sz; r += sz;
		T resl = e, resr = e;
		while(l <= r){
			if((l & 1) == 1) resl = op(resl, dt[l++]);
			if((r & 1) == 0) resr = op(dt[r--], resr);
			l >>= 1;
			r >>= 1;
		}
		return op(resl, resr);
	}

	// [l, r)
	T range1(size_t l, size_t r) const{
		l += sz; r += sz;
		T resl = e, resr = e;
		while(l < r){
			if((l & 1) == 1) resl = op(resl, dt[l++]);
			if((r & 1) == 1) resr = op(dt[--r], resr);
			l >>= 1;
			r >>= 1;
		}
		return op(resl, resr);
	}

	size_t lower_find(const T val, size_t l = 0) const{
		T cur = e, next;
		if(dt[sz + l] >= val) return l;
		l += sz;
		while((l & (l + 1)) != 0){
			next = op(cur, dt[l]);
			if(next >= val) break;
			if((l & 1) == 1){
				cur = next;
				l ++;
			}
			l >>= 1;
		}
		next = op(cur, dt[l]);
		if(next < val) return sz;
		while(l < sz){
			l <<= 1;
			next = op(cur, dt[l]);
			if(next < val){
				cur = next;
				l ++;
			}
		}
		return l - sz;
	}


	private:
	void update_all(){
		size_t dst = sz;
		while(--dst > 0){
			dt[dst] = op(dt[dst + dst], dt[dst + dst + 1]);
		}
	}
};
