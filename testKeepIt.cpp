#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <ctime>
#include <unordered_set>
#include <thread>
#include <future>

//Number of cores in processor
const int processor_count = std::thread::hardware_concurrency();

//Function that calculates file's size
int fileSize(std::string filename) {
	std::ifstream in_file(filename, std::ios::binary);
	in_file.seekg(0, std::ios::end);
	int file_size = in_file.tellg();
	return file_size;
}

//Structure to catch and contain pieces of words from parallelized chunk
struct connector {
	std::string firstWord;
	std::string lastWord;
};

//Function for single thread. 
//Fill connector container by piece of first word and piece of last word in a range of char symbols from file.
//All of words between first and last inserting in thread unordered set
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

//Function that check if pieces of words from different thread is one word and inserting that words to resulting unordered set
void connectPieces(	std::vector<connector>& pieces, 
					std::unordered_set<std::string>& stringSet) {
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

//Single thread unordered sets merge
void setInsert(	std::unordered_set<std::string>& set1, 
				std::unordered_set<std::string>& set2, int& endFlag) {
	set1.insert(set2.begin(), set2.end());
	++endFlag;
}

//Function that merging all of sets from vector to resulting set with binary steps
void concatSets(std::vector<std::unordered_set<std::string>>& sets, 
				std::unordered_set<std::string>& mainSet) {
	int length = sets.size();
	while (length > 1) {
		int endFlag{ 0 };
		if (length % 2 == 1)
			std::thread(setInsert, 
						std::ref(mainSet), 
						std::ref(sets[length-1]), 
						std::ref(endFlag)).detach();
		int halflen = length / 2;
		for (int i = 0; i < halflen; i++)
			std::thread(setInsert, 
						std::ref(sets[i]), 
						std::ref(sets[i+halflen]), 
						std::ref(endFlag)).detach();
		while (endFlag < (halflen + (length % 2))) { 
			std::this_thread::sleep_for(std::chrono::milliseconds(100)); 
		}
		length /= 2;
		sets.resize(length);																	//Clear some memory
	}
	mainSet.insert(sets[0].begin(), sets[0].end());
}

//Function that creating parallel tasks to read chunks of chars from given file and returns number of unique words in it
//In the case of file size < 1e7 it used only one core, because it gives better perfomance
int countChunkUnique(std::string fromFile) {
	std::ifstream input(fromFile);
	std::vector<connector> wordsParts;
	std::vector<std::unordered_set<std::string>> stringSets;
	std::unordered_set<std::string> stringSet;
	if (input.is_open()) {
		int endFlag{ 0 };
		int fileEnd{ fileSize(fromFile)};
		if (fileEnd < 1e7) {
			std::cout << "Joining thread 0" << '\n';
			stringSets.resize(1);
			wordsParts.resize(1);
			readChunk(0, fileEnd, 0, wordsParts, fromFile, stringSets[0], endFlag);				//First zero here is file start and second is 0 core
		}
		else {
			wordsParts.resize(processor_count);
			stringSets.resize(processor_count);
			int chunkSize {fileEnd/processor_count};
			for (int i = 0; i < processor_count; i++)	{
				int startPoint{i * chunkSize};
				int endPoint{(i == processor_count - 1) ? fileEnd : (i + 1) * chunkSize};		//In the last chunk alg need to go to file end
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
			while (endFlag != processor_count) { 
				std::this_thread::sleep_for(std::chrono::milliseconds(100));					//Waiting for all thread to complete
			}	
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
	
	std::string inputFile{"input4.txt"};
	std::cout << "Total unique words: " << countChunkUnique(inputFile) << '\n';

	time_t eTime{ 0 };
	eTime = time(NULL);
	std::cout << "Total completion time: " << eTime - sTime << '\n';
	return 0;
}
