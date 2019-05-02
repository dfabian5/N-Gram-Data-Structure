////////////////////////////////////////////////////////////////////////////////
//
// FILE:        main.cpp
// DESCRIPTION: uses trie data structure for queries
// AUTHOR:      Dan Fabian and Lauren Greathouse
// DATE:        5/1/2019

#include "trie.h"
#include "vocab.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <chrono>

using std::cout; using std::cin; using std::getline;
using std::ifstream;

// used for timing queries
typedef std::chrono::high_resolution_clock Clock; 

int main(int argc, char *argv[])
{
  if (argc < 3)
    {
      cout << "Need data input file and length of grams\n"
	   << "example ./a.out file.txt 5\n";
      return 1;
    }

  ifstream inFile(argv[1]);
  if (!inFile)
    {
      cout << "Could not open file, exiting\n";
      return 1;
    }

  const int gramSize = stoi(argv[2]);

  // create vocab, needed to construct the trie
  Vocab v(inFile, gramSize);

  // setup vocab
  Encoder::vocabID2S_ = &v.vocabID2S;
  Encoder::vocabS2ID_ = &v.vocabS2ID;
  
  // since vocab moves through the file we need to 
  // start back at the beginning
  inFile.clear();
  inFile.seekg(0, std::ios::beg);

  // main output
  cout << "Enter a K value: ";
  int k; cin >> k;
  Trie t(inFile, gramSize, k);

  // show size of data structure
  cout << "Size of trie in bytes: " << SIZE_TRACKER << "\n";

  // get input
  cout << "Choose a query:\n0. Most Likely Next\n1. Frequency Count\n\n";
  int querySelection; cin >> querySelection;
  
  int toReturn; 
  vector<string> finalInput; 
  string input;
  if (querySelection == 0)
    {
      cout << "Enter how many results to return: ";
      cin >> toReturn;
    }

  // get gram to query
  cout << "Enter a phrase, hit enter after each word and when done type 'e': ";
  cin >> input;
  while (input != "e")
    {
      finalInput.push_back(input);
      cin >> input;
    }

  vector<string> result;
  while (toReturn != 0)
    {
      
      // queries
      if (querySelection == 0)
	{
	  auto t1 = Clock::now();
	  t.mostLikelyNext(finalInput, toReturn);
	  auto t2 = Clock::now();

	  result = t.mostLikelyNext(finalInput, toReturn);

	  // print time for query
	  cout << "Query took: "
	       << std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count()
	       << " nanoseconds\n" << "or " 
	       << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count()
               << " microseconds\n";

	  // print result
	  for (int i = 0; i != result.size(); ++i)
	    cout << i <<  ". " << result[i] << '\n';
	  cout << '\n';
	}
      else
	{
	  auto t1 = Clock::now();
	  t.frequencyCount(finalInput);
	  auto t2 = Clock::now();
	  
	  size_t count = t.frequencyCount(finalInput);
	  
	  // print time for query
          cout << "Query took: "
               << std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count()
               << " nanoseconds\n" << "or " 
	       << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count()
	       << " microseconds\n";
	  
	  cout << "Occurs " << count << " times\n\n";
	}
      
      finalInput.clear();
      
      // get next input
      cout << "Choose a query:\n0. Most Likely Next\n1. Frequency Count\n\n";
      cin >> querySelection;
      
      // selection query
      if (querySelection == 0)
	{
	  cout << "Enter how many results to return, when done enter '0': ";
	  cin >> toReturn;
	}
      
      cout << "Enter a phrase, hit enter after each word "
	   << "and when done type 'e': ";
      cin >> input;
      while (input != "e")
	{
	  finalInput.push_back(input);
	  cin >> input;
	}
    }

  delete Encoder::vocabS2ID_;
  delete Encoder::vocabID2S_;
}
