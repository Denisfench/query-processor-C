#include <iostream>
#include <fstream>
#include <unordered_map>
#include<tuple>
using namespace std;

const string indexFileName = "test_index.bin";
const string lexiconFileName = "test_lexicon.txt";

unordered_map<string, tuple<int, int, int>> loadLexicon() {
    fstream lexiconReader(lexiconFileName, ios::in);
    if (!lexiconReader.is_open())
        cerr << "Error opening index file " << endl;
    unordered_map<string, tuple<int, int, int>> lexicon;
    return lexicon;
}

byte* openList(string term) {
    fstream indexReader(indexFileName, ios::in | ios::binary);
    if (!indexReader.is_open())
        cerr << "Error opening index file " << endl;
    byte* ptr;
    return ptr;
}

int main() {
    std::cout << "Hello, World!" << std::endl;
    return 0;
}

