#ifndef _DNA_STRING_H_
#define _DNA_STRING_H_

#include <string>
#include <cstdint>

namespace Sibelia
{
	class DnaString
	{
	public:

		char PopBack()
		{
			return GetChar(size_--);
		}

		char PopFront()
		{
			size_t remain = size_;
			char ret = GetChar(0);
			for (size_t pos = 0; remain > 0; ++pos)
			{
				size_t current = std::min(remain, UNIT_CAPACITY);
				char prev = GetChar(pos, 0);
				str_[pos] >>= 2;
				remain -= current;
				if (pos > 0)
				{
					SetChar(pos - 1, UNIT_CAPACITY - 1, prev);
				}
			}

			--size_;
			return ret;
		}

		void AppendBack(char ch)
		{
			SetChar(size_++, ch);
		}

		void AppendFront(char ch)
		{
			size_t remain = ++size_;
			for (size_t pos = 0; remain > 0; ++pos)
			{
				size_t current = std::min(remain, UNIT_CAPACITY);
				char nextCh = GetChar(pos, current - 1);
				str_[pos] <<= 2;
				str_[pos] |= DnaString::MakeUp(ch);
				ch = nextCh;
				remain -= current;
			}
		}

		size_t GetSize() const
		{
			return size_;
		}

		size_t MaxSize() const
		{
			return capacity_ * UNIT_CAPACITY;
		}

		size_t BytesInBody() const
		{
			return capacity_ * sizeof(str_[0]);
		}

		uint64_t* GetBody() const
		{
			return str_;
		}

		char GetChar(uint64_t idx) const
		{
			uint64_t element = TranslateIdx(idx);
			return GetChar(element, idx);
			
		}

		void SetChar(uint64_t idx, char ch)
		{
			uint64_t element = TranslateIdx(idx);
			SetChar(element, idx, ch);
		}

		~DnaString()
		{
			delete[] str_;
		}

		DnaString(DnaString && str) : size_(str.size_), capacity_(capacity_), str_(0)
		{
			std::swap(str_, str.str_);
		}

		DnaString(const DnaString & str);	
		DnaString(const std::string & str) :size_(str.size()), capacity_(CalculateCapacity(str.size()))
		{
			for (size_t i = 0; i < str.size(); i++)
			{
				SetChar(i, str[i]);
			}
		}

		DnaString(size_t maxSize, size_t size) : size_(size), capacity_(CalculateCapacity(maxSize)),
			str_(new uint64_t[capacity_])
		{

		}

		DnaString(size_t size, const uint64_t * body) :size_(size), capacity_(CalculateCapacity(size))
		{
			std::copy(body, body + capacity_, reinterpret_cast<uint64_t*>(str_));
		}
				
		static char Reverse(char ch)
		{
			switch (ch)
			{
			case 'A':
				return 'T';
			case 'T':
				return 'A';
			case 'C':
				return 'G';
			case 'G':
				return 'C';
			}

			return 'N';
		}

		std::string ToString() const
		{
			std::string ret(size_, ' ');
			for (size_t i = 0; i < size_; i++)
			{
				ret[i] = GetChar(i);
			}

			return ret;
		}

		static uint64_t MakeUp(char ch)
		{
			switch (ch)
			{
			case 'A':
				return 0;
			case 'C':
				return 1;
			case 'G':
				return 2;
			case 'T':
				return 3;
			}

			return 0;
		}

		DnaString RevComp() const
		{
			DnaString ret(size_, size_);
			for (size_t i = 0; i < size_; i++)
			{
				ret.SetChar(size_ - i - 1, Reverse(GetChar(i)));
			}

			return ret;
		}

		static std::string RevComp(const std::string & str)
		{
			std::string ret;
			std::transform(str.rbegin(), str.rend(), std::back_inserter(ret), DnaString::Reverse);
			return ret;
		}

		static const std::string LITERAL;
	private:		
		size_t size_;
		size_t capacity_;
		uint64_t * str_;		
		static const size_t UNIT_CAPACITY = 32;		
		uint64_t TranslateIdx(uint64_t & idx) const
		{
			uint64_t ret = idx >> 5;
			idx = idx & ((uint64_t(1) << uint64_t(5)) - 1);
			return ret;
		}

		static size_t CalculateCapacity(size_t size)
		{
			size_t mainPart = size / UNIT_CAPACITY;
			return mainPart + (mainPart * UNIT_CAPACITY == size ? 0 : 1);
		}

		char GetChar(uint64_t element, uint64_t idx) const
		{
			uint64_t charIdx = str_[element] >> (2 * idx);
			return DnaString::LITERAL[charIdx & 0x3];
		}

		void SetChar(uint64_t element, uint64_t idx, char ch)
		{
			uint64_t mask = uint64_t(0x3) << (idx * 2);
			str_[element] &= ~(mask);
			str_[element] |= DnaString::MakeUp(ch) << (2 * idx++);
		}

		friend bool operator < (const DnaString & a, const DnaString & b);
		friend bool operator == (const DnaString & a, const DnaString & b);
		friend bool operator != (const DnaString & a, const DnaString & b);
	};

	bool operator < (const DnaString & a, const DnaString & b);
	bool operator == (const DnaString & a, const DnaString & b);
	bool operator != (const DnaString & a, const DnaString & b);
}

#endif