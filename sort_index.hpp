#ifndef SORT_INDEX_HPP
#define SORT_INDEX_HPP

#include <algorithm>
#include <vector>

#include "int.hpp"

// なお、ソートに関するインデックスは出力時のみ使用すると良い

// ソート後の各要素に尋ねます。
// あなたはどこからきたのですか
template<typename T>
std::vector<size_t> sorted_index(const std::vector<T> & vec){
	size_t sz = vec.size();
	std::vector<size_t> res(sz);
	for(size_t i = 0; i < sz; ++i){
		res[i] = i;
	}
	std::sort(res.begin(), res.end(), [&](size_t i, size_t j){ return vec[i] < vec[j]; });
	return res;
}

// ソート前の各要素に尋ねます。
// あなたはどこへいくのですか
template<typename T>
std::vector<size_t> sorted_rank(const std::vector<T> & vec){
	std::vector<size_t> from = sorted_index(vec);
	size_t sz = vec.size();
	std::vector<size_t> res(sz);
	for(size_t i = 0; i < sz; ++i){
		res[from[i]] = i;
	}
	return res;
}


template<typename T>
std::vector<T> sort_follow(const std::vector<T> & vec, const std::vector<size_t> & index){
	size_t sz = vec.size();
	std::vector<T> res(sz);
	for(size_t i = 0; i < sz; ++i){
		res[i] = vec[index[i]];
	}
	return res;
}

#endif
