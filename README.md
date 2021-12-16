**Project structure**



**High-Level Design**

The query processor application consists of multiple programs written in
a mixture of Java, C++, and bash scripts. Below I'm going to describe the
high-level design of each program as well as technical challenges that I've
faced and solutions that I've developed.

**Program 1: web data file parser.**

**Part 1: reading the data**

I'm reading web documents from the input file, ```web_data.
trec```, in chunks of about 2 GB each. Initially, I'm reading the 2 GB
chunks of the file into the byte array.

Throughout the program, I'm using various java classes within the ```java.io```
package. For reading the data from the source file, I've decided to use
```RandomAccessFile``` class. Even though this class was designed primarily
for random disk accesses and our application is primarily reading the input
file sequentially, we need random disk accesses because we are reading the
input file in chunks. Every time we need to read a new chunk of data, we have
to start reading from a particular offset in the file, the offset where we
stopped reading the previous chunk. ```RandomAccessFile``` class provides
methods such as ```long getFilePointer()```, which returns the current position
of the file pointer that is being incremented every time we read or write a
byte of data, and ```void seek()```, which allows setting the file pointer to a
particular location within the file.

I’m reading a chunk of the file into the array of bytes. I’m also storing the
file offset so that after the data in the byte array has been processed,
we can keep reading a new chunk of the file into the array from where we’ve left off.

**Web data file parser: Version 2**

I have rewritten my Java parser from HW 2 in C++. The code ended up being
much simpler and more time / memory efficient than the previous
implementation. My new implementation is in ```web_data_processor``` file.
The new web data parser program consists of three main parts implemented in the
following functions: ```generateFilePostings()```, ```writePosting()``` and
```isValidTerm()```.
```generateFilePostings()``` traverses the file document by document,
extracting the terms from the document and calling ```writePosting()```
function passing it each term and docID.
```writePosting()``` function calls ```isValidTerm()``` function on each
term to verify that the term is valid before writing it to the
```postings.txt``` file. If the term is not valid, e.g. it contains
non-alphabetic, non-numeric, or non-punctuation characters, it will be
discarded.

**Part 2: parsing the data**

For parsing the file, I've tried using standard XML parsing libraries, however,
those libraries were throwing exceptions when attempting to parse certain
characters present in the input data.

Also, I've noticed that the input file has a very simple structure, and we don't
require most of the functionality that is provided by the XML parsing libraries.

Based on this I've decided to implement my own parser for this project.
The parsing logic is implemented in   ```parseData()``` and ```indexOf()```
functions. When parsing a file, my goal is to find a position of the opening
```<TEXT>``` and a closing ```</TEXT>``` tags, assign the document ID to the
the current document by incrementing the ```docID``` counter, and collecting the
terms from the document.

When encountering an edge case of the document being truncated, I discard
that document. This may happen when we are switching from reading one chunk
of the input file to another. There's a way to handle this case without
discarding the document by buffering the truncated part of the document and
concatenating it to the remaining part of that document in the next chunk.
However, given the number of documents we have, discarding approximately 12
documents in the worst case will not affect the overall performance.

During the parsing stage, I'm also extracting URLs from web pages and storing
them as a list of tuples, where the first value in each tuple is the URL and a
second value is a number of terms in that document. The ```urls``` list has the
following signature: ```List <Pair> urls = new ArrayList<>()```

**Part 3: writing intermediate postings to the file**

I'm writing intermediate postings to the file by calling the
```writePostings()``` function. This function receives a tuple
```Pair<String, Integer> posting``` from the ```parseData()``` function.
Then, I'm retrieving a ```String term``` and a ```String docID``` from the tuple.

After getting the term and the document ID of the document containing the term,
I'm parsing the term by removing any trailing whitespaces, punctuation marks,
and converting all upper-case letters to lower-case letters.

After parsing, I'm writing the data to the ```intermediate_postings.txt``` file
in the following format:

```term \tab docID \newline```

To take care of the non-ASCII characters, when writing the term and the docID
to the file, I'm first converting them to byte streams by calling
the ```getBytes()``` method of the ```java String``` module and them
passing ```StandardCharsets.UTF_8``` option to this method.

**Part 5: miscellaneous**

After writing postings and URLs to the file, I'm flushing the buffers, in case
there's some leftover data in them, and closing the files with streams by
calling the ```close()``` method of the ```java io``` package.

I'm also timing the program by recording the time at the beginning of
the ```main``` using ```System.currentTimeMillis()``` method and recording
the time at the end of ```main``` using the same method. By subtracting the
start time from the end time, I'm able to calculate the amount of time it takes
for the first program to execute.

**Key challenges and solutions**

My initial program was working correctly on small file size but was running
out of heap space when attempting to parse the original 23 GB file.
In this section I'd like, to discuss the cause of this problem and my
approach to solving it.

Using the original program, my plan was to read the original file in chunks of
2 GB (the current chunk size). However, after running for a while, the program
gave me an ```OutOfMemoryError```. I tried using chunk sizes of 2 GB,
1 GB, 500 MB, and 250 MB. In all cases, I was getting the same error,
even though with smaller chunk sizes the program was able to run successfully
for a longer period of time.

Initially, I was trying to increase the JVM heap space. However, I was able to
identify the cause of the problem was inefficient memory management and memory
leaks rather than the lack of heap space.

I solved the memory problem in the following way:

1. I simplified my program by removing various complex data structures from it,
   such as redundant hash tables and hash sets.

2. I've made my code more modular. Firstly, this improved the code readability.
   Secondly, the garbage collector is likelier to be triggered after the function
   execution complies. Thus, it doesn't have to wait for too long for a large
   function to terminate before clearing the heap space.

3. I'm manually triggering the garbage collector to clear certain temporarily
   allocated data structures.  For example, in the ```parseData()``` method,
   I'm collecting the terms and a URL by creating two ```StringBuilder```
   objects, which are mutable versions of ```String``` object in java.
   After parsing the document, I no longer need these data structures and
   I want them to be cleared from the heap before moving on to processing
   the next document. To achieve this I'm manually triggering the garbage
   collector by calling ```System.gc()```.

4. I'm also writing URL-page size mapping to the disk after processing each
   chunk of the data and clearing the urls array before moving on to the next chunk.

Such optimizations solved the ```OutOfMemoryError``` and greatly improved
the performance. The program now runs about 15 times faster than before.
Below I'm including the exact statistics for the provided dataset.

I've also experimented with triggering the garbage collector after processing
each document. However, this led to diminishing returns and significantly
slowed down the program. My final approach is to manually trigger the garbage
collector after processing each chunk of the file.

**Execution statistics**

The 23 GB ```web_data.trec``` file was processed in 13 chucks of about 2 GB.
(The last chunk is smaller than the rest). My chucks are actually a little
smaller than 2 GB because of the ```byte[]``` size limitations.

**The program execution took 6085670 milliseconds ≈ 6085 seconds.**

**Sorting program**

For sorting the intermediate postings file I've written a bash script that is
using Unix sort to sort the ```intermediate_postings.txt``` file based on the
term and a document ID.

**Key challenges and solutions**

When applied to the intermediate postings file, the Unix sort wasn't as fast
as I expected. I discovered that there's a way to run parallel Unix sort on a
dataset with multiple threads. I've rewritten my bash script to use this
option and ended up sorting the file on NYU High Computing Cluster.

**Execution statistics**

My intermediate postings file is about 50 GB in size. I'll be working on
compressing my data structures for Assignment 3.

Sorting such a large file locally turned out to be a major bottleneck.

1. I copied my intermediate postings file to the HPC Green cluster. While
   doing so it's important to be aware of the disc storage quotas. First, I
   tried to upload the file to the ```/home``` directory, but at the very end
   of the upload, I got the disk quota exceeded error. Turns out the ```/home```
   the directory has a 50 GB disk space quota. After learning this, I've uploaded the
   file to the ```/scratch``` directory with the 5 TB quota.

2. I've written a specialized bash script for the cluster which is doing
3. sorting in parallel using 30 CPUs and 50 GB of memory.

Here I'm attaching my HPC job log:

[df1911@log-3 df1911]$ seff 11157805
Job ID: 11157805
Cluster: greene
User/Group: df1911/df1911
State: COMPLETED (exit code 0)
Nodes: 1
Cores per node: 30
CPU Utilized: 13:59:11
CPU Efficiency: 13.70% of 4-06:05:30 core-walltime
Job Wall-clock time: 03:24:11
Memory Utilized: 50.00 GB
Memory Efficiency: 78.13% of 64.00 GB


**Program 2: cleaning intermediate postings file**

My second program is written in C++. It takes the intermediate postings file
produced by the previous program as its input and cleans the file by
removing all non-printing characters from it. It also cleans the file by
splitting up postings that contain multiple terms separated by a space into
separate postings. All the cleaning work is done in the ```cleanPostings()```
function. After cleaning each posting, the function writes the clean version
of the posting to the disk.

**Program 3: building inverted index and a lexicon**

Initially, I've written an index builder program in Java. However, when
writing the query processor in C++, I was unable to decompress the VarByte
encoded index file. It seems that there is a certain difference between Java
and C++ data types that might have caused this problem. After being unable
to solve this problem for a certain amount certain time, I've decided to
rewrite my index builder program in C++. Below I'm going to describe its
design.

Index builder program takes intermediate postings file as its input and
produces ```index.bin``` and ```lexicon.txt``` as its outputs.

I'm using ```ifstream``` to read the input file and ```ofstream``` to write
the output files. The core of the program is written in ```aggregatePostings
()``` function. This function calls ```readPosting()``` function to read a
line containing posting from the intermediate postings file. Then ```aggregatePostings
()``` function calculates the number of times the term occurs within each
document and writes this data to the index file using ```writePosting()```
function. This function doesn't actually write a posting to the file but
instead calls ```VBEncode()``` function which VarByte encodes the data and
writes it to ```index.bin``` file. After all the postings have been written
to the index file for a particular term, we insert an entry into the lexicon
in the following format: ```< term : indexStartOffset, indexEndOffset,
termCollectionFrequency >```.

Finally, ```writeLexicon()``` function will write an in memory lexicon,
represented as an unordered map, to the ```index.txt``` file.


**Program 4: query generator**

**Loading the data:**
The query generator program takes ```index.bin``` [size: ], ```lexicon.
txt``` [size: ], ```web_data.trec``` [size: ], and ```docMap.txt``` [size: ]
files as inputs. I'm creating ```ifstreams``` pointing to each of these
files. However, only ```lexicon.txt``` and ```docMap.txt``` are getting
loaded into memory. I will describe the structure of each of these files below.

The purpose of the lexicon is to give us access to inverted lists within the
index file, as well as to provide us the term frequency within the
collection to be used by ```BM25``` ranking function. The ```lexicon.txt```
file has the following structure: ```[term : indexStartOffset,
indexEndOffset, termCollectionFrequency]```. Index gets loaded into memory using
```loadLexicon()``` function. An in-memory version of the lexicon is represented
as an unordered map with the term being the key and other fields as values.

The second data structure that we are loading into memory is the ```docMap.
txt```. The purpose of the docMap file is to provide us with a URL, document
size, as well as the document location within the collection. ```docMap.
txt``` has the following structure: ```[docID : URL, termCount,
webDataStartOffset, webDataEndOffset]```. The in-memory version of the docMap is
represented as an unordered map with docID being the key and other fields as values.

**Getting the user input**
The first part of the query generator is ```getUserInput()``` function that
asks the user to choose the search mode, conjunctive vs disjunctive, and
enter the search query. The query string is then split up into separate terms
based on spaces and returned as a vector of strings.

**Retrieving relevant documents**
After the user input is being obtained from the user, based on the specified
search mode, we perform a conjunctive or disjunctive search by calling
```processConjunctive()``` and ```processDisjunctive()``` functions
respectively. If the user has entered an invalid search query, I'm printing
an error message and looping back to the ```getUserInput()``` function.

```processConjunctive()``` function takes a vector of strings, representing a
query, as input and returns a vector of integers with IDs of found documents
as output. The ```processConjunctive()``` function has the following implementation:
First, it creates a ```termListMap```, which is a min-heap mapping term to
its respective inverted list length, ```[term : invertedListLength]```. This
map will sort the query terms in ascending order of their inverted list
lengths.

After the query terms have been sorted, the function branches based on the
number of terms the user has entered. If the user has entered a single
query term, the function calls ```getDocsFromDocDiffs(getTermDocsDiff(term))```
functions that will retrieve and return the documents for a given query term.

If the user has entered 2 or more terms, the function will be loading
inverted lists for the corresponding terms into memory and finding the
document intersection between those lists, one term at a time until the min
heap with the query terms is empty.

The documents for a particular inverted list are being retrieved using
```getDocsFromDocDiffs(getTermDocsDiff(term))``` functions which also
are calling ```VBDecodeVec()``` function which will VarByte decode inverted
lists for the corresponding terms.


```processDisjunctive()``` function is calling ```getDocsFromDocDiffs
(getTermDocsDiff(term))``` functions to get the documents from the inverted
lists for each query term and returning these documents.


**Ranking the documents**
After we've retrieved relevant documents, our next step is to rank them
using ```rankDocs()``` function, which takes user query and a vector of
docIDs as inputs and returns a vector of tuples containing ```[docID :
docRank]``` mapping for each relevant document found.

```rankDocs()``` function is iterating over all docIDs and all query
terms, calling ```rankDoc()``` on each term and a corresponding document and
calculating that document's rank.

```rankDoc()``` function is ranking takes term and docID as parameters and
returns BM25 score for that document. It also uses auxiliary functions such
as ```getTermDocFreq()```, ```getTermColFreq()``` and ```getDocLength()```
to retrieve variables needed for BM25 measure from in-memory data structures.

**Returning result documents with their corresponding snippets**

After ranked documents have been returned by the ```rankDoc()``` function,
we are calling ```showTopNResults()``` on those documents, passing the
vector of docIDs and the number of documents we'd like to show as input parameters.
```showTopNResults()``` function pushes all relevant documents onto the max
heap. Priority of the documents within the max heap is based on BM25 score
of the document. Then I'm simply popping ```numResultsToShow``` documents
from the heap.

The next part of ```showTopNResults()``` function is responsible for
displaying the URL of a document and calling ```generateSnippet()```
function to display a document snippet to the user.

```generateSnippet()``` function is implementing a slightly simplified version
of a snippet generation algorithm presented in the paper *"Fast generation
of result snippets in web search"* by Andrew Turpin. First, we are calling
```breakDocIntoSentences()``` function which takes a document ID as input and
returns a vector of strings containing document sentences. Then it is
ranking each sentence in the document using ```rankSnippet()``` function and
pushing the sentence onto the max heap called ```snippets```. Once each
sentence has been ranked, the function pops the first ```snippetsToShow```
sentences off the heap and returns a vector of strings containing those
sentences. These sentences are getting displayed to the user as a document snippet.
