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
const string indexFileName = "data/Nov_25_test_index.bin";
//const string indexFileName = "data/index.bin";

const string lexiconFileName = "data/Nov_25_test_lexicon.txt";
//const string lexiconFileName = "data/lexicon.txt";

const string docCollectionFileName = "data/testFile.trec";
//const string docCollectionFileName = "data/web_data.trec";

const string docMapFilename = "data/test_docMap.txt";
//const string docMapFilename = "data/docMap.txt";

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
const string conjunctive = "C";
const string disjunctive = "D";

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
vector<int> getDocsFromDocDiffs(const vector<int>& docDiffs);
vector<char> read_com(ifstream& infile);
int getTermDocFreq(const string& term, int docID);

int main() {

  // ********** VB decode debugging area ***************
  /*
    cout << "decoding a file..." << endl;
//    vector<int> decodedIdx = VBDecodeFile(indexFileName);
    vector<char> vec = read_com(indexReader);
    cout << "printing decoded file..." << endl;
//    printVec(decodedIdx);
    vector<int> decodedIdx;
    for (const char& ch : vec)
        decodedIdx.push_back(VBDecodeByte(ch));
    printVec(decodedIdx);
    */
  loadLexicon();
  loadDocMap();
//  int freq = getTermDocFreq("hitler", 13);
//  cout << "the frequency is " << freq << endl;

//  vector<int> docDiffs = getTermDocsDiff("hitler");
//  printVec(docDiffs);

//  vector<char> docText = getDocText(8);
//  for (char letter : docText)
//    cout << letter;
  // ********** VB decode debugging area ***************

    cout << "Starting the execution..." << endl;
    auto start = chrono::high_resolution_clock::now();

    // error check the streams
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
      if (get<0>(userInput) == conjunctive) {
//        cout << "PROCESSING conjunctive QUERY..." << endl;
        docsFound = processConjunctive(get<1>(userInput));
      }
      else if (get<0>(userInput) == disjunctive)
        docsFound = processDisjunctive(get<1>(userInput));
      // TODO: change this later
      else exit(1);

      cout << "\n\n\n Result documents are: " << endl;
      printVec(docsFound);

      // * ranking the documents
      // // * rankDocs() : <docID : BM25 score>
      cout << "\n Ranking the documents..." << endl;
      vector<pair<int, int>> rankedDocs = rankDocs(get<1>(userInput),
          docsFound);

//      // *********** debugging area ***********
//      for (const pair<int, int>& doc : rankedDocs)
//        cout << "docID " << doc.first << " BM25 score " << doc.second << endl;
//      // *********** debugging area ***********

      // * displaying the result
      cout << "Displaying the result..." << endl;
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
      queryTermVec.push_back(toLowerCase(term));
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
    cout << "rankDoc() requested for the docID " << docID << "  on the term "
       << term << endl;
    int K = 0;
    int docRank = 0;
    int fDt = getTermDocFreq(term, docID);
    cout << "got the term doc freq  " << fDt << endl;
    int fT = getTermColFreq(term);
    cout << "got the term collection freq  " << fT << endl;
    int d = getDocLength(docID);
    cout << "got the document length " << d << endl;
    K = k1 * ((1 - b) + b * d / dAvg);
    fDt = log((N - fDt + 0.5) / (fDt + 0.5)) * ((k1 + 1) * fDt / (K + fDt));
    // * if the document rank is negative, set it to 0
    if (fDt < 0) fDt = 0;
    cout << "rankDoc() result " << fDt;
    return fDt;
}


// * <docID : BM25 score>
vector<pair<int, int>> rankDocs(const vector<string>& query, const
                                vector<int>& docIds) {
  cout << "The input query is " << endl;
  for (const string& qTerm : query)
    cout << qTerm << endl;
  vector<pair<int, int>> result;
  if (docIds.empty()) return result;
  int currDocScore = 0;
  // * rank every document that we've found
  for (int docId : docIds) {
    cout << "docID " << docId << endl;
    // * rank each document on every term in the query
    for (const string& term : query) {
      cout << "\n\n\n term " << term << " Rank : " << rankDoc(term, docId) <<
          endl;
      cout << "\n\n\n";
      currDocScore += rankDoc(term, docId);
    }
    result.emplace_back(make_pair(docId, currDocScore));
    currDocScore = 0;
  }
  cout << "returning result " << endl;
  for (pair<int, int> ranking : result)
    cout << "docId " << ranking.first << "rank " << ranking.second << endl;

  return result;
}

// TODO: the function hangs for some reason
// * Perhaps it's getting suck in decode byte function?
int getTermDocFreq(const string& term, int docID) {
    cout << "getTermDocFreq() term " << term << endl;
    cout << "getTermDocFreq() docID " << docID << endl;
    if (lexicon.find(term) == lexicon.end()) {
        cout << "There Aren't Any Great Matches for Your Search 1" << endl;
        return - 1;
    }

    long startList = get<0>(lexicon.at(term));
    long endList = get<1>(lexicon.at(term));

    cout << "getTermDocFreq() startList " << startList << endl;
    cout << "getTermDocFreq() endList " << endList << endl;
    char nextByte;
    long numBytesToRead = endList - startList;
    int count = 0;
    int currDocId = 0;
    indexReader.seekg(startList, ios::beg);
    cout << "getTermDocFreq() numBytesToRead " << numBytesToRead << endl;
    char * buffer = new char [numBytesToRead];
    indexReader.read(buffer, numBytesToRead);

    vector<char> encodedInvertedList(buffer, buffer + numBytesToRead);

    vector<int> decodedInvertedList = VBDecodeVec(encodedInvertedList);

    for (int i = 0; i < decodedInvertedList.size() - 1; i += 2) {
      currDocId += decodedInvertedList.at(i);
      cout << "entry " << decodedInvertedList.at(i) << endl;
      if (currDocId == docID)
        return decodedInvertedList.at(i + 1);
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


vector<char> read_com(ifstream& infile) {
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


// TODO: It seems like the function in principle might be non-workable
// TODO: the function hangs in case long query is being submitted
// TODO: seems like the problem is with specific query terms
// TODO: you can use VBDecodeVec instead, perhaps you've missed smt. in this
//  one OR you can figure out how it works at the first place
int VBDecodeByte(const char& byteId) {
    cout << "VBDecodeByte()" << endl;
    cout << "byteId " << byteId << endl;
    char c;
    int num;
    int p;
    c = byteId;
    bitset<8> byte(c);
    num = 0;
    p = 0;
    // TODO: this loop is infinite on some of the terms
    while(byte[7] == 1){
        byte.flip(7);
        num += byte.to_ulong()*pow(128, p);
        p++;
        c = byteId;
        byte = bitset<8>(c);
//        cout << "VBDecodeByte() while loop" << endl;
    }
    num = (byte.to_ulong())*pow(128, p);
    cout << "decoded number is " << num << endl;
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
//    cout << "term requested " << term << endl;
    vector<int> decodedDocList;
    tuple<int, int, int> termData;

    // * retrieve the term data from the lexicon if it is present
    if (lexicon.find(term) != lexicon.end()) {
        termData = lexicon.at(term);
    }

    else {
        cout << "There Aren't Any Great Matches for Your Search "
              "getTermDocsDiff()" << endl;
        return decodedDocList;
    }

    long startList = get<0>(termData);
    long endList = get<1>(termData);

    char nextByte;
    long numBytesToRead = endList - startList;
    int count = 0;
//    cout << "getTermDocsDiff()" << endl;
//    cout << "start List" << startList << endl;
//    cout << "end List" << endList << endl;
//    cout << "numBytesToRead" << numBytesToRead << endl;
    indexReader.seekg(startList, ios::beg);
    char * buffer = new char [numBytesToRead];
    indexReader.read(buffer, numBytesToRead);

    vector<char> encodedInvertedList(buffer, buffer + numBytesToRead);

    vector<int> decodedInvertedList = VBDecodeVec(encodedInvertedList);

    for (int i = 0; i < decodedInvertedList.size() - 1; i += 2)
      decodedDocList.push_back(decodedInvertedList.at(i));
//    cout << "decoding the documents" << endl;
   return decodedDocList;
}


long openList(const string& term, ifstream& fileStream) {
    // retrieve the term data from the lexicon
    tuple <int, int, int> termData = lexicon.at(term);
    long startList = get<0>(termData);
    long endList = get<1>(termData);
    fileStream.seekg(startList, ios::beg);
    return endList;
}


// TODO: the function is currently not in use. We are implementing the same
//  logic within processConjunctive and processDisjunctive functions.
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
vector<int> processDisjunctive (const vector<string>& query) {
//  cout << "\n vector<string>& query.size() \n " << query.size() << endl;

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
//      cout << "processDisjunctive " << "docIdDiff " << docIdDiff << endl;
      currDocId += docIdDiff;
//      cout << "processDisjunctive " << "currDocId " << currDocId << endl;
      result.insert(currDocId);
    }
    currDocId = 0;
  }

  // * copy set into the vector
  vecResult.assign(result.begin(), result.end());

//  cout << "processDisjunctive returning the result..." << endl;
  return vecResult;
}


vector<int> getDocsFromDocDiffs(const vector<int>& docDiffs) {
//  cout << " getDocsFromDocDiffs " << endl;
  printVec(docDiffs);
  vector<int> result;
  if (docDiffs.empty()) return docDiffs;
  int currDocId = 0;
  for (int docIdDiff : docDiffs) {
    currDocId += docIdDiff;
    result.push_back(currDocId);
  }
//  cout << " ****** getDocsFromDocDiffs result ***** " << endl;
//  printVec(result);
  return result;
}


// * a function that takes in a user query as an input and returns all 
// * documents containing every term in the query 
vector<int> processConjunctive(const vector<string>& queryTerms) {
    cout << "Processing conjunctive query" << endl;
    // * termLists is a min heap mapping terms to their inverted list lengths
    // * <term : invertedListLength>
    priority_queue<tuple<string, int>, vector<tuple<string, int>>,
            termLengthComparator>
            termListMap;

    vector<int> prevInterection;
    vector<int> currIntersection;

    // * the query is empty
    if (queryTerms.empty())
        return currIntersection;

    // * query has only 1 term
    if (queryTerms.size() == 1) {
        string term = queryTerms.at(0);
        // * return the list of differences for the documents containing the
        // * term
        return getDocsFromDocDiffs(getTermDocsDiff(term));
    }


    // * if the query has 2 or more terms, then
    // * all the docs containing the terms must be intersected

    // * put all the terms in the termListMap min heap
    // * and ensure that we have terms in the lexicon
    for (const string &queryTerm: queryTerms) {
        cout << "queryTerm " << queryTerm << endl;
        if (lexicon.find(queryTerm) == lexicon.end()) {
            cout << "There Aren't Any Great Matches for Your Search 3" << endl;
            return currIntersection;
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
    int firstListPtr = 0;

    char nextByteList2;
    int decodedByteList2 = 0;
    long numBytesToReadList2 = endList2 - startList2;
    int secListPtr = 0;

    // * set both streams to point to their
    // * corresponding lists in the index
    lp1.seekg(startList1, ios::beg);
    lp2.seekg(startList2, ios::beg);

    char *firstLstBuff = new char[numBytesToReadList1];
    char *secLstBuff = new char[numBytesToReadList2];

    // reading 2 inverted lists in blocks
    lp1.read(firstLstBuff, numBytesToReadList1);
    lp2.read(secLstBuff, numBytesToReadList2);

    // convert encoded char arrays to vectors
    vector<char> encodedInvertedList1(firstLstBuff, firstLstBuff + numBytesToReadList1);
    vector<char> encodedInvertedList2(secLstBuff, secLstBuff + numBytesToReadList2);

    // decode both lists
    vector<int> decodedInvertedList1 = VBDecodeVec(encodedInvertedList1);
    vector<int> decodedInvertedList2 = VBDecodeVec(encodedInvertedList2);

    while (firstListPtr < decodedInvertedList1.size() &&
           secListPtr < decodedInvertedList2.size()) {
        cout << "List one docID " << decodedInvertedList1.at(firstListPtr) << endl;
        cout << "List two docID " << decodedInvertedList2.at(secListPtr) << endl;
        // case 1: we've found a common document, store the result and move both pointers forward
        // skipping over the term frequencies
        if (decodedInvertedList1.at(firstListPtr) == decodedInvertedList2.at(secListPtr)) {
            // store the result
            currIntersection.push_back(decodedInvertedList1.at(firstListPtr));
            firstListPtr += 2;
            secListPtr += 2;
        }

            // case 2: lp1's docID is smaller than lp2's docID
            // move the firstListPtr pointer forward until >= docID is found skipping over the frequencies
        else if (decodedInvertedList1.at(firstListPtr) < decodedInvertedList2.at(secListPtr))
            firstListPtr += 2;

            // case 3: lp1's docID is greater than lp2's docID
            // move the secListPtr pointer forward until >= docID is found skipping over the frequencies
        else secListPtr += 2;
    }

    cout << "\n\n\n ***** intersection of the first and second lists *****\n\n\n" << endl;
    printVec(currIntersection);
    // * keep intersecting lists until we've done it for all query terms
    while (!termListMap.empty()) {
        string nextTerm = get<0>(termListMap.top());
        termListMap.pop();
        prevInterection = currIntersection;
        currIntersection.clear();
        long startNextList = get<0>(lexicon[term1]);
        long endNextList = get<1>(lexicon[term1]);

        char nextListByte;
        int nextListDecodedByte = 0;
        long nextListBytesToRead = endNextList - startNextList;
        int nextListIdx = 0;
        int prevIntersectionIdx = 0;

        // * reusing lp1 pointer
        lp1.seekg(startNextList, ios::beg);

        char *nextListBuff = new char[nextListIdx];

        lp1.read(nextListBuff, nextListBytesToRead);

        vector<char> encodedNextInvertedList(nextListBuff, nextListBuff + nextListBytesToRead);

        vector<int> decodedNextInvertedList = VBDecodeVec(encodedNextInvertedList);

        // intersect the previous intersection result with the next list
        while (nextListIdx < decodedNextInvertedList.size() && prevIntersectionIdx < prevInterection.size()) {

            // case 1: we've found a common document
            if (prevInterection.at(prevIntersectionIdx) == decodedNextInvertedList.at(nextListIdx)) {
                // store the result
                currIntersection.push_back(decodedNextInvertedList.at(nextListIdx));
                prevIntersectionIdx += 2;
                nextListIdx += 2;
            }

                // previous intersected list pointer is pointing at a smaller docID
            else if (prevInterection.at(prevIntersectionIdx) < decodedNextInvertedList.at(nextListIdx))
                prevIntersectionIdx += 2;

            else nextListIdx += 2;

        }

        // * close list streams
        lp1.close();
        lp2.close();

        if (currIntersection.empty())
            cout << "There Aren't Any Great Matches for Your Search 4" << endl;

        return currIntersection;
    }
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
    cout << "URL (key) of the document with ID " << docID << "couldn't be "
                                                           "found" <<
        endl;
    return result;
  }
  long webDataStartOffset = get<2>(docMap[docID]);
  long webDataEndOffset = get<3>(docMap[docID]);
  int charsToRead = webDataEndOffset - webDataStartOffset;
  int charsRead = 0;
  char nextChar;
  docCollectionStream.seekg(webDataStartOffset, ios::beg);

  char * buffer = new char [charsToRead];
  docCollectionStream.read(buffer, charsToRead);

  vector<char> text(buffer, buffer + charsToRead);

  return text;
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
  // stdSentence is the standardized sentence, i.g. all letters are lower-cased
  string stdSentence = toLowerCase(sentence);
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
    startSearchIdx = stdSentence.find(term, 0);
    while (startSearchIdx != string::npos) {
      distinctQueryTerms.insert(term);
      c++;
      startSearchIdx = stdSentence.find(term, startSearchIdx + 1);
    }
  }
  d = distinctQueryTerms.size();
  if (stdSentence.find(queryStr, 0) != string::npos)
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



