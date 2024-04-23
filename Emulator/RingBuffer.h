#pragma once

#include <array>

template<typename T, size_t N>
class RingBuffer {
protected:
	typedef std::array<T, N> buftype;

	buftype buffer;

	typename buftype::iterator read;
	typename buftype::iterator write;

public:
	RingBuffer(): read(buffer.begin()), write(buffer.begin()) {}

	size_t size() const {
		size_t diff = write - read;
		if(diff > N)
			diff += N;
		return diff;
	}

	void push(const T &item) {
		*write++ = item;
		if(write >= buffer.end())
			write = buffer.begin();
	}

	T pop() {
		const T ret = *read++;
		if(read >= buffer.end())
			read = buffer.begin();
		return ret;
	}
};
