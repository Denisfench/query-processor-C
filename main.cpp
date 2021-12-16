#include <iostream>
#include <fstream>
#include <unordered_map>
#include <tuple>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <queue>
#include <set>
#include <cctype>
using namespace std;

// original dataset
//const string indexFileName = "data/dec13_index.bin";
// 10M lines subset of the dataset
const string indexFileName = "data/10M_index.bin";

//const string lexiconFileName = "data/dec13_lexicon.txt";
const string lexiconFileName = "data/10M_lexicon.txt";

//const string docCollectionFileName = "data/web_data.trec";
const string docCollectionFileName = "data/10M_web_data.trec";

//const string docMapFilename = "data/docMap.txt";
const string docMapFilename = "data/10M_docMap.txt";

const string quit = "Q";
const char comma = ',';
const char space = ' ';
const char tab = '\t';
const char newline = '\n';
// * N and dAvg below are being retrieved from the docMetadataFile
// * N is the number of documents in the collection
// * dAvg is the average length of the document
const int N = 108678;
//const int N = 3213834;
const int dAvg = 302;
const float k1 = 2;
const float b = 0.75;
const string conjunctive = "C";
const string disjunctive = "D";

// * custom comparator that allows us to order tuples by the second column
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

// * <term : <(indexStartOffset, indexEndOffset], collectionFreqCount> >
// we are subtracting 1 from indexStartOffset to account for an open range
unordered_map <string, tuple<long, long, int>> lexicon;

// * < docID : <URL, termCount, webDataStartOffset, webDataEndOffset> >
unordered_map <int, tuple<string, int, long, long>> docMap;

priority_queue<pair <int, int>, vector<pair<int, int>>, docsScoreComparator>
    topNResults;

template <typename T>
void printVec(T vec);

// index is the binary file
ifstream indexReader(indexFileName, ios::binary);
ifstream docCollectionStream(docCollectionFileName);

// declare the function prototypes
void loadLexicon();
vector<int> VBDecodeVec(const vector<char>& encodedData);
int getDocLength(int docID);
vector<int> processDisjunctive(const vector<string>& query);
vector<int> processConjunctive(const vector<string>& queryTerms);

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

int main() {

    cout << "Starting the execution..." << endl;
    auto start = chrono::high_resolution_clock::now();

    // error check the streams
    if (!indexReader.is_open()) {
        cerr << "Cannot open " << indexFileName << endl;
        exit(1);
    }

    // load the lexicon into memory
    cout << "Please wait while we are starting our search engine..." << endl;

    cout << "Loading the lexicon..." << endl;
    loadLexicon();

    cout << "Loading the document map..." << endl;
    loadDocMap();

    pair<string, vector<string>> userInput;
    vector<int> docsFound;
    getUserInput: userInput = getUserInput();
    while (true) {
      if (get<1>(userInput).size() == 1 && get<1>(userInput).at(0) == quit)
        goto done;


      // collect the documents based on the user input
      cout << "\n Fetching the documents..." << endl;
      if (get<0>(userInput) == conjunctive) {
        docsFound = processConjunctive(get<1>(userInput));
        if (docsFound.empty() || docsFound.at(0) == -1) {
            cout << "Sorry, we couldn't find any good matches for your search." << endl;
            goto getUserInput;
        }
      }


      else if (get<0>(userInput) == disjunctive) {
          docsFound = processDisjunctive(get<1>(userInput));
          if (docsFound.empty() || docsFound.at(0) == -1) {
              cout << "Sorry, we couldn't find any good matches for your search." << endl;
              goto getUserInput;
          }
      }

      // the input is invalid
      else {
          cout << "Your menu selection is invalid. Please try again." << endl;
          goto getUserInput;
      }

      cout << "\n\n Result documents are: " << endl;
      printVec(docsFound);

      cout << "\n\n **********";

      // ranking the documents
      // * rankDocs() : <docID : BM25 score>
      cout << "\n\n Ranking the documents..." << endl;
      vector<pair<int, int>> rankedDocs = rankDocs(get<1>(userInput),
          docsFound);

      // displaying the result
      cout << "\n\n Displaying the result..." << endl;
      showTopNResults(get<1>(userInput), rankedDocs, 10);
      cout << endl;

      cout << "\n\n Preparing the engine for the next run..." << endl;
      topNResults = priority_queue<pair <int, int>, vector<pair<int, int>>, docsScoreComparator>();
      // * asking the user for input again
      cout << "\n\n\n" << endl;
      userInput = getUserInput();
    }

    // close the streams
    done:
    cout << "\n closing the streams..." << endl;
    indexReader.close();
    docCollectionStream.close();

    cout << "Producing execution time report..." << endl;
    auto stop = chrono::high_resolution_clock::now();
    auto duration = duration_cast <chrono::milliseconds>(stop - start);
    cout << "The execution time of the program is " << duration.count() << endl;
    return 0;
}


pair<string, vector<string>> getUserInput() {
    indexReader.clear();
    docCollectionStream.clear();
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


// * docMap : <docID : <URL, termCount, webDataStartOffset, webDataEndOffset> >
int getDocLength(int docID) {
  if (docMap.find(docID) == docMap.end()) {
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

    // d being - 1 implies that we weren't able to retrieve the document length
    if (d == - 1) return -1;

    K = k1 * ((1 - b) + b * d / dAvg);
    fDt = log((N - fT + 0.5) / (fT + 0.5)) * ((k1 + 1) * fDt / (K + fDt));

    // if the document rank is negative, set it to 0
    if (fDt < 0) fDt = 0;

    return fDt;
}


// * <docID : BM25 score>
vector<pair<int, int>> rankDocs(const vector<string>& query, const
                                vector<int>& docIds) {

  vector<pair<int, int>> result;
  if (docIds.empty()) return result;
  int currDocScore = 0;
  int docRank;

  // rank every document that we've found
  for (int docId : docIds) {

      // if the docID is invalid, skip it
      if (docId < 0 || docId > N) continue;

    // rank each document on every term in the query
    for (const string& term : query) {
      docRank = rankDoc(term, docId);
      if (docRank ==  - 1) continue;
      currDocScore += docRank;
    }
    result.emplace_back(make_pair(docId, currDocScore));
    currDocScore = 0;
  }

  return result;
}


int getTermDocFreq(const string& term, int docID) {

    if (lexicon.find(term) == lexicon.end()) {
        cout << "getTermDocFreq(): There Aren't Any Great Matches for Your Search..." << endl;
        return - 1;
    }

    long startList = get<0>(lexicon.at(term));
    long endList = get<1>(lexicon.at(term));


    long numBytesToRead = endList - startList;
    int currDocId = 0;
    indexReader.seekg(startList, ios::beg);


    char * buffer = new char [numBytesToRead];
    indexReader.read(buffer, numBytesToRead);

    vector<char> encodedInvertedList(buffer, buffer + numBytesToRead);

    vector<int> decodedInvertedList = VBDecodeVec(encodedInvertedList);

    if (decodedInvertedList.empty()) return - 1;

    for (int i = 0; i < decodedInvertedList.size(); i += 2) {
      currDocId += decodedInvertedList.at(i);
      if (currDocId == docID)
        return decodedInvertedList.at(i + 1);
    }

    return - 1;
}


// * lexicon definition
// * <term : <indexStartOffset, indexEndOffset, collectionFreqCount> >
int getTermColFreq(const string& term) {
    if (lexicon.find(term) == lexicon.end()) {
      cout << "getTermColFreq(): We couldn't find the query term " << term << " in our lexicon"
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


void loadLexicon() {

    ifstream lexiconStream(lexiconFileName);
    if (!lexiconStream)
        cerr << "Could not open " << lexiconFileName << endl;

    string line;
    string term;
    long startList;
    long endList;
    int numTerms;

    while (getline(lexiconStream, line)) {
        stringstream lineStream(line);
        getline (lineStream, term, '\t');
        lineStream >> startList;
        lineStream >> endList;
        lineStream >> numTerms;
        lexicon.insert(make_pair(term, make_tuple(startList, endList, numTerms)));
    }
    lexiconStream.close();
}


// index <docID : freq>
vector<int> getTermDocsDiff(const string& term) {
    vector<int> decodedDocList;
    tuple<int, int, int> termData;

    // * retrieve the term data from the lexicon if it is present
    if (lexicon.find(term) != lexicon.end()) {
        termData = lexicon.at(term);
    }

    else {
        cout << "getTermDocsDiff(): There Aren't Any Great Matches for Your Search " << endl;
        return decodedDocList;
    }

    long startList = get<0>(termData);
    long endList = get<1>(termData);

    long numBytesToRead = endList - startList;

    indexReader.seekg(startList, ios::beg);
    char * buffer = new char [numBytesToRead];
    indexReader.read(buffer, numBytesToRead);

    vector<char> encodedInvertedList(buffer, buffer + numBytesToRead);

    if (encodedInvertedList.empty()) return decodedDocList;

    vector<int> decodedInvertedList = VBDecodeVec(encodedInvertedList);

    for (int i = 0; i < decodedInvertedList.size(); i += 2) {
        decodedDocList.push_back(decodedInvertedList.at(i));
    }

   return decodedDocList;
}


template <typename T>
void printVec(T vec) {
    for (auto const &elem : vec)
        cout << elem << comma << space;
}


// a function that takes in a user query as an input and returns all
// documents containing the query as an output
vector<int> processDisjunctive (const vector<string>& query) {

  set<int> result;
  vector<int> vecResult;
  vector<int> docs;
  int currDocId = 0;

  // the query is empty
  if (query.empty())
    return vecResult;

  for (const string& word : query) {
    docs = getTermDocsDiff(word);
      for (int docIdDiff : docs) {
      currDocId += docIdDiff;
      result.insert(currDocId);
    }
    currDocId = 0;
  }

  // copy set into the vector
  vecResult.assign(result.begin(), result.end());

  return vecResult;
}


vector<int> getDocsFromDocDiffs(const vector<int>& docDiffs) {
  vector<int> result;
  if (docDiffs.empty()) return vector<int> { - 1 };
  int currDocId = 0;
  for (int docIdDiff : docDiffs) {
    currDocId += docIdDiff;
    result.push_back(currDocId);
  }
  return result;
}


// a function that takes in a user query as an input and returns all
// documents containing every term in the query
vector<int> processConjunctive(const vector<string>& queryTerms) {

    // termLists is a min heap mapping terms to their inverted list lengths
    // <term : invertedListLength>
    priority_queue<tuple<string, int>, vector<tuple<string, int>>,
            termLengthComparator>
            termListMap;

    vector<int> prevInterection;
    vector<int> currIntersection;

    // the query is empty
    if (queryTerms.empty())
        return currIntersection;

    // the query has only 1 term
    if (queryTerms.size() == 1) {
        string term = queryTerms.at(0);
        return getDocsFromDocDiffs(getTermDocsDiff(term));
    }

    // if the query has 2 or more terms, then
    // all the docs containing the terms must be intersected

    // put all the terms in the termListMap min heap
    // and ensure that we have terms in the lexicon
    for (const string &queryTerm: queryTerms) {
        if (lexicon.find(queryTerm) == lexicon.end()) {
            cout << "processConjunctive(): There Aren't Any Great Matches for Your Search" << endl;
            return currIntersection;
        }
        termListMap.push(make_pair(queryTerm, get<2>(lexicon[queryTerm])));
    }

    string term1 = get<0>(termListMap.top());
    termListMap.pop();
    string term2 = get<0>(termListMap.top());
    termListMap.pop();

    vector<int> listOneDocs = getDocsFromDocDiffs(getTermDocsDiff(term1));
    vector<int> listTwoDocs = getDocsFromDocDiffs(getTermDocsDiff(term2));

    int firstListPtr = 0;
    int secListPtr = 0;


    while (firstListPtr < listOneDocs.size() &&
           secListPtr < listTwoDocs.size()) {

        // case 1: we've found a common document, store the result and move both pointers forward
        // skipping over the term frequencies
        if (listOneDocs.at(firstListPtr) == listTwoDocs.at(secListPtr)) {
            // store the result
            currIntersection.push_back(listOneDocs.at(firstListPtr));
            firstListPtr++;
            secListPtr++;
        }

        // case 2: lp1's docID is smaller than lp2's docID
        // move the firstListPtr pointer forward until >= docID is found skipping over the frequencies
        else if (listOneDocs.at(firstListPtr) < listTwoDocs.at(secListPtr))
            firstListPtr++;

        // case 3: lp1's docID is greater than lp2's docID
        // move the secListPtr pointer forward until >= docID is found skipping over the frequencies
        else secListPtr++;
    }

    // queue being empty means we had only 2 terms to begin with, and we are
    // done
    if (termListMap.empty()) return currIntersection;

    // otherwise, keep intersecting the lists until the heap is empty
    while (!termListMap.empty()) {
        string nextTerm = get<0>(termListMap.top());
        termListMap.pop();
        prevInterection = currIntersection;
        currIntersection.clear();

        vector<int> decodedNextInvertedList = getDocsFromDocDiffs(getTermDocsDiff(nextTerm));
        int prevIntersectionIdx = 0;
        int nextListIdx = 0;

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

        if (currIntersection.empty())
            cout << "processConjunctive(): There Aren't Any Great Matches for Your Search" << endl;

    }
    return currIntersection;
}


// * docMap.txt unordered_map descriptions :
// * <docID : <URL, termCount, webDataStartOffset, webDataEndOffset> >
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
    cout << "getDocURL() URL of the document with ID " << docID << " couldn't be found" <<
        endl;
    return "";
  }
  return get<0>(docMap[docID]);
}


vector<char> getDocText(int docID) {
  vector<char> result;
  if (docMap.find(docID) == docMap.end()) {
    cout << "getDocText() URL (key) of the document with ID " << docID << "couldn't be "
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
  // get the document text
  vector<char> docText = getDocText(docID);
  // break the text into sentences
  vector<string> sentences;
  vector<char> currSentence;
  for (char letter : docText) {
      // if we've found a sentence
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
// the sentence
// * d is the number of distinct query terms that appear in the
// sentence
// * k is the contiguous run of the query terms in a sentence
// e.g. European Union
// * rankSnippet() function returns a weighted combination of c, d, and k
int rankSnippet(const string& sentence, const vector<string>& query) {

  // stdSentence is the standardized sentence, i.g. all letters are lower-cased
  string stdSentence = toLowerCase(sentence);

  // * wc, wd, wk are the corresponding term weights for the terms c, d, and
  // k respectively
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
    cout << "\n The document ID is " << doc.first << endl;
    cout << endl;
    cout << "\n The BM25 score is " << doc.second << endl;
    cout << endl;
    cout << "\n The document URL is " << getDocURL(doc.first) << endl;
    cout << endl;
    vector<string> snippets = generateSnippet(doc.first, query);
    cout << "\n The document snippet is: " << endl;
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



