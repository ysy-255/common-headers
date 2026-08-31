#ifndef SORT_INDEX_HPP
#define SORT_INDEX_HPP

#include <algorithm>
#include <vector>

using std::vector;
using std::sort;
using std::size_t;

// なお、ソートに関するインデックスは出力時のみ使用すると良い

// ソート後の各要素に尋ねます。
// あなたはどこからきたのですか
template<typename T>
inline vector<size_t> sorted_index(const vector<T> & vec){
	size_t sz = vec.size();
	vector<size_t> res(sz);
	for(size_t i = 0; i < sz; ++i){
		res[i] = i;
	}
	sort(res.begin(), res.end(), [&](size_t i, size_t j){ return vec[i] < vec[j]; });
	return res;
}

// ソート後の上位n個の各要素に尋ねます。
// あなたはどこからきたのですか
template<typename T>
inline vector<size_t> nth_sorted_index(const vector<T>& vec, size_t n) {
	size_t sz = vec.size();
	if (n > sz) n = sz;
	vector<size_t> res(sz);
	for (size_t i = 0; i < sz; ++i) {
		res[i] = i;
	}
	nth_element(res.begin(), res.begin() + n, res.end(), [&](size_t i, size_t j){ return vec[i] < vec[j]; });
	res.resize(n);
	sort(res.begin(), res.end(), [&](size_t i, size_t j){ return vec[i] < vec[j]; });
	return res;
}

// ソート前の各要素に尋ねます。
// あなたはどこへいくのですか
template<typename T>
inline vector<size_t> sorted_rank(const vector<T> & vec){
	vector<size_t> from = sorted_index(vec);
	size_t sz = vec.size();
	vector<size_t> res(sz);
	for(size_t i = 0; i < sz; ++i){
		res[from[i]] = i;
	}
	return res;
}


template<typename T>
inline vector<T> sort_follow(const vector<T> & vec, const vector<size_t> & index){
	size_t sz = vec.size();
	vector<T> res(sz);
	for(size_t i = 0; i < sz; ++i){
		res[i] = vec[index[i]];
	}
	return res;
}

#endif
