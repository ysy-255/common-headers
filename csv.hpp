#include "file.hpp"

class CSV{
public:
	CSV(){}
	CSV(const std::vector<std::vector<std::string>> & init_data) : data(init_data){}
	CSV(const CSV & csv) : data(csv.data) {}
	CSV(const std::string & path){
		read(path);
	}

	inline std::vector<std::string> & operator[](const size_t h){ return data[h]; }
	size_t size() const{ return data.size(); }
	auto begin(){ return data.begin(); }
	auto end(){ return data.end(); }
	auto begin() const{ return data.begin();}
	auto end() const{ return data.end();}

	enum class Warn{
		NONE,
		UNEXPECT_AFTER_DQUOTE, // ""で囲まれたフィールドの次がCRLFや,でない文字だった
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
		WriteOptions() {}
	};

	Err read(const std::string & path){
		err = Err::NONE;
		warn = Warn::NONE;
		std::vector<u8> src = readFile(path);
		data = {{}};
		reader_init(src);
		while(reader.now != reader.end){
			u8 c = *reader.now;
			if(c == '"') read_dquote();
			else read_normal();
			if(reader.now == reader.end) break;
			c = *reader.now;
			if(c == ',') read_comma();
			else if(c == '\r' || c == '\n') read_br();
			else{
				// unexpected
			}
		}
		if(err == Err::NONE && warn != Warn::NONE){
			err = Err::WARN;
		}
		return err;
	}

	void write(const std::string & path, WriteOptions w_op = {}){
		std::vector<u8> stream;
		if(w_op.align_width)
			for(const auto & row : data)
				if(row.size() > w_op.min_width)
					w_op.min_width = row.size();
		for(const auto & row : data){
			bool first = true;
			for(const std::string & el : row){
				if(!first) stream.push_back(',');
				first = false;
				bool dquote_temp = w_op.all_dquote;
				if(!dquote_temp){
					for(const char c : el){
						if(c == ',' || c == '"' || c == '\r' || c == '\n'){
							dquote_temp = true;
							break;
						}
					}
				}
				if(dquote_temp){
					stream.push_back('"');
					for(const char c : el){
						stream.push_back(c);
						if(c == '"') stream.push_back('"');
					}
					stream.push_back('"');
				}
				else{
					stream.insert(stream.end(), el.begin(), el.end());
				}
			}
			if(w_op.min_width)
				if(row.size() < w_op.min_width)
					stream.insert(stream.end(), w_op.min_width - row.size(), ',');
			stream.push_back('\r');
			stream.push_back('\n');
		}
		stream.pop_back();
		stream.pop_back();
		writeFile(path, stream);
	}


private:

	std::vector<std::vector<std::string>> data = {{}};

	struct ReadContext{
		std::vector<u8>::iterator l, now, end;
	} reader;
	void reader_init(std::vector<u8> & src){
		reader.now = reader.l = src.begin();
		reader.end = src.end();
	}

	// カンマまたは改行の位置まで進める
	inline void read_proceed(){
		u8 c;
		for(; reader.now < reader.end; reader.now ++){
			c = *reader.now;
			if(c == '\r' || c == '\n' || c == ',') return;
		}
		return;
	}

	// ノーマルフィールドでデータを追加
	inline void read_push(){
		data.back().emplace_back(reader.l, reader.now);
	}
	// ノーマルフィールドを処理
	inline void read_normal(){
		read_proceed();
		read_push();
	}

	// クォーテーションフィールドでデータを足す
	inline void read_add(){
		data.back().back() += std::string(reader.l, reader.now);
	}

	// クォーテーションフィールドの処理の中核
	inline void read_dquote_inner(){
		for(; reader.now < reader.end; ++reader.now){
			if(*reader.now == '"'){
				read_add();
				u8 nextc = '\r';
				bool close = false;
				close |= reader.now + 1 == reader.end;
				if(!close){
					nextc = *(reader.now + 1);
					close |= nextc != '"';
				}
				if(close){
					reader.now ++;
					if(nextc != ',' && nextc != '\r' && nextc != '\n'){
						warn = Warn::UNEXPECT_AFTER_DQUOTE;
						read_proceed();
					}
					return;
				}
				else{
					data.back().back().push_back('"');
					reader.now ++;
					reader.l = reader.now + 1;
				}
			}
		}
		warn = Warn::UNCLOSED_DQUOTE;
		read_add();
	}
	// クォーテーションフィールドを処理
	inline void read_dquote(){
		reader.now ++;
		reader.l = reader.now;
		data.back().push_back("");
		read_dquote_inner();
	}

	// 改行を処理
	inline void read_br(){
		while(reader.now + 1 < reader.end){
			u8 nextc = *(reader.now + 1);
			if(
				*reader.now == '\r' && nextc == '\n' ||
				*reader.now == '\n' && nextc == '\r'
			) reader.now ++;
			if(reader.now + 1 < reader.end){
				u8 nextc = *(reader.now + 1);
				if(nextc != '\r' && nextc != '\n') break;
				reader.now ++;
			}
		}
		reader.now ++;
		if(reader.now == reader.end) return;
		data.push_back({});
		reader.l = reader.now;
	}

	// カンマを処理
	inline void read_comma(){
		reader.now ++;
		reader.l = reader.now;
	}

};
