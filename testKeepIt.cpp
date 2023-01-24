#include <iostream>
#include <string>
#include <vector>
#include <fstream>


//Function that check if str already contains in lib
bool isUnique(const std::string& str, std::vector<std::string> lib, int count) {
	for (int i = 0; i < count; i++)
	{
		if (lib[i] == str) {
			return false;
		}
	}
	return true;
}

long countUnique(std::string inputStr) {
	std::string bufferStr{ "" };
	//Main answer counter
	long uniqueWordCounter{ 0 };
	//Length of current bufferStr
	long wordLen{ 0 };
	//Library of uniques words in input string
	std::vector<std::string> strLib;

	for (auto ch = inputStr.begin(); ch != inputStr.end(); ++ch) {
		//Checking if meet a separator 
		if (((*ch == ' ') && (wordLen)) ||
			((*ch != ' ') && ((ch + 1) == inputStr.end()))) {

			if (isUnique(bufferStr, strLib, uniqueWordCounter)) {
				//Resizing library and adding new word in it
				strLib.resize(uniqueWordCounter + 1);
				strLib[uniqueWordCounter] = bufferStr;
				++uniqueWordCounter;
				std::cout << "Current number of Uniq: " << uniqueWordCounter << std::endl;
			}
			//Clean buffer
			bufferStr = "";
			wordLen = 0;
		}
		else if (*ch != ' ') {
			//Collecting symbols into one word and count it length
			bufferStr += *ch;
			++wordLen;
		}
	}
	return uniqueWordCounter;
}

int main()
{
	std::string inputStr;
	std::ifstream inp("input1.txt");
	if (inp.is_open())
	{
		std::getline(inp, inputStr);
		std::cout << inputStr << std::endl;
		std::cout << "Number of unique word in input string: " << countUnique(inputStr);
	}
	inp.close();

	return 0;
}