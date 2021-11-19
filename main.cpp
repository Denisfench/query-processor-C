#include <iostream>
#include <fstream>
#include <unordered_map>
#include<tuple>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <queue>
#include <set>
using namespace std;

const string indexFileName = "index.bin";
const string lexiconFileName = "lexicon.txt";
const string testIndexFileName = "test_index.bin";
const string urlsFileName = "test_urls.txt";
const char comma = ',';
const char space = ' ';
const char tab = '\t';
const char newline = '\n';
const int N = 777;
const int averageDocLength = 222;
const float k1 = 2;
const float b = 0.75;

unordered_map <string, tuple<int, int, int>> lexicon;
unordered_map <int, string> URLs;
// <docId : docRank>
priority_queue<tuple <int, int>> result;

template <typename T>
void printVec(T vec);

void printLexicon();

// index is the binary file
ifstream indexReader(indexFileName, ios::binary);
ifstream URLsInStream(urlsFileName);

// declare the function prototypes
void loadLexicon();
void loadUrls();
vector<char> openList(string term);
vector<int> VBDecodeVec(const vector<char>& encodedData);
vector<int> VBDecodeFile(string filename);
int getDocLength(const string& document);
vector<int> processConjunctive(const string& query);
vector<int> processDisjunctive(const string& query);

int getTermFreq(string term);

int main() {
    cout << "The main begins!" << std::endl;

    // error check the streams
    if (!indexReader.is_open()) {
        cerr << "Error opening the index file " << endl;
        exit(1);
    }

    loadLexicon();

    // queue test
//    result.push(make_pair(3, 40));
//    cout << "the top the queue is : " << get<0>(result.top()) << " " << get<1>(result.top()) << endl;


    // test openList() API
    vector<char> fInvList = openList("take");
    vector<int> fDecodedList = VBDecodeVec(fInvList);
    printVec(fDecodedList);

    cout << endl;

    vector<char> sInvList = openList("which");
    vector<int> sDecodedList = VBDecodeVec(sInvList);
    printVec(sDecodedList);

    cout << endl;

    // testing conjunctive
//    vector<int> result = processConjunctive("take which");
//    printVec(result);

// testing disjunctive
//    vector<int> result = processDisjunctive("take which");
//    printVec(result);

// file decoding test
//    vector<int> decodedIndexFile = VBDecodeFile(indexFileName);
//    printVec(decodedIndexFile);

    // close the streams
    indexReader.close();
    URLsInStream.close();

    return 0;
}


// retrieves the frequency of the term in the collection
int getTermFreq(string term) {
    // sum up the entries in the decoded inverted list for the term that will be loaded in memory
    return 99;
}


int getDocLength(const string& document) {
    // requires an extra loop over the document collection using program X
    return 999;
}


int getTotalNumDocs() {
    return 5555;
}


int BM25(const string& query, const string& document) {
    stringstream lineStream(query);
    int K = 0;
    int result = 0;
    string term;
    int termFreq;
    while (lineStream >> term) {
        K = k1 * ((1 - b) + b * getDocLength(document) / averageDocLength);
        termFreq = getTermFreq(term);
        result += log((N - termFreq + 0.5) / (termFreq + 0.5)) * ((k1 + 1) * termFreq / (K + termFreq));
    }
    return 99;
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

// TODO: change the line parsing logic
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
        URLs.insert(make_pair(docId, URL));
    }

}


vector<char> openList(string term) {
    vector<int> invertedList;
    vector<char> encodedInvertedList;

    // retrieve the term data from the lexicon
    tuple <int, int, int> termData = lexicon.at(term);
    long startList = get<0>(termData);
    long endList = get<1>(termData);
    char nextByte;
//    long numBytesToRead = endList - startList + 1;
    long numBytesToRead = endList - startList;
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
// suppose there are 2 query terms for now
vector<int> processConjunctive(const string& query) {
    vector<int> result;
    stringstream lineStream(query);
    string term;
    lineStream >> term;
    vector<char> firstInvList = openList(term);
    vector<int> firstDecodedList = VBDecodeVec(firstInvList);
    lineStream >> term;
    vector<char> secInvList = openList(term);
    vector<int> secDecodedList = VBDecodeVec(secInvList);
    // <docID, termFreq>
    unordered_map <int, int> firstList;
    unordered_map <int, int> secList;
    for (vector<char>::iterator it = firstInvList.begin(); it != firstInvList.end(); it += 2)
        firstList.insert(make_pair(*it, *(it + 1)));

    for (vector<char>::iterator it = secInvList.begin(); it != secInvList.end(); it += 2)
        secList.insert(make_pair(*it, *(it + 1)));

    // find the documents containing both query terms
    // if the first lost is shorter, iterate over it
    if (firstList.size() < secList.size()) {
        for (auto entry = firstList.begin(); entry != firstList.end(); entry++) {
            if (secList.find(entry->first) != secList.end())
                result.push_back(entry->first);
        }
    }
    else {
        for (auto entry = secList.begin(); entry != secList.end(); entry++) {
            if (firstList.find(entry->first) != firstList.end())
                result.push_back(entry->first);
        }
    }
    return result;
}


vector<int> processDisjunctive(const string& query) {
    set<int> uniqDocs;
    vector<int> result;
    stringstream lineStream(query);
    string term;
    lineStream >> term;
    vector<char> firstInvList = openList(term);
    vector<int> firstDecodedList = VBDecodeVec(firstInvList);
    lineStream >> term;
    vector<char> secInvList = openList(term);
    vector<int> secDecodedList = VBDecodeVec(secInvList);
    for (vector<char>::iterator it = firstInvList.begin(); it != firstInvList.end(); it += 2)
        uniqDocs.insert(*it);

    for (vector<char>::iterator it = secInvList.begin(); it != secInvList.end(); it += 2)
        uniqDocs.insert(*it);

    for (auto itr = uniqDocs.begin(); itr != uniqDocs.end(); itr++)
        result.push_back(*itr);

    return result;
}

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

    for(vector<char>::iterator it = vec.begin(); it != vec.end(); it++) {
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

