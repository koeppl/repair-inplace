#include <iostream>
#include <fstream>
#include <filesystem>
#include "dcheck.hpp"
#include <algorithm>
#include <functional>
#include <numeric>


using namespace std;
namespace fs = std::filesystem;

using bigram_t = uint32_t;
using char_type = uint16_t; // representing the alphabet size

bigram_t make_bigram(uint32_t a, uint32_t b) {
	DCHECK_LT(a, 1ULL<<16);
	DCHECK_LT(b, 1ULL<<16);
	return (a<<16) + b;
}

struct Entry {
	bigram_t m_bigram;
	uint32_t m_freq;
	void set(bigram_t bigram, uint32_t freq) {
		m_bigram = bigram;
		m_freq = freq;
	}
	bigram_t bigram() const { return m_bigram; }

	bool valid() const { return m_freq > 0; }
	uint32_t frequency() const { return m_freq; }
	void increment() { DCHECK_NE(m_freq, 0); ++m_freq; }
	void decrement() { DCHECK_GT(m_freq, 0); --m_freq; }
	void clear() { m_bigram = m_freq = 0; }

	char_type first() const { return m_bigram >>16; }
	char_type second() const { return m_bigram & ((1ULL<<16)-1); }

	std::string to_string() const {
		std::stringstream ss;
		ss << "(" << first() << "," << second() << "):" << m_freq;
		return ss.str();
	}

} __attribute__((packed));

struct FrequencyTable {
	Entry*const m_entries;
	size_t m_length;
	FrequencyTable(Entry* entries, size_t length) : m_entries(entries), m_length(length) {
	}
	size_t length() const { return m_length; }

	size_t find(bigram_t bigram) {
		for(size_t i = 0; i < m_length; ++i) {
			if(m_entries[i].bigram() == bigram && m_entries[i].valid()) { return i; }
		}
		return -1ULL;
	}
	size_t insert(bigram_t bigram) {
		for(size_t i = 0; i < m_length; ++i) {
			if(!m_entries[i].valid()) {
				m_entries[i].set(bigram, 1);
				return i;
			}
		}
		return -1ULL;
	}
	size_t max() const {
		size_t maxel = 0;
		for(size_t i = 1; i < m_length; ++i) {
			if(m_entries[i].frequency() > m_entries[maxel].frequency()) maxel = i;
		}
		return maxel;
	}
	size_t min() const {
		size_t minel = 0;
		for(size_t i = 1; i < m_length; ++i) {
			if(m_entries[i].frequency() < m_entries[minel].frequency() && m_entries[i].valid()) minel = i;
		}
		return minel;
	}
	Entry& operator[](size_t pos) {
		DCHECK_LT(pos, m_length);
		return m_entries[pos];
	}
	void clear() {
		for(size_t i = 0; i < m_length; ++i) {
			m_entries[i].clear();
		}
	}
	std::string to_string() const {
		std::stringstream ss;
		ss << "[";
		for(size_t i = 0; i < m_length; ++i) {
			ss << m_entries[i].to_string();
			ss << ", ";
		}
			ss << "]";
		return ss.str();
	}

};

std::string text_to_string(const char_type*const text, size_t text_length) {
	std::stringstream ss;
	ss << "Text: ";
	for(size_t i = 0; i < text_length; ++i) {
		ss << text[i];
	}
	ss << std::endl;
	return ss.str();
}

size_t character_frequency(const char_type*const text, size_t text_length, char_type a) {
	size_t frequency = 0;
	for(size_t i = 0; i < text_length; ++i) {
		if(text[i] == a) { ++frequency; }
	}
	return frequency;
}


size_t bigram_frequency(const char_type*const text, size_t text_length, char_type a, char_type b) {
	bool matched_first = false;
	size_t frequency = 0;
	for(size_t i = 0; i < text_length; ++i) {
		if(matched_first && text[i] == b) {
			matched_first = false;
			++frequency;
			continue;
		}
		if(text[i] == a) {
			matched_first = true;
		} else {
			matched_first = false;
		}
	}
	return frequency;
}

#include <stdio.h>  
#include <unistd.h>  


int main(int argc, char *argv[]) {
	
	size_t additional_memory = 200;
	size_t text_length = 0;
	std::string filename;


	if(argc < 2) {
		std::cerr << "Usage: " << argv[0] << " -f filename [-p prefixlength] [-m additional_memory]" << std::endl;
		std::cerr << "  -f: " << '\t' << "file to compress with Re-Pair" << std::endl;
		std::cerr << "  -p: " << '\t' << "the prefix (number of characters) to compress (default is the entire file)" << std::endl;
		std::cerr << "  -m: " << '\t' << "additional memory to reserve (number in bytes)" << std::endl;
		return 1;
	}

	int opt;
    while((opt = getopt(argc, argv, ":f:p:m:")) != -1)  
    {  
        switch(opt)  
        {  
            case 'f':  
				filename = optarg;
                break;  
			case 'p':
				text_length = strtoul(optarg, nullptr, 10);
				if(text_length == 0) {
					std::cerr << "Cannot parse prefix length: " << optarg << std::endl;
					return 2;
				}
				break;
			case 'm':
				additional_memory = strtoul(optarg, nullptr, 10);
				if(additional_memory == 0) {
					std::cerr << "Cannot parse additional memory: " << optarg << std::endl;
					return 2;
				}
				if(sizeof(char_type)*additional_memory/sizeof(Entry) < 3) {
					additional_memory = sizeof(Entry)*3 / sizeof(char_type);
				}
				break;
            case ':':  
                std::cout << "option " << opt << " needs a value." << std::endl;
				return 3;
                break;  
            case '?':  
                printf("unknown option: %c\n", optopt); 
                break;  
        }  
    }  

	if(filename.empty()) {
		std::cerr << "Need to specify a filename" << std::endl;
		return 4;
	}
	{
		std::ifstream is(filename);
		if(!is) {
			std::cerr << "Could not read file " << filename << std::endl;
			return 4;
		}
	}
	try {
	    const size_t total_text_length = fs::file_size(filename);  // size of the text we deal with, subject to shrinkage
	    if(text_length == 0) { text_length = total_text_length; }
	    else { text_length = std::min(text_length, total_text_length); }
	} catch(fs::filesystem_error& e) {
	    std::cerr << "Could not read file " << filename << std::endl;
	    std::cerr << e.what() << '\n';
	    return 3;
	}

	std::cout << "text length: " << text_length << std::endl;
	std::cout << "additional memory: " << additional_memory << std::endl;
	std::cout << "file name: " << filename << std::endl;
	std::cout << "byte size of an entry: " << sizeof(Entry) << std::endl;
	try {
		// size_t text_length = fs::file_size(filename);  // size of the text we deal with, subject to shrinkage
		const size_t memory_budget = text_length+additional_memory; // constant size of total memory we have
		char_type maximum_character = 0; // the currently maximum value a character can have

		DCHECK_LT(text_length, memory_budget); // need at least some memory

		char_type*const text = new char_type[memory_budget];
		std::ifstream is(filename);
		for(size_t i = 0; i < text_length && is; ++i) {
			text[i] = is.get();
			if(text[i] > maximum_character) { maximum_character = text[i]; }
		}
		if(text_length < 100) {
			std::cout << text_to_string(text, text_length);
		}
		const char_type maximum_terminal_character = maximum_character;
		std::cout << "largest terminal: " << static_cast<size_t>(maximum_character) << std::endl;

		size_t round_k = 0;
		while(true) { // for loop for every round
			const size_t available_entries = (sizeof(char_type)*(memory_budget-text_length))/sizeof(Entry);
			DCHECK_GT(available_entries, 2); // need at least some memory

			++round_k;
			std::cout << "Round " << round_k << std::endl;
			std::cout << "frequency Storage: " << available_entries << std::endl;

			Entry*const entries = reinterpret_cast<Entry*>(text+text_length);
			for(size_t i = 0; i < available_entries; ++i) { entries[i].clear(); }

			FrequencyTable helperTable(entries+available_entries/2,available_entries/2);	
			FrequencyTable mainTable(entries, available_entries/2); //final table storing the most frequent bigrams
			DCHECK_EQ(helperTable.length(), mainTable.length());

			for(size_t i = 0; i+1 < text_length; ++i) {
				auto bigram = make_bigram(text[i],text[i+1]);
				size_t pos = helperTable.find(bigram);
				if(pos == (-1ULL) && mainTable.find(bigram) == (-1ULL)) {
					pos = helperTable.insert(bigram);
					DCHECK_NE(pos, (-1ULL));
				} 
				// else {
				// 	helperTable[pos].increment();
				// }
				// std::cout << pos << std::endl;
				if(pos == helperTable.length()-1 || i+2 == text_length) {
					size_t run_position = -1ULL;
					for(size_t j = 0; j < text_length-1; ++j) {
						if(run_position == -1ULL && text[j] == text[j+1]) {
							run_position = j;
						} else if(run_position != -1ULL && text[j] != text[j+1]) {
							run_position = -1ULL;
						}
						// if in a run, take only every second bigram
						if(run_position != -1ULL && ((j-run_position) % 2 != 0)) { continue; }
						bigram = make_bigram(text[j],text[j+1]);
						pos = helperTable.find(bigram);
						if(pos == (-1ULL)) { continue; }
						else { helperTable[pos].increment(); }
					}
					for(size_t k = 0; k < helperTable.length(); ++k) { 
						if(helperTable[k].valid()) {
							helperTable[k].decrement(); // since insert also counts one up
							DCHECK_EQ(bigram_frequency(text, text_length, helperTable[k].first(), helperTable[k].second()), helperTable[k].frequency());
						}
					} 
					VVLOG << "remainder helperTable before sort:" << helperTable.to_string();
					std::sort(entries, entries+available_entries, [](const Entry&a, const Entry&b) {
							// true if a > b
							return a.frequency() > b.frequency();
							});
					VVLOG << "remainder helperTable after sort:" << helperTable.to_string();

					helperTable.clear();
				}
			}
			VVLOG << "final table:" << mainTable.to_string();

 ON_DEBUG(
		for(size_t i = 0; i < mainTable.length(); ++i) {
			if(mainTable[i].frequency() == 0) continue;
			DCHECK_EQ(bigram_frequency(text, text_length, mainTable[i].first(), mainTable[i].second()), mainTable[i].frequency());
		}
		)

			if(mainTable[mainTable.max()].frequency() < 2) break;

			const size_t min_frequency = std::max<size_t>(2,mainTable[mainTable.min()].frequency());
			DCHECK_NE(min_frequency, 0);

			size_t turn_i = 0;
			for(size_t max_index = mainTable.max(); mainTable[max_index].frequency() >= min_frequency; max_index = mainTable.max()) {
				++turn_i; // in each turn we create a new non-terminal
				VVLOG << "turn " << turn_i << " of round " << round_k << '\n';
				DCHECK_LE(static_cast<size_t>(maximum_character)+1, std::numeric_limits<char_type>::max());

				Entry& max_entry = mainTable[mainTable.max()]; // this is the bigram we want to replace
				VLOG << "create rule " << static_cast<size_t>(maximum_character+1) << " -> " << max_entry.to_string() << '\n';
				++maximum_character; // new non-terminal gets value of maximum_character assigned
				size_t replacement_offset = 0;
				for(size_t i = 0; i+1 < text_length; ++i) {
					text[i] = text[i+replacement_offset];
					const char_type next_char = text[i+replacement_offset+1];
					if(i+replacement_offset+1 < text_length &&
							make_bigram(text[i],next_char) == max_entry.bigram()) {
						if(i > 0) {
							const size_t prec_bigram_index = mainTable.find(make_bigram(text[i-1],text[i]));
							// we do not want to decrement the maximum bigram, which can happen if it is twice 
							// the same character
							if(prec_bigram_index != (-1ULL) && prec_bigram_index != max_index) { 
								bool in_even_run = true; // when replacing 'ac' in 'aaac' we decrement 'aa' only if the run of 'aaa's is even. 
								if(text[i-1] == text[i] && i > 1) {
									size_t r; // CAVEAT: can be tracked while scanning for max_entry
									for(r = i-2; text[r] == text[i] && r >= 0; --r) {} 
									if( (i-r) % 2 == 1) { in_even_run = false; }
								}
								if(in_even_run) {
									Entry& affected_entry = mainTable[prec_bigram_index];
									affected_entry.decrement();
									if(affected_entry.frequency() < min_frequency) {
										affected_entry.clear();
									}
								}
							}
						}
						if(i+replacement_offset+2 < text_length) {
							const char_type nextnext_char = text[i+replacement_offset+2];

							const size_t next_bigram_index = mainTable.find(make_bigram(next_char, nextnext_char));
							if(next_bigram_index != (-1ULL) && next_bigram_index != max_index) {

								bool in_even_run = true; // when replacing 'ac' in 'accc' we decrement 'cc' only if the run of 'ccc's is even. 
								if(nextnext_char == next_char) { // the next bigram is in a run
									size_t r; // CAVEAT: compute this only once and not on the fly
									for(r = i+replacement_offset+2; text[r] == next_char && r < text_length; ++r) {} 
									if( (r-i-replacement_offset) % 2 == 0) { in_even_run = false; }
								}
								if(in_even_run) {
									Entry& affected_entry = mainTable[next_bigram_index];
									affected_entry.decrement();
									if(affected_entry.frequency() < min_frequency) {
										affected_entry.clear();
									}
								}
							}
						}
						text[i] = maximum_character;
						++replacement_offset;
					}
				}
				DCHECK_EQ(max_entry.frequency(), replacement_offset);
				max_entry.clear();
				text_length -= replacement_offset;

 ON_DEBUG(
		for(size_t i = 0; i < mainTable.length(); ++i) {
			if(mainTable[i].frequency() == 0) continue;
			DCHECK_EQ(bigram_frequency(text, text_length, mainTable[i].first(), mainTable[i].second()), mainTable[i].frequency());
		}
		)

				// create table D = text[text_length..text_length+replacement_offset-1]
				char_type*const D = text+text_length;
				size_t D_length = 0; // length of table D

				// take preceding characters
				for(size_t i = 0; i < text_length; ++i) {
					if(i > 0 && text[i] == maximum_character) {
						D[D_length] = text[i-1];
						DCHECK_GT(bigram_frequency(text, text_length, D[D_length], maximum_character), 0);
						++D_length;
					}
				}
				DCHECK_EQ(D_length + (text[0] == maximum_character ? 1 : 0), replacement_offset);
				std::sort(D, D+D_length);

				ON_DEBUG(
				for(size_t i = 0; i < D_length; ++i) {
					DCHECK_GT(bigram_frequency(text, text_length, D[i], maximum_character), 0);
				})

				{
					char_type c = D[0];
					size_t sum = 1;
					for(size_t i = 1; i < D_length; ++i) {
						if(D[i] == c) {
							++sum;
						} else {
							DCHECK_EQ(std::accumulate(D, D+D_length, 0ULL, [c] (size_t s, char_type ct) -> size_t { return c == ct ? s+1 : s; }), sum);
							auto&& min_entry = mainTable[mainTable.min()];
							if(min_entry.frequency() < sum) {
								DCHECK_GT(bigram_frequency(text, text_length, D[i-1], maximum_character), 0);
								min_entry.set(make_bigram(D[i-1], maximum_character), sum);
								DCHECK_EQ(bigram_frequency(text, text_length, D[i-1], maximum_character), sum);
							}
							c = D[i];
							sum = 1;
						}
					}
				}
				// take succeeding characters
				D_length = 0; // index in table D

				for(size_t i = 0; i+1 < text_length; ++i) {
					if(text[i] == maximum_character) {
						D[D_length] = text[i+1];
						++D_length;
					}
				}
				DCHECK_EQ(D_length + (text[text_length-1] == maximum_character ? 1 : 0), replacement_offset);
				std::sort(D, D+D_length);

				ON_DEBUG(
				for(size_t i = 0; i < D_length; ++i) {
					DCHECK_GT(bigram_frequency(text, text_length, maximum_character, D[i]), 0);
				})

				{
					char_type c = D[0];
					size_t sum = 1;
					for(size_t i = 1; i < D_length; ++i) {
						if(D[i] == c) {
							++sum;
						} else {
							DCHECK_EQ(std::accumulate(D, D+D_length, 0ULL, [c] (size_t s, char_type ct) -> size_t { return c == ct ? s+1 : s; }), sum);
							auto&& min_entry = mainTable[mainTable.min()];
							if(min_entry.frequency() < sum) {
								DCHECK_GT(bigram_frequency(text, text_length, maximum_character, D[i-1]), 0);
								min_entry.set(make_bigram(maximum_character, D[i-1]), sum);
								DCHECK_EQ(bigram_frequency(text, text_length, maximum_character, D[i-1]), sum);
							}
							c = D[i];
							sum = 1;
						}
					}
				}
				if(text_length < 100) {
					std::cout << text_to_string(text, text_length);
				}

			}// for looping all elements of the frequency table
		}// while loop for rounds
		std::cout << "size of start symbol: " << text_length << std::endl;
		std::cout << "number of rounds: " << round_k << std::endl;
		std::cout << "number of non-terminals: " << static_cast<size_t>(maximum_character - maximum_terminal_character) << std::endl;

		delete [] text;

	} catch(fs::filesystem_error& e) {
		std::cout << e.what() << '\n';
	}
	return 0;
}
