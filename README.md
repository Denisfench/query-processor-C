**Project structure**



**High Level Design** 

The query processor application consists multiple programs written in 
mixture of Java, C++, and bash scripts. Below I'm going to describe the 
high level design of each program as well as technical challenges that I've 
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
current document by incrementing the ```docID``` counter, and collecting the 
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
and  converting all upper-case letters to lower-case letters. 

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

My initial program was working correctly on a small file size but was running 
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
slowed down program. My final approach is to manually trigger the garbage 
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
compressing my data structures for the Assignment 3. 

Sorting such a large file locally turned out to be a major bottleneck.

1. I copied my intermediate postings file to the HPC Green cluster. While 
doing so it's important to be aware of the disc storage quotas. First, I 
tried to upload the file to the ```/home``` directory, but at the very end 
of the upload I got the disk quota exceeded error. Turns out the ```/home``` 
directory has a 50 GB disk space quota. After learning this, I've uploaded the 
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

My second program is written in C++. It takes intermediate postings file 
produced by the previous program as its input and cleans the file by 
removing all non-printing characters from it. It also cleans the file by 
splitting up postings which contain multiple terms separated by a space into 
separate postings. All the cleaning work is done in the ```cleanPostings()``` 
function. After cleaning each posting, the function writes the clean version 
of the posting to the disk. 

**Program 3: building inverted index and a lexicon**

Initially I've written index builder program in Java. However, when 
writing the query processor in C++, I was unable to decompress VarByte 
encoded index file. It seems that there is a certain difference between Java 
and C++ data types that might have caused this problem. After being unable 
to solve this problem for a certain amount certain time, I've decided to 
rewrite my index builder program in C++. Below I'm going to describe its 
design. 

Index builder program is takes intermediate postings file as its input and 
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



**Future improvements**

The first improvement that I'm planning to incorporate in the Assignment 3 is data compression. I'm planning to compress all three structures: inverted index, lexicon, and URLs-num_terms mapping using the *Variable Bytes compression* technique. 

I'm also planning to explore data storage in a no_SQL database, such as Mongo DB, rather than my SSD. 
