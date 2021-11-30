#include <iostream>
#include <fstream>
#include <unordered_map>
#include<tuple>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <queue>
#include <set>
#include <cctype>
using namespace std;

// TODO: RUN AGAIN ON THE COMPLETE DATASET
// TODO: IMPLEMENT CASE INSENSITIVE LOOKUPS / MATCHES
const string indexFileName = "Nov_25_test_index.bin";
//const string indexFileName = "index.bin";

const string lexiconFileName = "Nov_25_test_lexicon.txt";
//const string lexiconFileName = "lexicon.txt";

const string docCollectionFileName = "testFile.trec";
//const string docCollectionFileName = "../collection-metadata-generator/web_data.trec";

const string docMapFilename = "test_docMap.txt";
//const string docMapFilename = "docMap.txt";

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
const string CONJUNCTIVE = "C";

// * customer comparator that allows us to order tuples by the second column
// * in a priority queue
struct termLengthComparator {
  bool operator()(tuple<string, int>& t1, tuple<string, int>& t2) {
    return get<1>(t1) > get<1>(t2);
  }
};


struct snippetScoreComparator {
  bool operator()(pair<string, int>& t1, pair<string, int>& t2) {
    return get<1>(t1) <= get<1>(t2);
  }
};


// * pair <int, int> : pair <docId, docRank>
struct docsScoreComparator {
  bool operator()(pair<int, int>& t1, pair<int, int>& t2) {
    return get<1>(t1) <= get<1>(t2);
  }
};

// * <term : <indexStartOffset, indexEndOffset, collectionFreqCount> >
unordered_map <string, tuple<int, int, int>> lexicon;

// * <docID : <URL, termCount, webDataStartOffset, webDataEndOffset> >
unordered_map <int, tuple<string, int, long, long>> docMap;

priority_queue<pair <int, int>, vector<pair<int, int>>, docsScoreComparator>
    topNResults;
template <typename T>
void printVec(T vec);
void printLexicon();

// index is the binary file
ifstream indexReader(indexFileName, ios::binary);
ifstream docCollectionStream(docCollectionFileName);

// declare the function prototypes
void loadLexicon();
vector<char> intersectLists(string term);
vector<int> VBDecodeVec(const vector<char>& encodedData);
vector<int> VBDecodeFile(string filename);
int getDocLength(int docID);
vector<int> processDisjunctive(const vector<string>& query);
vector<int> processConjunctive(const vector<string>& queryTerms);
int VBDecodeByte(const char& byte);

pair<string, vector<string>> getUserInput();
int getTermDocFreq(const string& term, int docID);
int getTermColFreq(const string& term);
int rankDoc(const string& term, int docID);
vector<int> getTermDocsDiff(const string& term);
vector<string> generateSnippet(int docID, const vector<string>& query);
void loadDocMap();
vector<char> getDocText(int docID);
vector<string> breakDocIntoSentences(int docID);
int rankSnippet(const string& sentence, const vector<string>& query);
vector<pair<int, int>> rankDocs(const vector<string>& query, const
                                vector<int>& docIds);
void showTopNResults(const vector<string>& query, const vector<pair<int,
                                                                    int>>&
                                                      docs, int numResultsToShow);

string toLowerCase(string str);
    // TODO: conjunctive AND ; disjunctive OR -> you've the function names
//  backwards
int main() {
    cout << "Starting the execution..." << endl;
    auto start = chrono::high_resolution_clock::now();

    // error check the streams
    // TODO: check other streams
    if (!indexReader.is_open()) {
        cerr << "Error opening the index file " << endl;
        exit(1);
    }

    // load the lexicon structure into memory
    cout << "Please wait while we are starting our search engine..." << endl;

    loadLexicon();
    loadDocMap();

    pair<string, vector<string>> userInput;
    vector<int> docsFound;
    userInput = getUserInput();
    while (true) {
      if (get<1>(userInput).size() == 1 && get<1>(userInput).at(0) == quit)
        break;

      // * collect the documents based on the user input
      cout << "Fetching the documents..." << endl;
      if (get<0>(userInput) == CONJUNCTIVE)
        docsFound = processDisjunctive(get<1>(userInput));
      else
        docsFound = processConjunctive(get<1>(userInput));

      cout << "\n\n\n Result documents are: " << endl;
      printVec(docsFound);

      // * ranking the documents
      cout << "\n Ranking the documents..." << endl;
      vector<pair<int, int>> rankedDocs = rankDocs(get<1>(userInput),
          docsFound);

      // * displaying the result
      cout << "Displaying the result..." << endl;
      cout << "The user input is " << get<1>(userInput).size() << endl;
      showTopNResults(get<1>(userInput), rankedDocs, 10);

      // * ask for user input again
      cout << "\n\n\n" << endl;
      userInput = getUserInput();
    }

    // close the streams
    indexReader.close();
    docCollectionStream.close();
    cout << "Producing execution time report..." << endl;
    auto stop = chrono::high_resolution_clock::now();
    auto duration = duration_cast <chrono::milliseconds>(stop - start);
    cout << "The execution time of the program is " << duration.count() << endl;
    return 0;
}


pair<string, vector<string>> getUserInput() {
    vector<string> queryTermVec;
    string query;
    cout << "Please enter your query or \"Q\" to stop the engine: ";
    getline(cin, query, newline);
    string mode;
    cout << "Would you like to run conjunctive or disjunctive query? [C] or "
            "[D]? ";
    getline(cin, mode, newline);
    // * parse the user query
    string term;
    stringstream queryStream(query);
    while (queryStream >> term)
      queryTermVec.push_back(term);

    return make_pair(mode, queryTermVec);
}


// * <docID : <URL, termCount, webDataStartOffset, webDataEndOffset> >
int getDocLength(int docID) {
  if (docMap.find(docID) == docMap.end()) {
    cout << "docID wasn't found in the document map" << endl;
    return -1;
  }
  return get<1>(docMap[docID]);
}

// TODO: display this score along with the user query
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


vector<pair<int, int>> rankDocs(const vector<string>& query, const
                                vector<int>& docIds) {
  vector<pair<int, int>> result;
  if (docIds.empty()) return result;
  int currDocScore = 0;
  // * rank every document that we've found
  for (int docId : docIds) {
    // * rank each document on every term in the query
    for (const string& term : query) {
      currDocScore += rankDoc(term, docId);
    }
    result.emplace_back(make_pair(docId, currDocScore));
  }
  return result;
}


int getTermDocFreq(const string& term, int docID) {
    if (lexicon.find(term) == lexicon.end()) {
        cout << "There Aren't Any Great Matches for Your Search 1" << endl;
        return - 1;
    }

    long startList = get<0>(lexicon.at(term));
    long endList = get<1>(lexicon.at(term));

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
        cout << "There Aren't Any Great Matches for Your Search 2" << endl;
        return decodedList;
    }

    long startList = get<0>(termData);
    long endList = get<1>(termData);

    char nextByte;
    long numBytesToRead = endList - startList;
    int count = 0;
    cout << "getTermDocsDiff()" << endl;
    cout << "start List" << startList << endl;
    cout << "end List" << endList << endl;
    cout << "numBytesToRead" << numBytesToRead << endl;
    indexReader.seekg(startList, ios::beg);

    while (count < numBytesToRead) {
        indexReader.get(nextByte);
        encodedList.push_back(nextByte);
        count += 2;
        // * skip over the term frequency
        indexReader.get();
    }
    cout << "decoding the documents" << endl;
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


template <typename T>
void printVec(T vec) {
    for (auto const &elem : vec)
        cout << elem << comma << space;
}


// TODO: the query should be parsed in user input function
// * a function that takes in a user query as an input and returns all 
// * documents containing the query as an output 
vector<int> processDisjunctive(const vector<string>&query) {
  set<int> result;
  vector<int> vecResult;
  vector<int> docs;
  int currDocId = 0;

  // * the query is empty
  if (query.empty())
    return vecResult;

  for (const string& word : query) {
    docs = getTermDocsDiff(word);
    for (int docIdDiff : docs) {
      cout << "processDisjunctive " << "docIdDiff " << docIdDiff << endl;
      currDocId += docIdDiff;
      cout << "processDisjunctive " << "currDocId " << currDocId << endl;
      result.insert(currDocId);
    }
    currDocId = 0;
  }

  // * copy set into the vector
  vecResult.assign(result.begin(), result.end());

  cout << "processDisjunctive returning the result..." << endl;
  return vecResult;
}


// * a function that takes in a user query as an input and returns all 
// * documents containing every term in the query 
// TODO: the query should be parsed in user input function
vector<int> processConjunctive(const vector<string>& queryTerms) {
    // * termLists is a min heap mapping terms to their inverted list lengths
    // * <term : invertedListLength>
    priority_queue<tuple<string, int>, vector<tuple<string, int>>,
                   termLengthComparator>
        termListMap;

    vector<int> prevIntersected;
    vector<int> currIntersected;

    // * the query is empty
    if (queryTerms.empty())
        return currIntersected;

    // * query has only 1 term
    if (queryTerms.size() == 1) {
        string term = queryTerms.at(0);
        // * return the list of documents containing the term
        return getTermDocsDiff(term);
    }

    // * if the query has 2 or more terms, then
    // * all the docs containing the terms must be intersected

    // * put all the terms in the termListMap min heap
    // * and ensure that we have terms in the lexicon
    for (const string& queryTerm : queryTerms) {
      if (lexicon.find(queryTerm) == lexicon.end()) {
      cout << "There Aren't Any Great Matches for Your Search 3" << endl;
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
      cout << "There Aren't Any Great Matches for Your Search 4" << endl;
    
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

// * docMap.txt description :
// * <docID : <URL, termCount, webDataStartOffset, webDataEndOffset> >
// * http://www.ushistory.org/us/11.asp	3213832 901 23020090760 23020096277
// * unordered_map <int, tuple<string, int, long, long>> docMap;
void loadDocMap() {
  ifstream docMapStream(docMapFilename);
  if (!docMapStream)
    cerr << "Could not open the " << docMapFilename << endl;
  string line;
  int docID;
  string url;
  int termCount;
  long webDataStartOffset;
  long webDataEndOffset;
  // * fill in the docMap hash table line by line
  while (getline(docMapStream, line)) {
    stringstream lineStream(line);
    lineStream >> url;
    lineStream >> docID;
    lineStream >> termCount;
    lineStream >> webDataStartOffset;
    lineStream >> webDataEndOffset;
    docMap.insert(make_pair(docID, make_tuple(url, termCount,
                                              webDataStartOffset, webDataEndOffset)));
  }
  // * close the stream
  docMapStream.close();
}


// * unordered_map <int, tuple<string, int, long, long>> docMap;
string getDocURL(int docID) {
  if (docMap.find(docID) == docMap.end()) {
    cout << "URL of the document with ID " << docID << "couldn't be found" <<
        endl;
    return "";
  }
  return get<0>(docMap[docID]);
}


vector<char> getDocText(int docID) {
  vector<char> result;
  if (docMap.find(docID) == docMap.end()) {
    cout << "URL of the document with ID " << docID << "couldn't be found" <<
        endl;
    return result;
  }
  long webDataStartOffset = get<2>(docMap[docID]);
  long webDataEndOffset = get<3>(docMap[docID]);
  int charsToRead = webDataEndOffset - webDataStartOffset;
  int charsRead = 0;
  char nextChar;
  docCollectionStream.seekg(webDataStartOffset, ios::beg);
  while (charsRead < charsToRead) {
      docCollectionStream.get(nextChar);
      result.push_back(nextChar);
      charsRead++;
  }
  return result;
}


vector<string> breakDocIntoSentences(int docID) {
  set<char> punctuationMarks = {'.', '!', '?'};
  // * get the document text
  vector<char> docText = getDocText(docID);
  // * break the text into sentences
  vector<string> sentences;
  vector<char> currSentence;
  for (char letter : docText) {
      // * if we've found a sentence
      currSentence.push_back(letter);
      if (punctuationMarks.find(letter) != punctuationMarks.end()) {
        string sentence(currSentence.begin(), currSentence.end());
        sentences.push_back(sentence);
        currSentence.clear();
      }
  }
  return sentences;
}


// * c is the number of query terms including repetitions that appear in
// * the sentence
// * d is the number of distinct query terms that appear in the
// * sentence
// * k is the contiguous run of the query terms in a sentence
// * e.g. World War 2
// * rankSnippet() function returns a weighted combination of c, d, and k
int rankSnippet(const string& sentence, const vector<string>& query) {
  // * wc, wd, wk are the corresponding term weights for the terms c, d, and
  // * k respectively
  float wc = 0.25;
  float wd = 0.25;
  float wk = 0.5;
  string queryStr;
  set<string> distinctQueryTerms;
  int c = 0;
  int k = 0;
  uint d;
  int startSearchIdx = 0;
  for (const string& term : query) {
//    cout << "Term " << term << endl;
    queryStr += term + space;
    startSearchIdx = sentence.find(term, 0);
    while (startSearchIdx != string::npos) {
      distinctQueryTerms.insert(term);
      c++;
      startSearchIdx = sentence.find(term, startSearchIdx + 1);
    }
  }
  d = distinctQueryTerms.size();
  if (sentence.find(queryStr, 0) != string::npos)
    k = 1;
  int result = static_cast<int>((wc * c + wd * d + wk * k) * 100);
//  cout << "C : " << c << " D : " << d << " K : " << k << endl;
//  cout << sentence << " SCORE : " << result << endl;
  return result;

}

vector<string> generateSnippet(int docID, const vector<string>& query) {
  vector<string> result;
  auto snippetsToShow = 7;
  priority_queue<pair <string, int>, vector<pair<string, int>>, snippetScoreComparator> snippets;
  vector<string> sentences = breakDocIntoSentences(docID);
  for (const auto& sentence : sentences) {
    snippets.push(make_pair(sentence, rankSnippet(sentence, query)));
  }
  int snippetCount = 0;
  while (!snippets.empty() && snippetCount < snippetsToShow) {
//    cout << get<0>(snippets.top()) << "      :       " << get<1>(snippets.top()) << endl;
    result.push_back(get<0>(snippets.top()));
    snippets.pop();
    snippetCount++;
  }
  return result;
}

// * priority_queue<pair <int, int>> topNResults;
// * pair <int, int> : <docID, docScore>
void showTopNResults(const vector<string>& query, const vector<pair<int,
    int>>& docs, int numResultsToShow) {
  vector<pair<int, int>> docsToShow;
  for (const pair<int, int>& doc : docs)
    topNResults.push(doc);
  int docCount = 0;
  while (!topNResults.empty() && docCount < numResultsToShow) {
    docsToShow.push_back(topNResults.top());
    topNResults.pop();
    docCount++;
  }

  // * displaying result to the user...
  for (const pair<int, int>& doc : docsToShow) {
    cout << "The document ID is " << doc.first << endl;
    cout << endl;
    cout << "The BM25 score is " << doc.second << endl;
    cout << endl;
    cout << "The document URL is " << getDocURL(doc.first) << endl;
    cout << endl;
    vector<string> snippets = generateSnippet(doc.first, query);
    cout << "The document snippet is: " << endl;
    for (const string& subsnippet : snippets)
      cout << subsnippet << "... ";
    cout << endl;
  }
}


string toLowerCase(string str) {
  string result;
  for(auto letter : str)
    result += tolower(letter);
  return result;
}



