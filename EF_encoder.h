#ifndef ENCODER_H
#define ENCODER_H

////////////////////////////////////////////////////////////////////////////////
//
// FILE:        EF_encoder.h
// DESCRIPTION: contains class for encoding and accessing elements from EF
// AUTHOR:      Dan Fabian and Lauren Greathouse
// DATE:        4/18/2019

#include <vector>
#include <bitset>
#include <math.h>
#include <iostream>
#include <unordered_map>
#include <string>

using std::vector;
using std::bitset;
using std::cout;
using std::unordered_map;
using std::string;

const int bits = 64; // used for bitsets

size_t SIZE_TRACKER = 0; // Keeps track of memory storage used by trie

////////////////////////////////////////////////////////////////////////////////
//
// ENCODER

class Encoder {
public:
  // constructor
  Encoder(vector<size_t>);
  ~Encoder();

  // methods
  size_t access        (const size_t&) const; // access to i-th element
  size_t size          ()              const { return size_; }
  void   printSequence ()              const; // for testing

  // vocabulary of grams and IDs
  // assigned in main file by Encoder::vocab_ = map
  static unordered_map<string, size_t> *vocabS2ID_;
  static unordered_map<size_t, string> *vocabID2S_;

private:
  vector<bool> bitSequence_;
  int maxBits_;     // bits needed for largest number
  size_t size_;     // how many elements there are
  int lowerBitNum_; // number of lower bits
};

unordered_map<string, size_t>* Encoder::vocabS2ID_ = nullptr;
unordered_map<size_t, string>* Encoder::vocabID2S_ = nullptr;

////////////////////////////////////////////////////////////////////////////////
//
// Member functions
////////////////////////////////////////
// sequence MUST be non-decreasing and longer than 1 and use nums greater than 2
Encoder::Encoder(vector<size_t> sequence)
{
  // convert sequence of size_t to bits with size ceiling(log2(max(sequence)))
  // since the sequence is sorted back should contain the largest element
  maxBits_ = ceil(log2(sequence.back())) + 1; // m or the universe
  size_ = sequence.size(); // n
  vector<bitset<bits>> bitRepSequence;

  // converting
  for (size_t i = 0; i != size_; ++i)
    bitRepSequence.push_back(bitset<bits>(sequence[i]));

  // if it is just a single number, store it as binary without encoding
  if (size_ == 1)
    {
      for (size_t i = 0; i != bitRepSequence[0].size(); ++i)
	bitSequence_.push_back(bitRepSequence[0][i]);
      return;
    }

  lowerBitNum_ = ceil(log2(sequence.back()/sequence.size()));
  const size_t highBitNum = maxBits_ - lowerBitNum_;

  // create the bit sequence that will be returned
  // add the lower bits concatenated to the begining of the bitSequence
  lowerBitNum_ = maxBits_ - highBitNum;
  for (size_t i = 0; i != size_; ++i)
    for (size_t j = 0; j != lowerBitNum_; ++j)
      {
	bitSequence_.push_back(bitRepSequence[size_ - i - 1][j]);
	bitRepSequence[size_ - i - 1][j] = 0; // to get just the size of the upper bits
      }
  
  // create buckets to store high bit occurences
  vector<size_t> highBitBuckets(pow(2, highBitNum), 0);

  // getting high bit occuerences
  for (size_t i = 0; i != size_; ++i)
    {
      size_t num = bitRepSequence[i].to_ullong();
      
      // divide num by 2 for the amount of lowerBits
      for (size_t j = 0; j != lowerBitNum_; ++j)
	num = floor(num / 2);

      ++highBitBuckets[num]; 
    }

  // now finish the bit sequence
  size_t HBBsize = highBitBuckets.size();
  for (size_t i = 0; i != HBBsize; ++i)
    {
      bitSequence_.push_back(0);
      for (size_t j = 0; j != highBitBuckets[i]; ++j)
	bitSequence_.push_back(1);
    }

  // Add memory used
  SIZE_TRACKER += sizeof(*this);
}

////////////////////////////////////////
Encoder::~Encoder()
{
  SIZE_TRACKER -= sizeof(*this);
}

////////////////////////////////////////
size_t Encoder::access(const size_t& rank) const
{
  // get lower bits
  bitset<bits> binaryElement;

  // check if it was a size of 1, then just convert to size_t
  if (size_ == 1)
    {
      for (size_t i = 0; i != bitSequence_.size(); ++i)
	binaryElement[i] = bitSequence_[i];
      return binaryElement.to_ullong();
    }

  // the range for the lower bits is 
  // [(size_ - rank - 1) * lowerBitNum + 1, size_ - rank * lowerBitNum]
  for (int i = 0; i != lowerBitNum_; ++i)
    binaryElement[i] = bitSequence_[(size_ - rank - 1) * lowerBitNum_ + i];

  // get high bits
  // find the position of the i-th 1 from the right, after the lower bits
  int pos = 0;
  for (int count = 0; count != rank + 1; ++pos)
    if (bitSequence_[lowerBitNum_ * size_ + pos] == 1)
      ++count;

  size_t highNum = pos - rank - 2; // - 2 for the extra pos after the count is found
                                   // and finding 1 extra rank
  bitset<bits> highBits(highNum);

  // append the highBits to the end of the low ones
  for (int i = 0; i < maxBits_ - lowerBitNum_; ++i)
    binaryElement[lowerBitNum_ + i] = highBits[i];

  return binaryElement.to_ullong();
}

////////////////////////////////////////
void Encoder::printSequence() const
{
  for (size_t i = 0; i != bitSequence_.size(); ++i)
    cout << bitSequence_[bitSequence_.size() - i - 1];
  cout << '\n';
}

#endif // ENCODER_H
