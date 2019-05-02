#ifndef TRIE_H
#define TRIE_H

////////////////////////////////////////////////////////////////////////////////
//
// FILE:        trie.h
// DESCRIPTION: contains main structure - trie
// AUTHOR:      Dan Fabian and Lauren Greathouse
// DATE:        4/19/2019

#include "node.h"
#include <string>
#include <utility>
#include <cassert>
#include <vector>
#include <sstream>

using std::istream; using std::getline; 
using std::string; using std::stoi;
using std::pair; using std::make_pair;
using std::vector;
using std::stringstream;

////////////////////////////////////////////////////////////////////////////////
//
// TRIE

class Trie {
public:
  // constructor
  Trie(istream&, const int&, const int&); // pass istream to file where data is 
  ~Trie() { SIZE_TRACKER -= sizeof(*this); delete roots_; }

  // methods or queries
  vector<string> mostLikelyNext (const vector<string>&, const int&) const;
  size_t         frequencyCount (const vector<string>&)             const;

private:
  HashmapEF *roots_;

};

////////////////////////////////////////////////////////////////////////////////
//
// TRIE member functions
////////////////////////////////////////
// only works on trigram as of now
Trie::Trie(istream& inFile, const int& gramLen, const int& k)
{
  vector<vector<Node*>> levelNodes(gramLen);
  vector<int> levelCounts(gramLen - 1, 0);

  string word;
  int previousSimilarCount;
  vector<pair<vector<string>, int>> storedLines;
  while(true)
    {      
      // seperate lines into vec of words the back of the context vec is 
      // the leaf, and also store the count
      pair<vector<string>, int> line;
      for (int i = 0; i != gramLen + 1 && !inFile.eof(); ++i) // plus one to include the count
	{
	  inFile >> word;
	  if (word == "") // possible if at the end of the file
	    break;
	    
	  // if there is a tab then its not what we want in our vocab
	  if (word.find('\t') == string::npos && i != gramLen)
	    line.first.push_back(word);
	  else
	    line.second = stoi(word);
	}      

      // once line has been read and parsed, create nodes
      if (!storedLines.empty()) // if its the first line, skip this section
	{
	  // check similarities with previous line
	  int similarCount = 0;
	  pair<vector<string>, int> context = storedLines.back();
	  bool diff = false;
	  for (int i = 0; i != context.first.size() - 1 && !diff; ++i)
	    {
	      if (context.first[i] == line.first[i])
		++similarCount;
	      else
		diff = true;
	    }

	  // leaf case
	  if (similarCount <= gramLen - 1 || levelNodes[gramLen - 1].empty() ||
	      inFile.eof())
	    {
                  levelNodes[gramLen - 1].push_back
                    (new Node((*Encoder::vocabS2ID_)[context.first[context.first.size() - 1]],
                              context.second, k));
                  levelCounts[gramLen - 2] += context.second;
	    }
	  // intermediate node cases
	  for (int i = 2; i < gramLen; ++i)
	    {
	      if (similarCount <= gramLen - i || inFile.eof())
		{
		  levelNodes[gramLen - i].push_back
		    (new Node((*Encoder::vocabS2ID_)[context.first[context.first.size() - i]],
			      levelCounts[gramLen - i], k, levelNodes[gramLen - i + 1]));
		  levelCounts[gramLen - i - 1] += levelCounts[gramLen - i];
		  levelNodes[gramLen - i + 1].clear();
		  levelCounts[gramLen - i] = 0;
		}
	    }
	  // root case
	  if (similarCount == 0 || inFile.eof()) // root
	    {
	      levelNodes[0].push_back
		(new Node((*Encoder::vocabS2ID_)[context.first[0]],
			  levelCounts[0], k, levelNodes[1]));
	      levelCounts[0] = 0;
	      levelNodes[1].clear();
	    }

	  // at the very last line just insert the root then return
          if (inFile.eof())
            {
              roots_ = new HashmapEF(levelNodes[0]);
	      
	      // track size
	      SIZE_TRACKER += sizeof(*this);
              return;
            }

	  previousSimilarCount = similarCount;
	}

      // store line
      storedLines.push_back(line);
    }
}

////////////////////////////////////////
// returns the top num of successors of the context string
vector<string> Trie::mostLikelyNext(const vector<string>& tokens, 
				    const int& num) const
{
  Node* branch = roots_->get(tokens[0]);
  for (int i = 1; i != tokens.size(); ++i)
    branch = branch->findSuccessor(tokens[i]);

  return branch->mostLikelyNext(num);
}

size_t Trie::frequencyCount(const vector<string>& tokens) const
{
  Node* branch = roots_->get(tokens[0]);
  for (int i = 1; i != tokens.size(); ++i)
    branch = branch->findSuccessor(tokens[i]);

  return branch->getFreq();
}

#endif // TRIE_H
