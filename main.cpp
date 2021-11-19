#include <iostream>
#include <fstream>
#include <unordered_map>
#include<tuple>
#include <sstream>
#include <vector>
#include <cstdlib>
using namespace std;

const string indexFileName = "index.bin";
const string lexiconFileName = "lexicon.txt";
const string testIndexFileName = "test_index.bin";
const string urlsFileName = "test_urls.txt";
const char comma = ',';
const char space = ' ';
const char tab = '\t';
const char newline = '\n';

unordered_map <string, tuple<int, int, int>> lexicon;
unordered_map <string, int> URLs;

template <typename T>
void printVec(T vec);

void printLexicon();

ifstream indexReader(indexFileName);
ifstream URLsInStream(urlsFileName);

// declare the function prototypes
void loadLexicon();

void loadUrls();

vector<char> openList(string term);

vector<int> VBDecodeVec(const vector<char>& encodedData);

vector<int> VBDecodeFile(string filename);

int main() {
    cout << "The main begins!" << std::endl;

    // error check the streams
    if (!indexReader.is_open()) {
        cerr << "Error opening the index file " << endl;
        exit(1);
    }

    loadLexicon();

//    vector<char> invList = openList("courageous");
//    vector<int> decodedList = VBDecodeVec(invList);
    vector<int> decodedIndexFile = VBDecodeFile(indexFileName);
    printVec(decodedIndexFile);

    // close the streams
    indexReader.close();
    URLsInStream.close();

    return 0;
}


vector<char> read_com(ifstream& infile){
    char c;
    vector<char> result;
    while(infile.get(c)){
        result.push_back(c);
    }
    return result;
}

void write(vector<uint8_t> num, ofstream& ofile){
    for(auto it = num.begin(); it != num.end(); it++){
        ofile.write(reinterpret_cast<const char*>(&(*it)), 1);
    }
    ofile.close();
}


vector<int> VBDecodeVec(const vector<char>& encodedData) {
    vector<int> result;
    char c;
    int num;
    int p;

    for(auto it = encodedData.begin(); it != encodedData.end(); it++) {
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
        if (line.length() == 0) continue;
        // parse the line
        while ((start = line.find_first_not_of(tab, end)) != string::npos) {
            end = line.find(tab, start);
            temp.push_back(line.substr(start, end - start));
        }
        // process the data
        term = temp.at(0);
        termData = temp.at(1);
        temp.clear();
        end = 0;
        while ((start = termData.find_first_not_of(space, end)) != string::npos) {
            end = termData.find(space, start);
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
    // close the stream
    lexiconReader.close();
}


void loadUrls() {

    string line;
    string URL;
    int docId;

    while(getline(URLsInStream, line)) {
        stringstream lineStream(line);
        lineStream >> URL;
        lineStream >> docId;
        URLs.insert(make_pair(URL, docId));
    }

}


vector<char> openList(string term) {
    vector<int> invertedList;
    vector<char> encodedInvertedList;

    // retrieve the term data from the lexicon
    tuple <int, int, int> termData = lexicon.at(term);
    int startList = get<0>(termData);
    int endList = get<1>(termData);

    char nextByte;
    int numBytesToRead = endList - startList + 1;
    int count = 0;

    indexReader.seekg(startList, ios::beg);

    while (count < numBytesToRead) {
        indexReader.get(nextByte);
        encodedInvertedList.push_back(nextByte);
        count++;
    }

    return encodedInvertedList;
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
    return VBDecodeVec(invertedList);
}


template <typename T>
void printVec(T vec) {
    for (auto elem : vec)
        cout << elem << comma << space;
}


// a function that processes a conjunctive query
// and a returns a list of documents containing all the
// terms in a query
//vector<int> processConjunctive()


void printTuple(const string& term, const tuple<int, int, int>& entry) {
    cout << term << tab << get<0>(entry) << " " << get<1>(entry) << " " << get<2>(entry) << newline;
}


void printLexicon() {
    for (auto const& entry: lexicon) {
        printTuple(entry.first, entry.second);
    }
}


vector<int> VBDecodeFile(string filename) {
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

