#ifndef NODE_H
#define NODE_H

////////////////////////////////////////////////////////////////////////////////
//
// FILE:        node.h
// DESCRIPTION: contains lower level classes - hashmap, sorted array, and node
// AUTHOR:      Dan Fabian and Lauren Greathouse
// DATE:        4/17/2019

#include <vector>
#include <string>
#include <algorithm>
#include "EF_encoder.h"

using std::vector;
using std::string;
using std::sort; using std::reverse;

// SIZE TRACKER in bytes, stored in EF_encoder.h

////////////////////////////////////////////////////////////////////////////////
//
// NODE

class HashmapEF; // forward declarations
class SortedEF;

class Node {
public:
  Node(const size_t&, const size_t&, const int&, vector<Node*>);    
  ~Node();

  // methods
  size_t         getGramID      ()              const { return gram_; }
  size_t         getFreq        ()              const { return frequency_; }
  Node*          findSuccessor  (const string&) const; // return nullptr 
                                                       // if not found
  vector<string> mostLikelyNext (const size_t&) const;
  
  // used for sorting
  bool operator<(const Node& rhs) const { return frequency_ < rhs.frequency_; }
  
private:
  size_t gram_;
  size_t frequency_;
  static int k_; // MUST BE GREATER THAN ONE
  HashmapEF *successors_;
  SortedEF *topK_;
};

int Node::k_ = 0; // static member init

////////////////////////////////////////////////////////////////////////////////
//
// HASHMAP EF

class HashmapEF {
public:
  // constructor
  HashmapEF(const vector<Node*>&);
  ~HashmapEF();

  // methods
  Node*  get     (const string&)    const;
  Node*  getRank (const int&)       const; // inefficient compared to SortedEF's 
  size_t hash    (const size_t& ID) const { return ID % size_; }
  size_t getSize ()                 const { return size_; }

private:
  Encoder *grams_;    // contains words of the Nodes
  Encoder *pointers_; // contains pointers to Nodes, indicies 
                      // correspond with grams_
  size_t size_;
};

////////////////////////////////////////////////////////////////////////////////
//
// SORTED EF

class SortedEF {
public:
  SortedEF(const vector<Node*>&);
  ~SortedEF();

  // methods
  size_t getSize ()              const { return size_; }
  Node*  get     (const string&) const;
  Node*  getRank (const int&)    const; // sorted in decreasing order so rank 
                                        // 0 is most freq
  void   print   ()              const; // for testing

private:
  Encoder *grams_;    // contains words of the Nodes
  Encoder *pointers_; // contains pointers to Nodes, indicies
                      // correspond with grams_
  int size_;
};

////////////////////////////////////////////////////////////////////////////////
//
// NODE member functions
////////////////////////////////////////
Node::Node(const size_t& gramID, const size_t& freq, const int& k, 
	   vector<Node*> successors = vector<Node*>())
: gram_(gramID), frequency_(freq), topK_(nullptr), successors_(nullptr)
{
  k_ = k;
  if (successors.size() == 0)
    return;

  // first sort the vector to get the top k
  // since we start with a vector of pointers we have to turn them into
  // normal nodes to be sorted properly
  sort(successors.begin(), successors.end(), 
       [](Node* a, Node* b) { return *a < *b; });

  // need to reverse since sort puts in increasing order
  reverse(successors.begin(), successors.end());

  vector<Node*> sortedTopK(k_);

  // if k_ + 1 is larger than the amount of successors only store the 
  // exact amount of successors because WE CAN'T have a sequence of length
  // 1 for the encoder to work properly
  if (k_ + 1 >= successors.size())
    sortedTopK.resize(successors.size());

  for (int i = 0; i != sortedTopK.size(); ++i)
    sortedTopK[i] = successors[i];

  topK_ = new SortedEF(sortedTopK);

  // if k_ + 1 >= successors.size() then leave successors_ nullptr
  if (k_ + 1 >= successors.size())
    return;

  // once topk has been placed erase the first k entries
  successors.erase(successors.begin(), successors.begin() + k_);
  successors_ = new HashmapEF(successors);

  // track size
  SIZE_TRACKER += sizeof(*this);
}

////////////////////////////////////////
Node::~Node()
{
  SIZE_TRACKER -= sizeof(*this);

  delete successors_;
  delete topK_;
}

////////////////////////////////////////
Node* Node::findSuccessor(const string& word) const
{
  // if its a leaf node
  if (topK_ == nullptr && successors_ == nullptr)
    return nullptr;

  // first search the topK_, then successors_
  Node* element = topK_->get(word);
  if (element != nullptr)
    return element;
  
  // then search successors hashmapEF
  return successors_->get(word);
}

////////////////////////////////////////
vector<string> Node::mostLikelyNext(const size_t& num) const
{
  // return empty vector if there are no successors
  if (topK_ == nullptr && successors_ == nullptr)
    return vector<string>();

  vector<string> result(num);
  size_t maxReturn;

  // check if there is a successors_ hashmapEF
  if (successors_ != nullptr)
    maxReturn = topK_->getSize() + successors_->getSize();
  else
    maxReturn = topK_->getSize();

  // if num is larger than total successors_ then return all successors
  if (maxReturn < num)
    result.resize(maxReturn);

  size_t i = 0;
  for (; i != result.size() && i != topK_->getSize(); ++i)
    result[i] = 
      (*Encoder::vocabID2S_)[topK_->getRank(i)->getGramID()]; 

  // if we got all the results we want return
  if (i == result.size())
    return result;

  // if we need more we start going through the successors_
  for (size_t j = 0; j != result.size() - i && j != successors_->getSize(); ++j)
    result[i + j] = 
      (*Encoder::vocabID2S_)[successors_->getRank(j)->getGramID()]; 

  return result;
}

////////////////////////////////////////////////////////////////////////////////
//
// HASHMAP EF member functions
////////////////////////////////////////
HashmapEF::HashmapEF(const vector<Node*>& nodes) : size_(nodes.size())
{
  // to make the sequence of gramIDs non-decreasing we add the previous
  // elements ID and so on
  vector<size_t*> prefixSumGrams(size_, nullptr); // using size_t pointer
                                                  // to know where an empty 
                                                  // pos is since size_t is 
                                                  // unsigned 
  vector<size_t> gramPointers(size_);

  // filling grams and pointers vectors
  bool posFound;
  size_t hashValue;
  for (size_t i = 0; i != size_; ++i)
    {
      hashValue = nodes[i]->getGramID();
      posFound = false;
      // linear probing stage
      for (size_t j = 0; !posFound && j != size_; ++j) 
	if (prefixSumGrams[hash(hashValue + j)] == nullptr)
	  {
	    prefixSumGrams[hash(hashValue + j)] = 
	      new size_t(nodes[i]->getGramID());
	    gramPointers[hash(hashValue + j)] = 
	      reinterpret_cast<size_t>(nodes[i]);
	    
	    posFound = true;
	  }
    }

  // use prefix sums on gramPointers to make it into an increasing sequence
  for (size_t i = 0; i != gramPointers.size(); ++i)
    if (i != 0)
      gramPointers[i] = gramPointers[i] + gramPointers[i - 1];

  pointers_ = new Encoder(gramPointers);

  // now that prefixSumGrams is filled with pointers to IDs, we add
  // the previous element to it to make it increasing, the first element 
  // remains the same
  vector<size_t> nonPointerGrams(size_);
  for (size_t i = 0; i != size_; ++i)
    {
      if (i != 0)
	nonPointerGrams[i] = 
	  (*prefixSumGrams[i]) + nonPointerGrams[i - 1];
      else
	nonPointerGrams[i] = (*prefixSumGrams[i]);
    }

  grams_ = new Encoder(nonPointerGrams);

  // track size
  SIZE_TRACKER += sizeof(*this);
}

////////////////////////////////////////
HashmapEF::~HashmapEF()
{
  SIZE_TRACKER -= sizeof(*this);

  // grams is just encoded ints so nothing special needs tbd
  delete grams_;

  for (size_t i = 0; i != pointers_->size(); ++i)
    if (i != 0)
      delete reinterpret_cast<Node*>(pointers_->access(i) - 
				     pointers_->access(i - 1));
  delete reinterpret_cast<Node*>(pointers_->access(0));
  delete pointers_;
}

////////////////////////////////////////
Node* HashmapEF::get(const string& gramName) const
{
  // first retrieve gramName's ID
  size_t ID = (*Encoder::vocabS2ID_)[gramName];
  
  // search for index
  bool posFound = false;
  size_t index = hash(ID);
  size_t elementID;
  for (size_t i = 0; !posFound && i != size_; ++i) // linear probe
    {
      if (hash(index + i) != 0)
	elementID = 
	  grams_->access(hash(index + i)) - 
	  grams_->access(hash(index + i) - 1);
      else
	elementID = grams_->access(0);

      if (elementID == ID)
	{
	  index = hash(index + i);
	  posFound = true;
	}
    }

  if (posFound && index != 0)
    return reinterpret_cast<Node*>(pointers_->access(index) - 
				   pointers_->access(index - 1));
  else if (posFound)
    return reinterpret_cast<Node*>(pointers_->access(index));

  return nullptr;
}

////////////////////////////////////////
Node* HashmapEF::getRank(const int& rank) const
{
  if (rank > size_)
    return nullptr;

  vector<Node*> sortedList; // could use a priority queue instead
  // the rest of the function could be done in linear time but I was lazy
  for (size_t i = 0; i != size_; ++i)
    {
      if (i != 0)
	sortedList.push_back(reinterpret_cast<Node*>(pointers_->access(i) - pointers_->access(i - 1)));
      else
	sortedList.push_back(reinterpret_cast<Node*>(pointers_->access(i)));
    }
  sort(sortedList.begin(), sortedList.end(),
       [](Node* a, Node* b){ return *a < *b; });
  reverse(sortedList.begin(), sortedList.end());

  return sortedList[rank];
}

////////////////////////////////////////////////////////////////////////////////
//
// SORTED EF member functions
////////////////////////////////////////
// nodes are sorted already
SortedEF::SortedEF(const vector<Node*>& nodes) : size_(nodes.size())
{
  // to make the sequence of gramIDs non-decreasing we add the previous
  // elements ID and so on
  vector<size_t> prefixSumGrams(size_);
  vector<size_t> gramPointers(size_);

  // filling grams and pointers vectors
  for (int i = 0; i != size_; ++i)
    {
      prefixSumGrams[i] = nodes[i]->getGramID();
      gramPointers[i] = reinterpret_cast<size_t>(nodes[i]);
    }

  // use prefix sums on gramPointers to make it into an increasing sequence
  for (size_t i = 0; i != gramPointers.size(); ++i)
    if (i != 0)
      gramPointers[i] = gramPointers[i] + gramPointers[i - 1];

  pointers_ = new Encoder(gramPointers);

  // now that prefixSumGrams is filled with IDs, we add
  // the previous element to it to make it increasing, the first element
  // remains the same
  for (size_t i = 0; i != size_; ++i)
    if (i != 0)
      prefixSumGrams[i] = prefixSumGrams[i] + prefixSumGrams[i - 1];

  grams_ = new Encoder(prefixSumGrams);

  // track size
  SIZE_TRACKER += sizeof(*this);
}

////////////////////////////////////////
SortedEF::~SortedEF()
{
  SIZE_TRACKER -= sizeof(*this);

  // grams is just encoded ints so nothing special needs tbd
  delete grams_;

  for (size_t i = 0; i != pointers_->size(); ++i)
    if (i != 0)
      delete reinterpret_cast<Node*>(pointers_->access(i) -
                                     pointers_->access(i - 1));
  delete reinterpret_cast<Node*>(pointers_->access(0));
  delete pointers_;
}

////////////////////////////////////////
Node* SortedEF::get(const string& gramName) const
{
  // first get ID
  size_t ID = (*Encoder::vocabS2ID_)[gramName];

  bool posFound = false;
  size_t elementID;
  int i = 0;
  for (; !posFound && i != size_; ++i)
    {
      if (i != 0)
	elementID = grams_->access(i) - grams_->access(i - 1);
      else
	elementID = grams_->access(0);

      if (elementID == ID)
	posFound = true;
    }

  // need minus 1 because when the for loop ends it increments i
  // one more than needed
  if (posFound && i != 1)
    return reinterpret_cast<Node*>(pointers_->access(i - 1) - 
				   pointers_->access(i - 2));
  else if (posFound)
    return reinterpret_cast<Node*>(pointers_->access(i - 1));

  return nullptr;
}

////////////////////////////////////////
Node* SortedEF::getRank(const int& rank) const
{
  // if rank is too large to be in the topK results
  if (rank > size_)
    return nullptr;

  if (rank != 0)
    return reinterpret_cast<Node*>(pointers_->access(rank) - 
				   pointers_->access(rank - 1));
  else
    return reinterpret_cast<Node*>(pointers_->access(rank));
}

////////////////////////////////////////
void SortedEF::print() const
{
  for (size_t i = 0; i != size_; ++i)
    cout << (*Encoder::vocabID2S_)
      [reinterpret_cast<Node*>(pointers_->access(i))->getGramID()] 
	 << '\n';
}

#endif // NODE_H
