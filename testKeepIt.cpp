#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <ctime>
#include <unordered_set>


size_t countUnique(std::string fromFile) {
	std::ifstream input(fromFile);
	std::unordered_set<std::string> stringSet;
	if (input.is_open()) {
		std::string bufferStr{ "" };
		//Length of current bufferStr
		bool isWord{ false };
		//Library of uniques words in input string
		char ch;
		while (input.get(ch))
		{
			//Checking if meet a separator 
			if ((ch == ' ') && (isWord))  {
				stringSet.insert(bufferStr);
				bufferStr = "";
				isWord = false;
			}
			else if (ch != ' ') {
				bufferStr += ch;
				isWord = true;
			}
		}
		//Collecting last word
		if (input.eof())
			stringSet.insert(bufferStr);
	}
	return stringSet.size();
}

int main()
{
	time_t sTime{ 0 };
	sTime = time(NULL);

	std::string inputFile{"large_input.txt"};
	std:: cout << "Total unique words: " << countUnique(inputFile) << '\n';

	time_t eTime{ 0 };
	eTime = time(NULL);
	std::cout << "Total completeon time: " << eTime - sTime << '\n';
	return 0;
}