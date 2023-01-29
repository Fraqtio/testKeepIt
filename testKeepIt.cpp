#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <ctime>
#include <unordered_set>
#include <thread>
#include <future>


const int processor_count = std::thread::hardware_concurrency();


int fileSize(std::string filename) {
	std::ifstream in_file(filename, std::ios::binary);
	in_file.seekg(0, std::ios::end);
	int file_size = in_file.tellg();
	return file_size;
}


struct connector {
	std::string firstWord;
	std::string lastWord;
};


void readChunk(	int startInd, 
				int endInd,
				int core,
				std::vector<connector> &wordsParts,
				std::string fromFile, 
				std::unordered_set<std::string> &stringSet,
				int& endFlag) {
	connector change{};
	std::string bufferStr{ "" };
	char ch;
	int shift{ 0 };
	std::ifstream input(fromFile);
	input.seekg(startInd);
	input.get(ch);
	while ((ch != ' ') && (startInd + shift < endInd)) {
		bufferStr += ch;
		++shift;
		input.get(ch);
	}
	change.firstWord = bufferStr;
	if ((startInd + shift) == endInd){
		wordsParts[core] = change;
		++endFlag;
		return;
	}
	bufferStr = "";
	bool isWord{ false };
	for (int i = startInd + shift; i < endInd; i++)
	{ 
		if ((ch == ' ') && (isWord)) {
			stringSet.insert(bufferStr);
			bufferStr = "";
			isWord = false;
		}
		else if (ch != ' ') {
			bufferStr += ch;
			isWord = true;
		}
		input.get(ch);
	}
	if ((bufferStr != "") && (bufferStr != " "))
		change.lastWord = bufferStr;
	else
		change.lastWord = " ";
	wordsParts[core] = change;
	std::cout << "Calculated thread " << core << '\n';
	++endFlag;
	return;
}


void connectPieces(std::vector<connector>& pieces, std::unordered_set<std::string>& stringSet) {
	if (pieces.size() == 1) {
		stringSet.insert(pieces[0].firstWord);
		if (pieces[0].lastWord != " ")
			stringSet.insert(pieces[0].lastWord);
	}
	else {
		std::string bufferStr{""};
		for (int i = 0; i < processor_count; i++) {
			if ((pieces[i].firstWord != "") && (pieces[i].lastWord == ""))
				bufferStr += pieces[i].firstWord;
			else if ((pieces[i].firstWord != "") && (pieces[i].lastWord == " ")) {
				bufferStr += pieces[i].firstWord;
				stringSet.insert(bufferStr);
				bufferStr = "";
			}
			else if (pieces[i].firstWord != "") {
				bufferStr += pieces[i].firstWord;
				stringSet.insert(bufferStr);
				bufferStr = pieces[i].lastWord;
			}
			else if ((pieces[i].firstWord == "") && (pieces[i].lastWord != " ")) {
				stringSet.insert(bufferStr);
				bufferStr = pieces[i].lastWord;
			}
			else{
				stringSet.insert(bufferStr);
				bufferStr = "";
			}
		}
		stringSet.insert(bufferStr);
	}
}


void setInsert(std::unordered_set<std::string>& set1, std::unordered_set<std::string>& set2, int& endFlag) {
	set1.insert(set2.begin(), set2.end());
	++endFlag;
}


void concatSets(std::vector<std::unordered_set<std::string>>& sets, std::unordered_set<std::string>& set) {
	int length = sets.size();
	while (length > 1) {
		int endFlag{ 0 };
		if (length % 2 == 1)
			std::thread(setInsert, std::ref(set), std::ref(sets[length-1]), std::ref(endFlag)).detach();
		int halflen = length / 2;
		for (int i = 0; i < halflen; i++)
			std::thread(setInsert, std::ref(sets[i]), std::ref(sets[i+halflen]), std::ref(endFlag)).detach();
		while (endFlag < (halflen + (length % 2))) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }
		length /= 2;
		sets.resize(length);
	}
	set.insert(sets[0].begin(), sets[0].end());
}


int countChunkUnique(std::string fromFile) {
	std::ifstream input(fromFile);
	std::vector<connector> wordsParts;
	std::vector<std::unordered_set<std::string>> stringSets;
	std::unordered_set<std::string> stringSet;
	if (input.is_open()) {
		int endFlag{ 0 };
		int fileEnd{ fileSize(fromFile)};
		if (fileEnd < 1e8) {
			std::cout << "Joining thread 1" << '\n';
			stringSets.resize(1);
			wordsParts.resize(1);
			readChunk(0, fileEnd, 0, wordsParts, fromFile, stringSets[0], endFlag);
		}
		else {
			wordsParts.resize(processor_count);
			stringSets.resize(processor_count);
			int chunkSize {fileEnd/processor_count};
			for (int i = 0; i < processor_count; i++)	{
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
				int startPoint{i * chunkSize};
				int endPoint{(i == processor_count - 1) ? fileEnd : (i + 1) * chunkSize};
				std::cout << "Joining thread " << i << '\n';
				std::thread(readChunk, 
							startPoint, 
							endPoint, 
							i, 
							std::ref(wordsParts), 
							fromFile, 
							std::ref(stringSets[i]),
							std::ref(endFlag)).detach();	
			}
			while (endFlag != processor_count) { std::this_thread::sleep_for(std::chrono::milliseconds(100));}
		}
		concatSets(stringSets, stringSet);
		connectPieces(wordsParts, stringSet);
	}
	stringSet.erase("");
	return stringSet.size();
}


int main()
{
	time_t sTime{ 0 };
	sTime = time(NULL);

	
	std::string inputFile{"large_input.txt"};
	std::cout << "Total unique words: " << countChunkUnique(inputFile) << '\n';

	time_t eTime{ 0 };
	eTime = time(NULL);
	std::cout << "Total completion time: " << eTime - sTime << '\n';
	return 0;
}
