#ifndef CSV_HPP
#define CSV_HPP

#include <span>

#include "file.hpp"

using std::string;
using std::span;

// [RFC4180](https://datatracker.ietf.org/doc/html/rfc4180)よりも緩くCSVを扱うクラス
/**
 * 読み込み時は、UTF-8を想定したデータを読み込み、空行を無視します。\n
 * 詳しい仕様は以下を参照してください。\n
 * 最も詳しい仕様は実装を参照してください。\n
 * \n
 * ABNF形式で、このクラスで読み込む未実装の仕様`CSVR`および書き出す未実装の仕様`CSVW`を定義します。\n
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
 * `fieldR` = `non-escapedR` / `escapedRW`\n
 * `fieldW` = `non-escapedW` / `escapedRW`\n
 * `fieldO` = `non-escapedO` / `escapedO`\n
 * `recordR` = `fieldR` \*(`COMMA` \*(%x20 / %x09) `fieldR`)\n
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
	CSV (const vector<vector<string>>& init_data) : data_ (init_data) {}
	CSV (const string& path) {
		read(path);
	}
	CSV (const vector<string>& header, const vector<vector<string>>& records) {
		data_.clear();
		data_.push_back(header);
		data_.insert(data_.end(), records.begin(), records.end());
	}
	CSV& operator= (const vector<vector<string>>& init_data) {
		data_ = init_data;
		return *this;
	}

	const vector<vector<string>>& data() const{ return data_; }
	vector<vector<string>>& data() { return data_; }
	vector<vector<string>> copy_data() const{ return data_; }

	const vector<string>& operator[] (const size_t h) const{ return data_[h]; }
	const vector<string>& row (const size_t h) const{ return data_[h]; }
	vector<string>& operator[] (const size_t h) { return data_[h]; }
	vector<string>& row (const size_t h) { return data_[h]; }
	vector<string> copy_row (const size_t h) const{ return data_[h]; }

	vector<string> copy_col (const size_t w) const {
		vector<string> res;
		for (const auto& row : data_) {
			res.push_back(w < row.size() ? row[w] : "");
		}
		return res;
	}

	size_t size() const{ return data_.size(); }
	auto begin() { return data_.begin(); }
	auto end() { return data_.end(); }
	auto begin() const{ return data_.begin(); }
	auto end() const{ return data_.end(); }

	// ヘッダー付きのCSV用
	const vector<string>& header() const{ return data_.front(); }
	vector<string>& header() { return data_.front(); }
	vector<string> copy_header() { return data_.front(); }
	auto record_begin() const { return data_.begin() + 1; }
	auto record_end() const { return data_.end(); }
	span<const vector<string>> records() const { return {data_.begin() + 1, data_.end()}; }
	// レコード全体を書き換えることはできませんが、個々のレコードは書き換えることができます
	span<vector<string>> access_records() { return {data_.begin() + 1, data_.end()}; }
	vector<vector<string>> copy_records() const { return {data_.begin() + 1, data_.end()}; }

	enum class Warn{
		NONE,
		UNEXPECT_AFTER_DQUOTE, // ""で囲まれたフィールドの次が[CRLFまたは,]以外の文字だった
		UNCLOSED_DQUOTE // "が閉じられずにデータの終端に達した
	} warn;

	enum class Err{
		NONE,
		WARN
	} err;

	struct WriteOptions{
		bool align_width = true;
		u32 min_width = 0;
		bool all_dquote = false;
	};

	Err read (const string& path) {
		err = Err::NONE;
		warn = Warn::NONE;
		vector<u8> src = readFile(path);
		data_ = {{}};
		reader_init(src);
		while (reader.now != reader.end) {
			u8 c = *reader.now;
			if (c == '"') read_dquote();
			else read_normal();
			if (reader.now == reader.end) break;
			c = *reader.now;
			if (c == ',') read_comma();
			else if(c == '\r' || c == '\n') read_br();
			else{
				// unexpected
			}
		}
		if (err == Err::NONE&& warn != Warn::NONE) {
			err = Err::WARN;
		}
		return err;
	}

	void write (const string& path, WriteOptions w_op) {
		vector<u8> stream;
		if (w_op.align_width)
			for (const auto& row : data_)
				if (row.size() > w_op.min_width)
					w_op.min_width = row.size();
		for (const auto& row : data_) {
			bool first = true;
			for (const string& el : row) {
				if (!first) stream.push_back(',');
				first = false;
				bool dquote_temp = w_op.all_dquote;
				if (!dquote_temp) {
					for (const char c : el) {
						if (c == ',' || c == '"' || c == '\r' || c == '\n') {
							dquote_temp = true;
							break;
						}
					}
				}
				if (dquote_temp) {
					stream.push_back('"');
					for (const char c : el) {
						stream.push_back(c);
						if (c == '"') stream.push_back('"');
					}
					stream.push_back('"');
				} else {
					stream.insert(stream.end(), el.begin(), el.end());
				}
			}
			if (w_op.min_width)
				if (row.size() < w_op.min_width)
					stream.insert(stream.end(), w_op.min_width - row.size(), ',');
			stream.push_back('\r');
			stream.push_back('\n');
		}
		stream.pop_back();
		stream.pop_back();
		writeFile(path, stream);
	}


private:

	vector<vector<string>> data_ = {{}};

	struct ReadContext{
		vector<u8>::iterator l, now, end;
	} reader;
	void reader_init (vector<u8>& src) {
		reader.now = reader.l = src.begin();
		reader.end = src.end();
	}

	// カンマまたは改行の位置まで進める
	void read_proceed () {
		u8 c;
		for (; reader.now < reader.end; reader.now ++) {
			c = *reader.now;
			if (c == '\r' || c == '\n' || c == ',') return;
		}
		return;
	}

	// ノーマルフィールドでデータを追加
	void read_push() {
		data_.back().emplace_back(reader.l, reader.now);
	}
	// ノーマルフィールドを処理
	void read_normal() {
		read_proceed();
		read_push();
	}

	// クォーテーションフィールドでデータを足す
	void read_add() {
		data_.back().back() += string(reader.l, reader.now);
	}

	// クォーテーションフィールドの処理の中核
	void read_dquote_inner() {
		for (; reader.now < reader.end; ++reader.now) {
			if (*reader.now == '"') {
				read_add();
				u8 nextc = '\r';
				bool close = false;
				close |= reader.now + 1 == reader.end;
				if (!close) {
					nextc = *(reader.now + 1);
					close |= nextc != '"';
				}
				if (close) {
					reader.now ++;
					if (nextc != ','&& nextc != '\r'&& nextc != '\n') {
						warn = Warn::UNEXPECT_AFTER_DQUOTE;
						read_proceed();
					}
					return;
				} else {
					data_.back().back().push_back('"');
					reader.now ++;
					reader.l = reader.now + 1;
				}
			}
		}
		warn = Warn::UNCLOSED_DQUOTE;
		read_add();
	}
	// クォーテーションフィールドを処理
	void read_dquote() {
		reader.now ++;
		reader.l = reader.now;
		data_.back().push_back("");
		read_dquote_inner();
	}

	// 改行を処理
	void read_br() {
		while (reader.now + 1 < reader.end) {
			u8 nextc = *(reader.now + 1);
			if (
				*reader.now == '\r'&& nextc == '\n' ||
				*reader.now == '\n'&& nextc == '\r'
			) reader.now ++;
			if (reader.now + 1 < reader.end) {
				u8 nextc = *(reader.now + 1);
				if (nextc != '\r'&& nextc != '\n') break;
				reader.now ++;
			}
		}
		reader.now ++;
		if (reader.now == reader.end) return;
		data_.push_back({});
		reader.l = reader.now;
	}

	// カンマを処理
	void read_comma() {
		reader.now ++;
		reader.l = reader.now;
	}

};

#endif
