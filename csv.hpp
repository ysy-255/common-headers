#ifndef CSV_HPP
#define CSV_HPP

#include <span>

#include "file.hpp"

using std::string;
using std::span;

struct CSV_ReadOptions{
	bool align_width = true;
	u32 min_width = 0;
	bool trim_space = true;
};

struct CSV_WriteOptions{
	bool align_width = true;
	u32 min_width = 0;
	bool all_dquote = false;
};

// [RFC4180](https://datatracker.ietf.org/doc/html/rfc4180)よりも緩くCSVを扱うクラス
/**
 * 読み込み時は、UTF-8を想定したデータを読み込み、前後のスペースと空行を無視します。\n
 * 詳しい仕様は以下を参照してください。\n
 * 読み込みについて、最も詳しい仕様は実装を参照してください。\n
 * \n
 * ABNF形式で、このクラスで読み込むおおよその仕様`CSVR`および書き出す正確な仕様`CSVW`を定義します。\n
 * ([RFC4180](https://datatracker.ietf.org/doc/html/rfc4180)のものは`CSVO`とします)\n
 * `CR` = %x0D\n
 * `LF` = %x0A\n
 * `DQUOTE` = %x22\n
 * `COMMA` = %x2C\n
 * `CRLF` = `CR` `LF`\n
 * `TEXTDATARW` = %x00-09 / %x0B-0C / %x0E-21 / %x23-2B / %x2D-FF ; `OCTET` - (`CR` / `LF` / `DQUOTE` / `COMMA`)\n
 * `TEXTDATAO` = %x20-21 / %x23-2B / %x2D-7E ; `VCHAR` - (`DQUOTE` / `COMMA`)\n
 * `non-escapedR` = \*(`TEXTDATARW`) / (`TEXTDATARW` \*(`TEXTDATARW` / `DQUOTE`))\n
 * `non-escapedW` = 1\*(`TEXTDATARW`)\n
 * `non-escapedO` = \*(`TEXTDATAO`)\n
 * `escapedRW` = `DQUOTE` \*(`TEXTDATARW` / `COMMA` / `CR` / `LF` / 2`DQUOTE`) `DQUOTE`\n
 * `escapedO` = `DQUOTE` \*(`TEXTDATAO` / `COMMA` / `CR` / `LF` / 2`DQUOTE`) `DQUOTE`\n
 * `field_baseR` = `non-escapedR` / `escapedRW`\n
 * `fieldR` = \*(%x20 / %x09) `field_baseR` \*(%x20 / %x09)\n
 * `fieldW` = `non-escapedW` / `escapedRW`\n
 * `fieldO` = `non-escapedO` / `escapedO`\n
 * `recordR` = `fieldR` \*(`COMMA`  `fieldR`)\n
 * `recordW` = `fieldW` \*(`COMMA` `fieldW`)\n
 * `recordO` = `fieldO` \*(`COMMA` `fieldO`)\n
 * `headerR` = `recordR`\n
 * `headerO` = `recordO`\n
 * `line-breakR` = 1\*(`CR` / `LF`)\n
 * `line-breakWO` = `CRLF`\n
 * `CSVR` = [`headerR` `line-breakR`] `recordR` \*(`line-breakR` `recordR`) [`line-breakR`]\n
 * `CSVW` = `recordW` \*(`line-breakWO` `recordW`)\n
 * `CSVO` = [`headerO` `line-breakWO`] `recordO` \*(`line-breakWO` `recordO`) [`line-breakWO`]
 */
class CSV{
public:
	CSV() = default;
	CSV (const CSV& csv) = default;
	CSV (const vector<vector<string>>& init_data) : data (init_data) {}
	CSV (const string& path) {
		read(path);
	}
	CSV (const vector<string>& header, const vector<vector<string>>& records) {
		data.clear();
		data.push_back(header);
		data.insert(data.end(), records.begin(), records.end());
	}
	CSV& operator= (const vector<vector<string>>& init_data) {
		data = init_data;
		return *this;
	}

	const vector<vector<string>>& records() const{ return data; }
	vector<vector<string>>& records() { return data; }
	vector<vector<string>> copy_records() const{ return data; }

	const vector<string>& operator[] (const size_t h) const{ return data[h]; }
	const vector<string>& row (const size_t h) const{ return data[h]; }
	vector<string>& operator[] (const size_t h) { return data[h]; }
	vector<string>& row (const size_t h) { return data[h]; }
	vector<string> copy_row (const size_t h) const{ return data[h]; }

	vector<string> copy_col (const size_t w) const {
		vector<string> res;
		for (const auto& row : data) {
			res.push_back(w < row.size() ? row[w] : "");
		}
		return res;
	}

	size_t size() const{ return data.size(); }
	auto begin() { return data.begin(); }
	auto end() { return data.end(); }
	auto begin() const{ return data.begin(); }
	auto end() const{ return data.end(); }

	// ヘッダー付きのCSV用
	const vector<string>& header() const{ return data.front(); }
	vector<string>& header() { return data.front(); }
	vector<string> copy_header() const{ return data.front(); }
	auto hrecord_begin() const{
		if (data.size() <= 1) return data.end();
		return data.begin() + 1;
	}
	auto hrecord_end() const{ return data.end(); }
	span<const vector<string>> hrecords() const{
		if (data.size() <= 1) return {};
		return span<const vector<string>>(data.data() + 1, data.size() - 1);
	}
	// レコード全体を書き換えることはできませんが、個々のレコードは書き換えることができます
	span<vector<string>> access_hrecords() {
		if (data.size() <= 1) return {};
		return span<vector<string>>(data.data() + 1, data.size() - 1);
	}
	vector<vector<string>> copy_hrecords() const{
		if (data.size() <= 1) return {};
		return vector<vector<string>>(data.begin() + 1, data.end());
	}

	enum class Warn{
		NONE,
		UNEXPECT_AFTER_DQUOTE, // ""で囲まれたフィールドの次が[CRLFまたは,]以外の文字だった
		UNCLOSED_DQUOTE // "が閉じられずにデータの終端に達した
	} warn;

	enum class Err{
		NONE,
		WARN
	} err;

	using ReadOptions = CSV_ReadOptions;
	using WriteOptions = CSV_WriteOptions;


	Err read (const string& path, ReadOptions r_op = {}) {
		err = Err::NONE;
		warn = Warn::NONE;
		vector<u8> src = readFile(path);
		data = {{}};
		reader_init(src, r_op);
		while (reader.now != reader.end) {
			if (r_op.trim_space) read_space();
			if (reader.now == reader.end) break;
			data.back().emplace_back();
			u8 c = *reader.now;
			if (c == DQUOTE) read_dquote();
			else read_normal();
			if (r_op.trim_space) trim_space_back();
			if (reader.now == reader.end) break;
			c = *reader.now;
			if (c == COMMA) read_comma();
			if (c == CR || c == LF) read_br();
		}
		if (err == Err::NONE && warn != Warn::NONE) {
			err = Err::WARN;
		}
		if (r_op.align_width) {
			for (const auto& row : data)
				if (row.size() > r_op.min_width)
					r_op.min_width = row.size();
			for (auto& row : data)
				row.resize(r_op.min_width);
		}
		return err;
	}

	void write (const string& path, WriteOptions w_op = {}) {
		vector<u8> stream;
		if (w_op.align_width)
			for (const auto& row : data)
				if (row.size() > w_op.min_width)
					w_op.min_width = row.size();
		bool firstrow = true;
		for (const auto& row_ : data) {
			auto row = row_;
			if (!firstrow) {
				stream.push_back(CR);
				stream.push_back(LF);
			}
			firstrow = false;
			bool firstel = true;
			u32 width = row.size();
			if (w_op.min_width && width < w_op.min_width) row.resize(w_op.min_width);
			for (const string& el : row) {
				if (!firstel) stream.push_back(COMMA);
				firstel = false;
				bool dquote_temp = w_op.all_dquote;
				if (!dquote_temp) {
					if (el.empty()) dquote_temp = true;
					for (const char c : el) {
						if (c == COMMA || c == DQUOTE || c == CR || c == LF) {
							dquote_temp = true;
							break;
						}
					}
				}
				if (dquote_temp) {
					stream.push_back(DQUOTE);
					for (const char c : el) {
						stream.push_back(c);
						if (c == DQUOTE) stream.push_back(DQUOTE);
					}
					stream.push_back(DQUOTE);
				} else {
					stream.insert(stream.end(), el.begin(), el.end());
				}
			}
		}
		writeFile(path, stream);
	}


private:

	vector<vector<string>> data = {{}};

	static constexpr u8 CR = 0x0D;
	static constexpr u8 LF = 0x0A;
	static constexpr u8 DQUOTE = 0x22;
	static constexpr u8 COMMA = 0x2C;
	static constexpr u8 SPACE = 0x20;
	static constexpr u8 TAB = 0x09;

	struct ReadContext{
		vector<u8>::iterator l, now, end;
		ReadOptions op;
	} reader;
	void reader_init (vector<u8>& src, ReadOptions r_op) {
		reader.now = reader.l = src.begin();
		reader.end = src.end();
		reader.op = r_op;
	}


	// ノーマルフィールドでデータを追加
	void read_push() {
		data.back().back() = string(reader.l, reader.now);
	}
	// ノーマルフィールドを処理
	void read_normal() {
		read_proceed();
		read_push();
	}

	// クォーテーションフィールドでデータを足す
	void read_add() {
		data.back().back() += string(reader.l, reader.now);
	}
	// クォーテーションフィールドを処理
	void read_dquote() {
		reader.now ++;
		reader.l = reader.now;
		while (reader.now != reader.end) {
			if (*reader.now == DQUOTE) {
				read_add();
				reader.now ++;
				if (reader.now == reader.end) {
					return;
				}
				u8 c = *reader.now;
				if (c == DQUOTE) {
					data.back().back().push_back(DQUOTE);
					reader.l = reader.now + 1;
				} else {
					if (reader.op.trim_space) {
						read_space();
						if (reader.now == reader.end) return;
						c = *reader.now;
					}
					if (c != COMMA && c != CR && c != LF) {
						warn = Warn::UNEXPECT_AFTER_DQUOTE;
						read_proceed();
					}
					return;
				}
			}
			reader.now ++;
		}
		warn = Warn::UNCLOSED_DQUOTE;
		read_add();
	}

	// カンマまたは改行の位置まで進める
	void read_proceed () {
		u8 c;
		while (reader.now != reader.end) {
			c = *reader.now;
			if (c == COMMA || c == CR || c == LF) return;
			reader.now ++;
		}
		return;
	}

	// 改行を処理
	void read_br() {
		while (reader.now != reader.end) {
			u8 c = *reader.now;
			if (c == CR || c == LF) reader.now ++;
			else break;
		}
		reader.l = reader.now;
		if (reader.now != reader.end) {
			data.emplace_back();
		}
	}

	// 空白を処理
	void read_space() {
		while (reader.now != reader.end) {
			u8 c = *reader.now;
			if (c == SPACE || c == TAB) reader.now ++;
			else break;
		}
		reader.l = reader.now;
	}

	// カンマを処理
	void read_comma() {
		reader.now ++;
		reader.l = reader.now;
	}

	// 後方の空白を処理
	void trim_space_back() {
		while (data.back().back().size() > 0) {
			u8 c = data.back().back().back();
			if (c == SPACE || c == TAB) data.back().back().pop_back();
			else break;
		}
	}

};

#endif
