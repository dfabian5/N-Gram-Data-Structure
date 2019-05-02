#ifndef VOCAB_H
#define VOCAB_H

////////////////////////////////////////////////////////////////////////////////
//
// FILE:        vocab.h
// DESCRIPTION: builds vocabID2S and vocabS2ID
// AUTHOR:      Dan Fabian and Lauren Greathouse
// DATE:        4/19/2019

#include <unordered_map>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <utility>
#include "EF_encoder.h" // for SIZE_TRACKER

using std::unordered_map;
using std::istream; using std::getline;
using std::string;
using std::vector;
using std::sort; using std::reverse;
using std::pair; using std::make_pair;

////////////////////////////////////////////////////////////////////////////////
//
// VOCAB

class Vocab {
public:
  Vocab(istream&, int);
  ~Vocab() 
    { SIZE_TRACKER = SIZE_TRACKER - sizeof(vocabID2S) - sizeof(vocabS2ID); }

  static unordered_map<size_t, string> vocabID2S;
  static unordered_map<string, size_t> vocabS2ID;
};

// init static members
unordered_map<size_t, string> Vocab::vocabID2S;
unordered_map<string, size_t> Vocab::vocabS2ID;

////////////////////////////////////////////////////////////////////////////////
//
// VOCAB member functions
////////////////////////////////////////
Vocab::Vocab(istream& inFile, int gramLen)
{
  // store each word and count how many times they occur so 
  // words that occur the most get the smallest IDs
  unordered_map<string, size_t> allWords;
  string word;
  while (!inFile.eof())
    {
      for (int i = 0; i != gramLen; ++i)
	{
	  inFile >> word;
	  // if there is a tab then its not what we want in our vocab
	  if (word.find('\t') == string::npos)
	    ++allWords[word];
	}
      
      // move to next line, ignore the counts
      getline(inFile, word);
    }

  // now put unordered_map into vector to be sorted
  vector<pair<string, size_t>> wordsToSort;
  for (const auto& e: allWords)
    wordsToSort.push_back(make_pair(e.first, e.second));
  
  // sort by occurence
  sort(wordsToSort.begin(), wordsToSort.end(),
       [](pair<string, size_t> a, pair<string, size_t> b)
       { return a.second < b.second; });

  // highest is now at index 0
  reverse(wordsToSort.begin(), wordsToSort.end());
  
  // now make vocab
  size_t startNum = 3; // need this since EF Encoding won't work with
                       // integers of 2 or less
  for (size_t i = 0; i != wordsToSort.size(); ++i)
    {
      vocabID2S[startNum + i] = wordsToSort[i].first;
      vocabS2ID[wordsToSort[i].first] = startNum + i;
    }

  // track size
  SIZE_TRACKER += sizeof(vocabID2S);
  SIZE_TRACKER += sizeof(vocabS2ID);
}

#endif // VOCAB_H
