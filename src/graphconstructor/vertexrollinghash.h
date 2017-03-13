#ifndef _VERTEX_ROLLING_HASH_H_
#define _VERTEX_ROLLING_HASH_H_

#include "common.h"
#include "ngramhashing/cyclichash.h"

namespace TwoPaCo
{
	typedef CyclicHash<uint64_t> HashFunction;
	typedef std::unique_ptr<HashFunction> HashFunctionPtr;

	class VertexRollingHashSeed
	{
	public:
		VertexRollingHashSeed(size_t numberOfFunctions, size_t vertexLength, size_t bits)
		{
			hashFunction_.resize(numberOfFunctions);
			for (HashFunctionPtr & ptr : hashFunction_)
			{
				ptr = HashFunctionPtr(new HashFunction(vertexLength, bits));
			}
		}	

	private:
		DISALLOW_COPY_AND_ASSIGN(VertexRollingHashSeed);
		std::vector<HashFunctionPtr> hashFunction_;
		friend class VertexRollingHash;
	};

	class VertexRollingHash
	{
	public:
		VertexRollingHash(const VertexRollingHashSeed & seed, std::string::const_iterator begin)
		{
			size_t size = seed.hashFunction_[0]->wordsize;
			posVertexHash_.resize(seed.hashFunction_.size());
			negVertexHash_.resize(seed.hashFunction_.size());
			for (size_t i = 0; i < posVertexHash_.size(); i++)
			{
				posVertexHash_[i] = HashFunctionPtr(new HashFunction(*seed.hashFunction_[i]));
				negVertexHash_[i] = HashFunctionPtr(new HashFunction(*seed.hashFunction_[i]));
				for (auto it = begin; it != begin + size; ++it)
				{
					posVertexHash_[i]->eat(*it);
				}

				assert(posVertexHash_[i]->hashvalue == posVertexHash_[i]->hash(std::string(begin, begin + size)));
				for (std::string::const_reverse_iterator it(begin + size); it != std::string::const_reverse_iterator(begin); ++it)
				{
					char ch = DnaChar::ReverseChar(*it);
					negVertexHash_[i]->eat(DnaChar::ReverseChar(*it));
				}

				assert(negVertexHash_[i]->hashvalue == negVertexHash_[i]->hash(DnaChar::ReverseCompliment(std::string(begin, begin + size))));
			}
		}

		void Update(char positivePreviousChar, char positiveNextChar)
		{
			char negativeNextChar = DnaChar::ReverseChar(positiveNextChar);
			char negativePreviousChar = DnaChar::ReverseChar(positivePreviousChar);
			for (size_t i = 0; i < posVertexHash_.size(); i++)
			{
				posVertexHash_[i]->update(positivePreviousChar, positiveNextChar);
				negVertexHash_[i]->reverse_update(negativeNextChar, positivePreviousChar);
			}
		}

		bool Assert(std::string::const_iterator begin) const
		{
			size_t size = posVertexHash_[0]->wordsize;
			for (size_t i = 0; i < posVertexHash_.size(); i++)
			{
				assert(posVertexHash_[i]->hashvalue == posVertexHash_[i]->hash(std::string(begin, begin + size)));
				assert(negVertexHash_[i]->hashvalue == negVertexHash_[i]->hash(DnaChar::ReverseCompliment(std::string(begin, begin + size))));
			}

			return true;
		}

		uint64_t GetVertexHash() const
		{			
			uint64_t posHash = posVertexHash_[0]->hashvalue;
			uint64_t negHash = negVertexHash_[0]->hashvalue;
			return std::min(posHash, negHash);
		}

		uint64_t GetIngoingEdgeHash(char nextPositiveCharacter, std::vector<uint64_t> & value) const
		{
			value.clear();
			char nextNegativeCharacter = DnaChar::ReverseChar(nextPositiveCharacter);
			StrandComparisonResult result = DetermineStrandPrepend(nextPositiveCharacter, nextNegativeCharacter);
			if (result == positiveLess || result == tie)
			{
				GetPrependValues(nextPositiveCharacter, posVertexHash_, value);
			}
			else
			{
				GetExtendValues(nextNegativeCharacter, negVertexHash_, value);
			}
		}

		uint64_t GetOutgoingEdgeHash(char previousPositiveCharacter, std::vector<uint64_t> & value) const
		{
			value.clear();
			char previousNegativeCharacter = DnaChar::ReverseChar(previousPositiveCharacter);
			StrandComparisonResult result = DetermineStrandExtend(previousPositiveCharacter, previousNegativeCharacter);
			if (result == positiveLess || result == tie)
			{
				GetExtendValues(previousPositiveCharacter, posVertexHash_, value);
			}
			else
			{
				GetPrependValues(previousNegativeCharacter, negVertexHash_, value);
			}
		}

	private:
		DISALLOW_COPY_AND_ASSIGN(VertexRollingHash);		
		std::vector<HashFunctionPtr> posVertexHash_;
		std::vector<HashFunctionPtr> negVertexHash_;

		enum StrandComparisonResult
		{
			positiveLess,
			negativeLess,
			tie
		};

		void GetPrependValues(char nextCh, const std::vector<HashFunctionPtr> & hashFunction, std::vector<uint64_t> & value) const
		{
			for (size_t i = 0; i < hashFunction.size(); i++)
			{
				value.push_back(hashFunction[i]->hash_extend(nextCh));
			}
		}

		void GetExtendValues(char nextCh, const std::vector<HashFunctionPtr> & hashFunction, std::vector<uint64_t> & value) const
		{
			for (size_t i = 0; i < hashFunction.size(); i++)
			{
				value.push_back(hashFunction[i]->hash_prepend(nextCh));
			}
		}

		StrandComparisonResult DetermineStrandExtend(char nextCh, char revNextCh) const
		{
			for (size_t i = 0; i < posVertexHash_.size(); i++)
			{
				uint64_t posHash = posVertexHash_[i]->hash_extend(nextCh);
				uint64_t negHash = negVertexHash_[i]->hash_prepend(revNextCh);
				if (posHash != negHash)
				{
					return posHash < negHash ? positiveLess : negativeLess;
				}
			}

			return tie;
		}

		StrandComparisonResult DetermineStrandPrepend(char prevCh, char revPrevCh) const
		{
			for (size_t i = 0; i < posVertexHash_.size(); i++)
			{
				uint64_t posHash = posVertexHash_[i]->hash_prepend(prevCh);
				uint64_t negHash = negVertexHash_[i]->hash_extend(revPrevCh);
				if (posHash != negHash)
				{
					return posHash < negHash ? positiveLess : negativeLess;
				}
			}

			return tie;
		}
	};
}


#endif