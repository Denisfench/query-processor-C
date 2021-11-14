#include <iostream>
#include <fstream>
#include <unordered_map>
#include<tuple>
#include <sstream>
#include <vector>
#include <cstdlib>
using namespace std;

const string indexFileName = "test_index.bin";
const string lexiconFileName = "test_lexicon.txt";
const char tabDelim = '\t';
const char commaDelim = ',';


unordered_map<string, tuple<int, int, int>> loadLexicon() {
    fstream lexiconReader(lexiconFileName, ios::in);
    if (!lexiconReader.is_open())
        cerr << "Error opening index file " << endl;
    unordered_map<string, tuple<int, int, int>> lexicon;
    string line;
    size_t start;
    vector<string> temp;
    size_t end = 0;
    string term;
    string termData;
    int termStart;
    int termEnd;
    int numTerms;
    // read a line from the lexicon
    while (getline(lexiconReader, line)) {
        // parse the line
        while ((start = line.find_first_not_of(tabDelim, end)) != string::npos) {
            end = line.find(tabDelim, start);
            temp.push_back(line.substr(start, end - start));
        }
        // process the data
        term = temp.at(0);
        termData = temp.at(1);
        temp.clear();
        end = 0;
        while ((start = termData.find_first_not_of(commaDelim, end)) != string::npos) {
            end = termData.find(commaDelim, start);
            temp.push_back(termData.substr(start, end - start));
        }
        stringstream termStartStream(temp.at(0));
        termStartStream >> termStart;
        stringstream termEndStream(temp.at(1));
        termEndStream >> termEnd;
        stringstream numDocsStream(temp.at(2));
        numDocsStream >> numTerms;
        tuple <int,char,float> tup1(20,'g',17.5);
        lexicon.insert(make_pair(term, make_tuple(termStart, termEnd, numTerms)));
        temp.clear();
        end = 0;
    }
//    while (getline(lexiconReader, lexiconFileName)) {
//
//    }
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
    cout << "The main begins!" << std::endl;
    loadLexicon();
    return 0;
}

