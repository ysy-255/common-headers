// ひどい有様なのであまり使わないようにすること

#include "file.hpp"

class CSV{
	public:
	CSV(){}

	CSV(const std::vector<std::vector<std::string>> & init_data) : data(init_data){}

	CSV(const CSV & csv) : data(csv.data) {}

	CSV(const std::string & path){
		read(path);
	}


	inline std::vector<std::string> & operator[](const size_t h){
		return data[h];
	}

	void read(const std::string & path){
		std::vector<u8> stream = readFile(path);
		data = {{""}};
		bool dquote = false;
		auto itr = stream.begin(), l = itr;
		while(itr != stream.end()){
			if(dquote){
				if(*itr == '"'){
					data.back().back() += std::string(l, itr);
					if(++itr == stream.end()) break;
					if(*itr == '"'){
						data.back().back() += '"';
						l = ++itr;
					}
					else{
						dquote = false;
						l = itr;
					}
				}
				else{
					itr++;
				}
			}
			else{
				switch(*itr){
					case ',':{
						data.back().back() += std::string(l, itr);
						if(itr + 1 == stream.end()) break;
						if(*(itr + 1) != '\r' && *(itr + 1) != '\n') data.back().push_back("");
						while(++itr != stream.end()){
							if(*itr != ' ') break;
						}
						break;
					}
					case '\r':{
						data.back().back() += std::string(l, itr);
						if(++itr == stream.end()) break;
						if(*itr == '\n'){
							if(++itr == stream.end()) break;
						}
						data.push_back({""});
						break;
					}
					case '\n':{
						data.back().back() += std::string(l, itr);
						if(++itr == stream.end()) break;
						data.push_back({""});
						break;
					}
					case '"':{
						if(itr != l){
							itr++;
							continue;
						}
						dquote = true;
						itr++;
						break;
					}
					default:{
						itr++;
						continue;
					}
				}
				l = itr;
			}
		}
		if(data.back() == std::vector<std::string>{""}) data.resize(data.size() - 1);
		return;
	}

	void write(const std::string & path, const bool dquote = false){
		std::vector<u8> stream;
		for(const auto & line : data){
			for(const std::string & s : line){
				bool dquote_temp = dquote;
				if(!dquote){
					for(const char c : s){
						if(c == ',' || c == '"' || c == '\r' || c == '\n'){
							dquote_temp = true;
							break;
						}
					}
				}
				if(!dquote_temp){
					size_t current_size = stream.size();
					stream.resize(current_size + s.size());
					std::copy(s.begin(), s.end(), &stream[current_size]);
				}
				else{
					stream.reserve(stream.size() + line.size() + 2);
					stream.push_back('"');
					for(const char c : s){
						stream.push_back(c);
						if(c == '"') stream.push_back('"');
					}
					stream.push_back('"');
				}
				stream.push_back(',');
			}
			if(stream.back() == ','){
				stream.back() = '\r';
				stream.push_back('\n');
			}
			else{
				stream.push_back('\r');
				stream.push_back('\n');
			}
		}
		writeFile(path, stream);
	}

	private:
	std::vector<std::vector<std::string>> data = {{}};

};
