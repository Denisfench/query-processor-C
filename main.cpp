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
unordered_map <string, tuple<int, int, int>> lexicon;
const string testIndexFileName = "test_index.bin";


template <typename T>
void printVec(T vec);

vector<char> read_com(ifstream& infile){
    char c;
    vector<char> result;
    while(infile.get(c)){
        result.push_back(c);
    }
    return result;
}

void write(vector<uint8_t> num, ofstream& ofile){
    for(vector<uint8_t>::iterator it = num.begin(); it != num.end(); it++){
        ofile.write(reinterpret_cast<const char*>(&(*it)), 1);
    }
    ofile.close();
}


vector<int> VBDecode(string filename){
    ifstream ifile;
    ifile.open(filename, ios::binary);
    char c;
    int num;
    int p;
    vector<int> result;
    vector<char> vec = read_com(ifile);

    for(vector<char>::iterator it = vec.begin(); it != vec.end(); it++){
        c = *it;
        bitset<8> byte(c);
        num = 0;
        p = 0;
        while(byte[7] == 1){
            byte.flip(7);
            num += byte.to_ulong()*pow(128, p);
            cout << "num " << num << endl;
            p++;
            it ++;
            c = *it;
            byte = bitset<8>(c);
        }
        num += (byte.to_ulong())*pow(128, p);

        result.push_back(num);
    }
    return result;
}


vector<int> VBDecodeList(vector<char>& encodedData){
    vector<int> result;
    char c;
    int num;
    int p;

    for(vector<char>::iterator it = encodedData.begin(); it != encodedData.end(); it++) {
        c = *it;
        bitset<8> byte(c);
        num = 0;
        p = 0;
        while(byte[7] == 1){
            byte.flip(7);
            num += byte.to_ulong()*pow(128, p);
            cout << "num " << num << endl;
            p++;
            it ++;
            c = *it;
            byte = bitset<8>(c);
        }
        num += (byte.to_ulong())*pow(128, p);
        result.push_back(num);
    }
    return result;
}


void VBEncode(unsigned int num){
    ofstream ofile;
    ofile.open(testIndexFileName, ios::binary);
    vector<uint8_t> result;
    uint8_t b;
    while(num >= 128){
        int a = num % 128;
        bitset<8> byte(a);
        byte.flip(7);
        num = (num - a) / 128;
        b = byte.to_ulong();
        cout << byte << endl;
        result.push_back(b);
    }
    int a = num % 128;
    bitset<8> byte(a);
    cout << byte << endl;
    b = byte.to_ulong();
    result.push_back(b);
    write(result, ofile);
}


void loadLexicon() {
    fstream lexiconReader(lexiconFileName, ios::in);
    if (!lexiconReader.is_open())
        cerr << "Error opening index file " << endl;
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
        lexicon.insert(make_pair(term, make_tuple(termStart, termEnd, numTerms)));
        temp.clear();
        end = 0;
    }
}


vector<int> openList(string term) {
    vector<int> invertedList;
    vector<char> encodedInvertedList;

    ifstream indexReader(indexFileName, ios::binary);
    if (!indexReader.is_open())
        cerr << "Error opening index file " << endl;
    // retrieve the term data from the lexicon
    tuple <int, int, int> termData = lexicon.at(term);
    int startList = get<0>(termData);
    int endList = get<1>(termData);
    cout << "Term is " << term << endl;
    cout << "The start of the list is " << startList << endl;
    cout << "The end of the list is " << endList << endl;
    // seekg sets the read pointer
    indexReader.seekg(startList);
    char nextByte;
    int numBytesToRead = endList - startList + 1;
    int count = 0;
//    myFile.seekg(6, ios::beg);
    while (count < numBytesToRead) {
        indexReader.get(nextByte);
        encodedInvertedList.push_back(nextByte);
        count++;
    }
    invertedList = VBDecodeList(encodedInvertedList);
    return invertedList;
}


void printLexicon(unordered_map<string, tuple<int, int, int>>& lexicon) {
    for (auto const &pair: lexicon)
        cout << "{" << pair.first << ": " << get<0>(pair.second) <<
             "," << get<1>(pair.second) << "," << get<2>(pair.second) << "}\n";
}


vector<int> loadAndPrintIndex() {
    vector<char> invertedList;
    char nextInt;
    fstream indexReader(testIndexFileName, ios::in | ios::binary);
    if (!indexReader.is_open())
        cerr << "Error opening index file " << endl;
    while (indexReader) {
        indexReader.read((char*)& nextInt, sizeof(int));
        invertedList.push_back(nextInt);
    }
    return VBDecodeList(invertedList);
}


template <typename T>
void printVec(T vec) {
    cout << "Printing the vector " << endl;
    for (auto elem : vec)
        cout << elem << endl;
}


int main() {
    cout << "The main begins!" << std::endl;
    loadLexicon();
    vector<int> invList = openList("suffered");
    printVec(invList);
//    VBEncode(9999);
////    vector<int> decoded = VBDecodeList();
//    vector<int> lex = loadAndPrintIndex();
//    for (auto i : lex)
//        cout << i << endl;

//    uint8_t a = 255;
//    vector<char> c;
//    vector<int> i;
//    int in;
//    cin >> in;
//    VBEncode(in);
//    i = VBDecode(testIndexFileName);
//    for (int & it : i)
//        cout << "result " << it << endl;

    return 0;
}

