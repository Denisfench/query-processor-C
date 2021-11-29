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
// * N and dAvg below are being retrieved from the docMetadataFile
// * N is the number of documents in the collection
// * dAvg is the average length of the document
const int N = 3213834;
const int dAvg = 302;
const float k1 = 2;
const float b = 0.75;
const char CONJUNCTIVE = 'C';
const char DISJUNCTIVE = 'D';

// * customer comparator that allows us to order tuples by the second column
// * in a priority queue
struct termLengthComparator {
  bool operator()(tuple<string, int>& t1, tuple<string, int>& t2) {
    return get<1>(t1) > get<1>(t2);
  }
};

// * <term : <indexStartOffset, indexEndOffset, collectionFreqCount> >
unordered_map <string, tuple<int, int, int>> lexicon;

// TODO: deprecated, use docMap instead
unordered_map <int, string> URLs;

// TODO: deprecated, use docMap instead
unordered_map <string, tuple<int, long, long>> docLocations;

// * <docID : <URL, termCount, webDataStartOffset, webDataEndOffset> >
unordered_map <int, tuple<string, int, long, long>> docMap;

// TODO: implement a custom comparator to change the order to <docID : docRank>
// * <docRank : docId>
priority_queue<pair <int, int>> top10Results;

template <typename T>
void printVec(T vec);

void printLexicon();

// index is the binary file
ifstream indexReader(indexFileName, ios::binary);
ifstream URLsInStream(urlsFileName);
ifstream docPositionsStream(urlsFileName);
ifstream docCollectionStream(docCollectionFileName);
ifstream docCollectionDataIn(docCollectionDataName);

// declare the function prototypes
void loadLexicon();
void loadUrls();
vector<char> intersectLists(string term);
vector<int> VBDecodeVec(const vector<char>& encodedData);
vector<int> VBDecodeFile(string filename);
int getDocLength(int docID);
vector<int> processConjunctive(const string& query);
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
int getTermDocFreq(const string& term, int docID);
int getTermColFreq(const string& term);
int rankDoc(const string& term, int docID);
vector<int> getTermDocsDiff(const string& term);


// TODO: conjunctive AND ; disjunctive OR -> you've the function names
//  backwards
int main() {
    cout << "The main begins!" << endl;

    // error check the streams
    // TODO: check other streams
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
//    cout << "testing the implementation of disjunctive query" << endl;
//    vector<int> result_1 = processDisjunctive("well wellness welcomes water");
//    printVec(result_1);

//        cout << "\n \n ******************************** \n\n" << endl;
//        vector<int> termDocs = getTermDocsDiff("well");
//        printVec(termDocs);
//        cout << "********************************" << endl;

    cout << "testing the implementation of conjunctive query" << endl;
    vector<int> result_1 = processConjunctive("well wellness welcomes water");
    printVec(result_1);

//    cout << "********************************" << endl;
//    int termFreq = getTermDocFreq("well", 16);
//    cout << "termFreq " <<  termFreq << endl;
//    cout << "********************************" << endl;
//    int docRank = rankDoc("well", 1);
//    cout << "The rank of the document is " << docRank << endl;

    // BEGIN PLAYGROUND TEST
//    tuple<string, int, long, long> termData = make_tuple("http/cats.com", 10, 299, 399);
//    docMap.insert(make_pair(99, termData));
//
//    int termFreq = getTermDocFreq("term", 99);
//
//    cout << "The termFreq is " << termFreq << endl;
    // END PLAYGROUND TEST

    /*
    int main () {
        // <docRank docID>
        tuple<int, int> doc = make_pair(10, 9);
        tuple<int, int> doc1 = make_pair(10, 300);
        tuple<int, int> doc2 = make_pair(10, 300);
        result.push(doc);
        result.push(doc1);
        result.push(doc2);
        pair<int, int> top = result.top();
        cout << top.first << " " << top.second;
    */

//    cout << "The docID is " << result.at(0) << endl;
//    printDocByURL(getURL(result.at(0)));
//    cout << getURL(result.at(0)) << endl;

    // close the streams
    indexReader.close();
    URLsInStream.close();
    docPositionsStream.close();
    return 0;
}

// TODO: docLocations interface has changed
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


// * <docID : <URL, termCount, webDataStartOffset, webDataEndOffset> >
int getDocLength(int docID) {
  if (docMap.find(docID) == docMap.end()) {
    cout << "docID wasn't found in the document map" << endl;
    return -1;
  }
  return get<1>(docMap[docID]);
}


// rankDoc() takes a term and a docID of the documents containing the term as parameters
// and returns an integer ranking of that document with respect to the term
// * fDt is the frequency of the given term in the document with a given docID
// * fT is the frequency of the given term in the collection of documents
// * dAvg is the average length of a document in our collection
// * k is the constant set to 2
// * b is the constant set to 0.75
// * d is the document length
int rankDoc(const string& term, int docID) {
    int K = 0;
    int docRank = 0;
    int fDt = getTermDocFreq(term, docID);
    int fT = getTermColFreq(term);
    int d = getDocLength(docID);
    K = k1 * ((1 - b) + b * d / dAvg);
    fDt = log((N - fDt + 0.5) / (fDt + 0.5)) * ((k1 + 1) * fDt / (K + fDt));
    return abs(fDt);
}

// * docMap description
// * <docID : <URL, termCount, webDataStartOffset, webDataEndOffset> >
//int getTermDocFreq(const string& term, int docID) {
////    m.find(key) == m.end()
//    if (docMap.find(docID) == docMap.end())
//        cout << "Failed to find the corresponding document" << endl;
//    long docStart = get<2>(docMap[docID]);
//    long docEnd = get<3>(docMap[docID]);
//    char nextByte;
//    long numBytesToRead = endList - startList;
//    int count = 0;
//
//    indexReader.seekg(startList, ios::beg);
//
//    while (count < numBytesToRead) {
//        indexReader.get(nextByte);
//        encodedList.push_back(nextByte);
//        count += 2;
//        indexReader.get(nextByte);
//    }
//
//    return VBDecodeVec(encodedList);
//}

int getTermDocFreq(const string& term, int docID) {
    if (lexicon.find(term) == lexicon.end()) {
        cout << "There Aren't Any Great Matches for Your Search" << endl;
        return - 1;
    }

    long startList = get<0>(lexicon.at(term));
    long endList = get<1>(lexicon.at(term));

    cout << "start list " << startList << endl;
    cout << "end list " << endList << endl;

    char nextByte;
    long numBytesToRead = endList - startList;
    int count = 0;
    int currDocId = 0;
    indexReader.seekg(startList, ios::beg);

    while (count < numBytesToRead) {
        indexReader.get(nextByte);
        currDocId += VBDecodeByte(nextByte);
        cout << "currDocId " << currDocId << endl;
        if (currDocId == docID) {
            indexReader.get(nextByte);
            return VBDecodeByte(nextByte);
        }
        count += 2;
        indexReader.get(nextByte);
    }
    return - 1;
}

// * lexicon definition
// * <term : <indexStartOffset, indexEndOffset, collectionFreqCount> >
int getTermColFreq(const string& term) {
    if (lexicon.find(term) == lexicon.end()) {
      cout << "We couldn't find the query term " << term << " in our lexicon"
           << endl;
      return - 1;
    }
  return get<2>(lexicon[term]);
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


// TODO: index <docID, freq, docID, freq>
vector<int> getTermDocsDiff(const string& term) {
    cout << "term requested " << term << endl;
    vector<char> encodedList;
    vector<int> decodedList;
    tuple<int, int, int> termData;

    // * retrieve the term data from the lexicon if it is present
    if (lexicon.find(term) != lexicon.end()) {
        termData = lexicon.at(term);
    }

    else {
        cout << "There Aren't Any Great Matches for Your Search" << endl;
        return decodedList;
    }

    long startList = get<0>(termData);
    long endList = get<1>(termData);

    char nextByte;
    long numBytesToRead = endList - startList;
    int count = 0;

    indexReader.seekg(startList, ios::beg);

    while (count < numBytesToRead) {
        indexReader.get(nextByte);
        encodedList.push_back(nextByte);
        count += 2;
        indexReader.get();
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


// TODO: the query should be parsed in user input function
// * a function that takes in a user query as an input and returns all 
// * documents containing the query as an output 
vector<int> processConjunctive(const string& query) {
  set<int> result;
  vector<int> vecResult;
  vector<int> docs;
  int currDocId = 0;
  // * parse the user query
  vector<string> queryTerms;
  string term;
  stringstream queryStream(query);
  while (queryStream >> term)
    queryTerms.push_back(term);

  // * the query is empty
  if (queryTerms.empty())
    return vecResult;

  for (const string& word : queryTerms) {
    docs = getTermDocsDiff(word);
    for (int docIdDiff : docs) {
      currDocId += docIdDiff;
      result.insert(currDocId);
    }
    currDocId = 0;
  }

  // * copy set into the vector
  vecResult.assign(result.begin(), result.end());

  return vecResult;
}


// * a function that takes in a user query as an input and returns all 
// * documents containing every term in the query 
// TODO: the query should be parsed in user input function
vector<int> processDisjunctive(const string& query) {
    // * termLists is a min heap mapping terms to their inverted list lengths
    // * <term : invertedListLength>
    priority_queue<tuple<string, int>, vector<tuple<string, int>>,
                   termLengthComparator>
        termListMap;

    vector<int> prevIntersected;
    vector<int> currIntersected;

    // * parse the user query
    vector<string> queryTerms;
    string term;
    stringstream queryStream(query);
    while (queryStream >> term)
        queryTerms.push_back(term);

    // * the query is empty
    if (queryTerms.empty())
        return currIntersected;

    // * query has only 1 term
    if (queryTerms.size() == 1) {
        term = queryTerms.at(0);
        // * return the list of documents containing the term
        return getTermDocsDiff(term);
    }

    // * if the query has 2 or more terms, then
    // * all the docs containing the terms must be intersected

    // * put all the terms in the termListMap min heap
    // * and ensure that we have terms in the lexicon
    for (const string& queryTerm : queryTerms) {
      if (lexicon.find(queryTerm) == lexicon.end()) {
      cout << "There Aren't Any Great Matches for Your Search" << endl;
      return currIntersected;
      }
      termListMap.push(make_pair(queryTerm, get<2>(lexicon[queryTerm])));
    }

    // * initialize list streams
    ifstream lp1(indexFileName, ios::binary);
    ifstream lp2(indexFileName, ios::binary);

    string term1 = get<0>(termListMap.top());
    termListMap.pop();
    string term2 = get<0>(termListMap.top());
    termListMap.pop();

    // * intersect the first 2 lists
    long startList1 = get<0>(lexicon[term1]);
    long endList1 = get<1>(lexicon[term1]);

    long startList2 = get<0>(lexicon[term2]);
    long endList2 = get<1>(lexicon[term2]);

    char nextByteList1;
    int decodedByteList1 = 0;
    long numBytesToReadList1 = endList1 - startList1;
    int numBytesReadList1 = 0;

    char nextByteList2;
    int decodedByteList2 = 0;
    long numBytesToReadList2 = endList2 - startList2;
    int numBytesReadList2 = 0;

    // * set both streams to point to their
    // * corresponding lists in the index
    lp1.seekg(startList1, ios::beg);
    lp2.seekg(startList2, ios::beg);

    // * grab the initial values for both lists
    lp1.get(nextByteList1);
    lp2.get(nextByteList2);

    // * decode the values
    decodedByteList1 += VBDecodeByte(nextByteList1);
    decodedByteList2 += VBDecodeByte(nextByteList2);

    // * list 1 is smaller than list 2 (min heap condition)
    // * need both conditions in the while loops because of the forward skips
    // * to find the next greatest element
    while (numBytesReadList1 < numBytesToReadList1 &&
           numBytesReadList2 < numBytesToReadList2) {

      // * move both list pointers forward
      if (decodedByteList1 == decodedByteList2) {
        // * shift the list1 pointer from frequency on the docID
        lp1.get();
        // * grab the next docID in the list 1
        lp1.get(nextByteList1);
        // * shift the list2 pointer from frequency on the docID
        lp2.get();
        // * grab the next docID in the list 2
        lp2.get(nextByteList2);

        // * increment the counters
        numBytesReadList1 += 2;
        numBytesReadList2 += 2;

        // * add the common docID to our result collection
        currIntersected.push_back(decodedByteList1);
        decodedByteList1 += VBDecodeByte(nextByteList1);
        decodedByteList2 += VBDecodeByte(nextByteList2);
      }

      // * move the first list pointer forward
      else if (decodedByteList1 < decodedByteList2) {
          // * shift the list1 pointer from frequency on the docID
          lp1.get();
          // * grab the next docID in the list 1
          lp1.get(nextByteList1);
          numBytesReadList1 += 2;
          decodedByteList1 += VBDecodeByte(nextByteList1);
      }

      // * move the second list pointer forward
      else {
          // * shift the list2 pointer from frequency on the docID
            lp2.get();
          // * grab the next docID in the list 2
            lp2.get(nextByteList2);
            numBytesReadList2 += 2;
            decodedByteList2 += VBDecodeByte(nextByteList2);
        }
      }

      // * keep intersecting list until we've done it for all query terms
      while (!termListMap.empty()) {
        string nextTerm = get<0>(termListMap.top());
        termListMap.pop();
        prevIntersected = currIntersected;
        currIntersected.clear();
        long startNextList = get<0>(lexicon[term1]);
        long endNextList = get<1>(lexicon[term1]);

        char nextListByte;
        int nextListDecodedByte = 0;
        long nextListBytesToRead = endNextList - startNextList;
        int nextListBytesRead = 0;

        // * reusing lp1 pointer
        lp1.seekg(startNextList, ios::beg);

        // * grab the initial values for the next list
        lp1.get(nextListByte);

        // * decode the value
        nextListDecodedByte += VBDecodeByte(nextListByte);
        int prevListIdx = 0;

        while (prevListIdx < prevIntersected.size() &&
               nextListBytesRead < nextListBytesToRead) {

          // * if we've found a common document, store the result and advance
          // * both pointers
          if (prevIntersected.at(prevListIdx) == nextListDecodedByte) {
            currIntersected.push_back(nextListDecodedByte);
            // * shift the next list pointer from frequency on the docID
            lp1.get();
            // * grab the next docID in the next list
            lp1.get(nextListByte);
            nextListDecodedByte += VBDecodeByte(nextListByte);
            prevListIdx++;
          }

          // * advance the previous list pointer
          else if (prevIntersected.at(prevListIdx) < nextListDecodedByte)
            prevListIdx++;

          // * advance the next list pointer
          else {
            // * shift the next list pointer from frequency on the docID
            lp1.get();
            // * grab the next docID in the next list
            lp1.get(nextListByte);
            nextListDecodedByte += VBDecodeByte(nextListByte);
          }
        }
      }
    // * close list streams
    lp1.close();
    lp2.close();
    
    if (currIntersected.empty())
      cout << "There Aren't Any Great Matches for Your Search" << endl;
    
    return currIntersected;
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


