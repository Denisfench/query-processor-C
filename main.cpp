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

//const string indexFileName = "index.bin";
const string indexFileName = "Nov_25_test_index.bin";

//const string lexiconFileName = "test_lexicon.txt";
const string lexiconFileName = "Nov_25_test_lexicon.txt";

const string testIndexFileName = "test_index.bin";
//const string urlsFileName = "test_urls.txt";
const string urlsFileName = "urls_dict.txt";
const string docPositionsFilename = "doc_locations.txt";
//const string docCollectionFileName = "testFile.trec";
const string docCollectionFileName = "../collection-metadata-generator/web_data.trec";
const string docCollectionDataName = "docCollectionData.txt";
const int maxDocId = 3213840;

const string quit = "Q";
const char comma = ',';
const char space = ' ';
const char tab = '\t';
const char newline = '\n';
const int N = 3213835;
const int averageDocLength = 200;
const float k1 = 2;
const float b = 0.75;
const char CONJUNCTIVE = 'C';
const char DISJUNCTIVE = 'D';

unordered_map <string, tuple<int, int, int>> lexicon;
unordered_map <int, string> URLs;
unordered_map <string, tuple<int, long, long>> docLocations;

// <docId : docRank>
priority_queue<tuple <int, int>> result;

template <typename T>
void printVec(T vec);

void printLexicon();

// index is the binary file
ifstream indexReader(indexFileName, ios::binary);
ifstream URLsInStream(urlsFileName);
ifstream docPositionsStream(urlsFileName);
ifstream docCollectionStream(docCollectionFileName);
ifstream docCollectionDataIn(docCollectionDataName);
ifstream lp1(indexFileName, ios::binary);
ifstream lp2(indexFileName, ios::binary);

// declare the function prototypes
void loadLexicon();
void loadUrls();
vector<char> intersectLists(string term);
vector<int> VBDecodeVec(const vector<char>& encodedData);
vector<int> VBDecodeFile(string filename);
int getDocLength(int document);
tuple<vector<int>, int > processConjunctive(const string& query);
vector<int> processDisjunctive(const string& query);
int getTermFreq(const tuple<vector<int>, int> & docs);
int VBDecodeByte(const char& byte);

string getURL(int docID);
void rankPages(const vector<int>& docs);
int getTermFreq(string term);
void loadDocLocations();
void printDocLocations();
string getUserInput();
void printDocByURL(string URL);
void printUrls();


int main() {
    cout << "The main begins!" << endl;

    // error check the streams
    if (!indexReader.is_open()) {
        cerr << "Error opening the index file " << endl;
        exit(1);
    }

    // load the lexicon structure into memory
    cout << "Loading the lexicon into memory..." << endl;

//    vector<int> decodedList = VBDecodeFile(indexFileName);
//    printVec(decodedList);

    loadLexicon();
//    loadUrls();
//    loadDocLocations();
////
//    printDocLocations();
//    string query = getUserInput();

    vector<int> result_1 = processDisjunctive("well");
    printVec(result_1);
    cout << "///////////////////" << endl;
    vector<int> result_2 = processDisjunctive("wellness");
    printVec(result_2);

//    cout << "The docID is " << result.at(0) << endl;
//    printDocByURL(getURL(result.at(0)));
//    cout << getURL(result.at(0)) << endl;

    // close the streams
    indexReader.close();
    URLsInStream.close();
    docPositionsStream.close();
    return 0;
}


void printDocByURL(string URL) {
    tuple<int, long, long> docLocation = docLocations[URL];
    if (docLocations.empty())
        cout << "Failed to find the corresponding document" << endl;
    long docStart = get<1>(docLocation);
    long docEnd = get<2>(docLocation);
    long toRead = docEnd - docStart;
    int count = 0;
    char nextChar;
    docCollectionStream.seekg(docStart);
    while (count < toRead) {
        docCollectionStream.get(nextChar);
        cout << nextChar;
        count++;
    }
}


string getUserInput() {
//    char mode;
//    cout << "Would you like to run conjunctive or disjunctive query? [C] or [D]" << endl;
//    cin >> mode;
    string query;
    cout << "Please enter your query" << endl;
    cin >> query;
    return query;
}


// - 1 is the adjustment ?
string getURL(int docID) {
    cout << "looking for the " << docID - 1 << endl;
    if (URLs.find(docID - 1) == URLs.end()) {
        return "The URL not found";
    }
    return URLs[docID - 1];
}


int getDocLength(int docID) {
    string docURL = URLs[docID];
    tuple <int, long, long> location = docLocations[docURL];
    return get<1>(location) - get<0>(location);
}


int BM25(const string& query, int document, int termFreq) {
    stringstream lineStream(query);
    int K = 0;
    int result = 0;
    string term;
    while (lineStream >> term) {
        K = k1 * ((1 - b) + b * getDocLength(document) / averageDocLength);
//        termFreq = getTermFreq(term);
        result += log((N - termFreq + 0.5) / (termFreq + 0.5)) * ((k1 + 1) * termFreq / (K + termFreq));
    }
    return result;
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


// TODO: needs to be tested
int VBDecodeByte(const char& byteId) {
    char c;
    int num;
    int p;
    c = byteId;
    bitset<8> byte(c);
    num = 0;
    p = 0;
    while(byte[7] == 1){
        byte.flip(7);
        num += byte.to_ulong()*pow(128, p);
        p++;
        byte = bitset<8>(c);
    }
    num = (byte.to_ulong())*pow(128, p);
    return num;
}


// TODO: parsing logic can be refactored
void loadLexicon() {
    fstream lexiconReader(lexiconFileName, ios::in);
    if (!lexiconReader.is_open())
        cerr << "Error opening lexicon file " << endl;
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


void loadDocLocations() {
    string line;
    string URL;
    int docId;
    long docStart;
    long docEnd;
    tuple <int, long, long> location;
    while(getline(docPositionsStream, line)) {
        stringstream lineStream(line);
        lineStream >> URL;
        lineStream >> docId;
        lineStream >> docStart;
        lineStream >> docEnd;
        location = make_tuple(docId, docStart, docEnd);
        docLocations.insert(make_pair(URL, location));
    }
}


void printDocLocations() {
    for (auto const &pair: docLocations)
        cout << "{" << pair.first << ": " << get<0>(pair.second) <<
             "," << get<1>(pair.second) << "}\n";
}


vector<char> intersectLists(string term) {
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


// TODO: index <ddocID, freq, docID, freq>
// TODO: you have to be skipping frequencies
// TODO: you are not reading the encoded list correctly
vector<int> getTermDocs(string term) {
    vector<char> encodedList;
    vector<int> decodedList;
    tuple<int, int, int> termData;
    // retrieve the term data from the lexicon if it is present

    if (lexicon.find(term) != lexicon.end()) {
        termData = lexicon.at(term);
    }

    else {
        cout << "There Aren't Any Great Matches for Your Search" << endl;
        return decodedList;
    }

    long startList = get<0>(termData);
    long endList = get<1>(termData);

    cout << "startList is " << startList << endl;
    cout << "endList is " << endList << endl;
    char nextByte;
    long numBytesToRead = endList - startList;
    int count = 0;

    indexReader.seekg(startList, ios::beg);

    while (count < numBytesToRead) {
        indexReader.get(nextByte);
        encodedList.push_back(nextByte);
        count += 2;
        indexReader.get(nextByte);
    }

   return VBDecodeVec(encodedList);
}


long openList(const string& term, ifstream& fileStream) {
    // retrieve the term data from the lexicon
    tuple <int, int, int> termData = lexicon.at(term);
    long startList = get<0>(termData);
    long endList = get<1>(termData);
    fileStream.seekg(startList, ios::beg);
    return endList;
}


int nextGEQ(int currDocId, ifstream& listStream, long endList) {
    char nextByte;
    char docId = - 1;
    while (listStream.tellg() != endList && docId < currDocId) {
        listStream.get(nextByte);
        // decode the byte and retrieve the int docID
        docId = VBDecodeByte(nextByte);
    }
    return docId;
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
    for (auto const &elem : vec)
        cout << elem << comma << space;
}


// a function that processes a conjunctive query
// and a returns a list of documents containing all the
// terms in a query
// suppose there are 2 query terms for now
tuple<vector<int>, int > processConjunctive(const string& query) {
    int termFreq = 0;
    vector<int> result;
    stringstream lineStream(query);
    string term;
    lineStream >> term;
    vector<char> firstInvList = intersectLists(term);
    vector<int> firstDecodedList = VBDecodeVec(firstInvList);
    lineStream >> term;
    vector<char> secInvList = intersectLists(term);
    vector<int> secDecodedList = VBDecodeVec(secInvList);
    // <docID, termFreq>
    unordered_map <int, int> firstList;
    unordered_map <int, int> secList;
    for (vector<char>::iterator it = firstInvList.begin(); it != firstInvList.end(); it += 2) {
        termFreq += *(it + 1);
        firstList.insert(make_pair(*it, *(it + 1)));
    }

    for (vector<char>::iterator it = secInvList.begin(); it != secInvList.end(); it += 2) {
        termFreq += *(it + 1);
        secList.insert(make_pair(*it, *(it + 1)));
    }

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
    return make_pair(result, termFreq);
}

// TODO: lexicon has the doc count, no need to return it from here
vector<int> processDisjunctive(const string& query) {
    cout << "The query is " << query << endl;
    vector<int> result;
    int termFreq = 0;
    vector<string> queryTerms;
    string term;
    stringstream queryStream(query);
    while (queryStream >> term)
        queryTerms.push_back(term);

    // query is empty case
    if (queryTerms.empty())
        return result;

    // query has only 1 term
    if (queryTerms.size() == 1) {
        term = queryTerms.at(0);
        cout << "The term is " << term << endl;
        // return the list of documents containing the term
        return getTermDocs(term);
    }

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


void printUrls() {
    for (auto const& url: URLs)
        cout << "docID " << url.first << " " << "URL " << url.second << endl;
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
//            cout << "num " << num << endl;
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

