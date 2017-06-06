#ifndef _VERTEX_ROLLING_HASH_H_
#define _VERTEX_ROLLING_HASH_H_

#include "common.h"
#include "concurrentbitvector.h"
#include "ngramhashing/cyclichash.h"

namespace TwoPaCo
{
	typedef CyclicHash<uint64_t> HashFunction;
	typedef std::unique_ptr<HashFunction> HashFunctionPtr;

	class VertexRollingHashSeed
	{
	public:
		VertexRollingHashSeed(const VertexRollingHashSeed & s)
		{
			hashFunction_.resize(s.HashFunctionsNumber());
			for (HashFunctionPtr & ptr : hashFunction_)
			{
				ptr = HashFunctionPtr(new HashFunction(s.VertexLength(), s.BitsNumber()));
			}
		}

		VertexRollingHashSeed(size_t numberOfFunctions, size_t vertexLength, size_t bits)
		{
			hashFunction_.resize(numberOfFunctions);
			for (HashFunctionPtr & ptr : hashFunction_)
			{
				ptr = HashFunctionPtr(new HashFunction(vertexLength, bits));
			}
		}		

		size_t VertexLength() const
		{
			return hashFunction_[0]->n;
		}

		size_t BitsNumber() const
		{
			return hashFunction_[0]->wordsize;
		}

		size_t HashFunctionsNumber() const
		{
			return hashFunction_.size();
		}

	private:		
		std::vector<HashFunctionPtr> hashFunction_;
		friend class VertexRollingHash;
	};

	class VertexRollingHash
	{
	public:
		enum StrandComparisonResult
		{
			positiveLess,
			negativeLess,
			tie,
		};

		size_t VertexLength() const
		{
			return posVertexHash_[0]->n;
		}

		size_t BitsNumber() const
		{
			return posVertexHash_[0]->wordsize;
		}

		size_t HashFunctionsNumber() const
		{
			return posVertexHash_.size();
		}

		VertexRollingHash(const VertexRollingHashSeed & seed, std::string::const_iterator begin, size_t hashFunctions, bool singleStrand) : singleStrand_(singleStrand)
		{			
			size_t size = seed.hashFunction_[0]->n;
			posVertexHash_.resize(hashFunctions);
			negVertexHash_.resize(!singleStrand ? hashFunctions : 0);
			for (size_t i = 0; i < posVertexHash_.size(); i++)
			{
				posVertexHash_[i] = HashFunctionPtr(new HashFunction(*seed.hashFunction_[i]));				
				for (auto it = begin; it != begin + size; ++it)
				{
					posVertexHash_[i]->eat(*it);
				}

				assert(posVertexHash_[i]->hashvalue == posVertexHash_[i]->hash(std::string(begin, begin + size)));
			}

			for (size_t i = 0; i < negVertexHash_.size(); i++)
			{
				negVertexHash_[i] = HashFunctionPtr(new HashFunction(*seed.hashFunction_[i]));
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
			}

			for (size_t i = 0; i < negVertexHash_.size(); i++)
			{
				negVertexHash_[i]->reverse_update(negativeNextChar, negativePreviousChar);				
			}
		}

		bool Assert(std::string::const_iterator begin) const
		{
			size_t size = posVertexHash_[0]->n;
			for (size_t i = 0; i < posVertexHash_.size(); i++)
			{
				assert(posVertexHash_[i]->hashvalue == posVertexHash_[i]->hash(std::string(begin, begin + size)));
				if (!singleStrand_)
				{
					assert(negVertexHash_[i]->hashvalue == negVertexHash_[i]->hash(DnaChar::ReverseCompliment(std::string(begin, begin + size))));
				}
			}

			return true;
		}

		uint64_t RawPositiveHash(size_t hf) const
		{
			return posVertexHash_[hf]->hashvalue;
		}

		uint64_t RawNegativeHash(size_t hf) const
		{
			return negVertexHash_[hf]->hashvalue;
		}

		uint64_t GetVertexHash() const
		{	
			uint64_t posHash = posVertexHash_[0]->hashvalue;
			if (!singleStrand_)
			{				
				uint64_t negHash = negVertexHash_[0]->hashvalue;
				return min(posHash, negHash);
			}
			
			return posHash;
		}

		uint64_t GetIngoingEdgeHash(char previousPositiveCharacter, StrandComparisonResult result, size_t idx) const
		{
			char previousNegativeCharacter = DnaChar::ReverseChar(previousPositiveCharacter);
			if (result == positiveLess || result == tie)
			{
				return posVertexHash_[idx]->hash_prepend(previousPositiveCharacter);				
			}
			else
			{
				return negVertexHash_[idx]->hash_extend(previousNegativeCharacter);
			}
		}
		
		uint64_t GetOutgoingEdgeHash(char nextPositiveCharacter, StrandComparisonResult result, size_t idx) const
		{
			char nextNegativeCharacter = DnaChar::ReverseChar(nextPositiveCharacter);
			if (result == positiveLess || result == tie)
			{
				return posVertexHash_[idx]->hash_extend(nextPositiveCharacter);
			}
			else
			{
				return negVertexHash_[idx]->hash_prepend(nextNegativeCharacter);
			}
		}

		StrandComparisonResult DetermineStrandExtend(char nextCh) const
		{
			if (singleStrand_)
			{
				return positiveLess;
			}

			char revNextCh = DnaChar::ReverseChar(nextCh);
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

		StrandComparisonResult DetermineStrandPrepend(char prevCh) const
		{
			if (singleStrand_)
			{
				return positiveLess;
			}

			char revPrevCh = DnaChar::ReverseChar(prevCh);
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

	private:
		DISALLOW_COPY_AND_ASSIGN(VertexRollingHash);
		bool singleStrand_;
		std::vector<HashFunctionPtr> posVertexHash_;
		std::vector<HashFunctionPtr> negVertexHash_;			
	};

	inline bool IsOutgoingEdgeInBloomFilter(const VertexRollingHash & hash, const ConcurrentBitVector & filter, char nextCh)
	{
		VertexRollingHash::StrandComparisonResult result = hash.DetermineStrandExtend(nextCh);	
		for (size_t i = 0; i < hash.HashFunctionsNumber(); i++)
		{
			if (!filter.GetBit(hash.GetOutgoingEdgeHash(nextCh, result, i)))
			{
				return false;
			}
		}

		return true;
	}

	inline bool IsIngoingEdgeInBloomFilter(const VertexRollingHash & hash, const ConcurrentBitVector & filter, char prevCh)
	{
		VertexRollingHash::StrandComparisonResult result = hash.DetermineStrandPrepend(prevCh);
		for (size_t i = 0; i < hash.HashFunctionsNumber(); i++)
		{
			if (!filter.GetBit(hash.GetIngoingEdgeHash(prevCh, result, i)))
			{
				return false;
			}
		}

		return true;
	}

	inline void GetOutgoingEdgeHash(const VertexRollingHash & hash, char nextCh, std::vector<uint64_t> & value)
	{
		VertexRollingHash::StrandComparisonResult result = hash.DetermineStrandExtend(nextCh);
		for (size_t i = 0; i < hash.HashFunctionsNumber(); i++)
		{
			value.push_back(hash.GetOutgoingEdgeHash(nextCh, result, i));
		}
	}

	inline void GetIngoingEdgeHash(const VertexRollingHash & hash, char prevCh, std::vector<uint64_t> & value)
	{
		VertexRollingHash::StrandComparisonResult result = hash.DetermineStrandPrepend(prevCh);
		for (size_t i = 0; i < hash.HashFunctionsNumber(); i++)
		{
			value.push_back(hash.GetIngoingEdgeHash(prevCh, result, i));
		}
	}
}


#endif