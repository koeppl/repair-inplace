#include <iostream>
#include <fstream>
#include <filesystem>
#include "dcheck.hpp"
#include <algorithm>
#include <functional>

using namespace std;
namespace fs = std::filesystem;

using bigram_t = uint32_t;

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

	uint32_t first() const { return m_bigram >>16; }
	uint32_t second() const { return m_bigram & ((1ULL<<16)-1); }

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
			if(m_entries[i].frequency() < m_entries[minel].frequency()) minel = i;
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

using char_type = uint32_t; // representing the alphabet size

int main() {

	std::cout << sizeof(Entry) << std::endl;
	const std::string filename = "Makefile";
	try {
		size_t textsize = fs::file_size(filename);  // size of the text we deal with, subject to shrinkage
		const size_t memory_budget = textsize+10; // constant size of total memory we have
		char_type maximum_character = 0; // the currently maximum value a character can have

		DCHECK_LT(textsize, memory_budget); // need at least some memory

		char_type* text = new char_type[memory_budget];
		std::ifstream is(filename);
		for(size_t i = 0; i < textsize && is; ++i) {
			text[i] = is.get();
			if(text[i] > maximum_character) { maximum_character = text[i]; }
		}
		size_t available_entries = (sizeof(char_type)*(memory_budget-textsize))/sizeof(Entry);
		DCHECK_GT(available_entries, 2); // need at least some memory
		// for(size_t i = textsize; i <  memory_budget; ++i) { text[i] = 0; }
		Entry* entries = reinterpret_cast<Entry*>(text+textsize);
		for(size_t i = 0; i < available_entries; ++i) { entries[i].clear(); }

		FrequencyTable helperTable(entries+available_entries/2,available_entries/2);	
		for(size_t i = 0; i < textsize-1; ++i) {
			auto bigram = make_bigram(text[i],text[i+1]);
			size_t pos = helperTable.find(bigram);
			if(pos == (-1ULL)) {
				pos = helperTable.insert(bigram);
				DCHECK_NE(pos, (-1ULL));
			} else {
				helperTable[pos].increment();
			}
			std::cout << pos << std::endl;
			if(pos == helperTable.length()-1) {
				for(size_t j = i+1; j < textsize-1; ++j) {
					bigram = make_bigram(text[j],text[j+1]);
					pos = helperTable.find(bigram);
					if(pos == (-1ULL)) { continue; }
					else { helperTable[pos].increment(); }
				}
				std::cout << "remainder helperTable before sort:" << helperTable.to_string();
				std::sort(entries, entries+available_entries, [](const Entry&a, const Entry&b) {
						// true if a > b
						return a.frequency() > b.frequency();
						});
				std::cout << "remainder helperTable after sort:" << helperTable.to_string();

				helperTable.clear();
			}
		}
		FrequencyTable mainTable(entries, available_entries/2); //final table storing the most frequent bigrams
		DCHECK_EQ(helperTable.length(), mainTable.length());
		std::cout << "final table:" << mainTable.to_string();

		
		auto&& max_entry = mainTable[mainTable.max()];
		std::cout << "Replace " << max_entry.to_string() << std::endl;
		++maximum_character; // new non-terminal gets value of maximum_character assigned
		size_t replacement_offset = 0;
		for(size_t i = 0; i+1 < textsize; ++i) {
			text[i] = text[i+replacement_offset];
			text[i+1] = text[i+replacement_offset+1];
			text[i+2] = text[i+replacement_offset+2];
			if(i+replacement_offset+1 < textsize &&
					make_bigram(text[i],text[i+1]) == max_entry.bigram()) {
				if(i > 0) {
					const size_t prec_bigram_index = mainTable.find(make_bigram(text[i-1],text[i]));
					if(prec_bigram_index != (-1ULL)) {
						mainTable[prec_bigram_index].decrement();
					}
				}
				if(i+replacement_offset+2 < textsize) {
					const size_t next_bigram_index = mainTable.find(make_bigram(text[i+1],text[i+2]));
					if(next_bigram_index != (-1ULL)) {
						mainTable[next_bigram_index].decrement();
					}
				}
				text[i] = maximum_character;
				++replacement_offset;
				++i;
			}
		}
		DCHECK_EQ(max_entry.frequency(), replacement_offset);
		textsize -= replacement_offset;

		// create table D = text[textsize..textsize+replacement_offset-1]
		size_t offset = 0; // index in table D
		char_type*const D = text+textsize;

		// take preceding characters
		for(size_t i = 0; i < textsize-1; ++i) {
			if(i > 0 && text[i] == maximum_character) {
				D[offset] = text[i-1];
				++offset;
			}
		}
		DCHECK_EQ(offset, replacement_offset);
		std::sort(D, D+replacement_offset);
		
		{
			char_type c = D[0];
			size_t sum = 1;
			for(size_t i = 1; i < replacement_offset; ++i) {
				if(D[i] == c) {
					++sum;
				} else {
					auto&& min_entry = mainTable[mainTable.min()];
					if(min_entry.frequency() < sum) {
						min_entry.set(make_bigram(D[i-1], maximum_character), sum);
					}
				}
			}
		}
		// take succeeding characters
		offset = 0; // index in table D

		for(size_t i = 0; i+1 < textsize; ++i) {
			if(i+1 < textsize && text[i] == maximum_character) {
				D[offset] = text[i+1];
				++offset;
			}
		}
		DCHECK_EQ(offset, replacement_offset);
		std::sort(D, D+replacement_offset);
		
		{
			char_type c = D[0];
			size_t sum = 1;
			for(size_t i = 1; i < replacement_offset; ++i) {
				if(D[i] == c) {
					++sum;
				} else {
					auto&& min_entry = mainTable[mainTable.min()];
					if(min_entry.frequency() < sum) {
						min_entry.set(make_bigram(maximum_character, D[i-1]), sum);
					}
				}
			}
		}
		




		delete [] text;

	} catch(fs::filesystem_error& e) {
		std::cout << e.what() << '\n';
	}
	return 0;
}
