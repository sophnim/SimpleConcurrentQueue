/*
* SimpleConcurrentQueue
* created by sophnim. 2021-08-28
*/

#include <atomic>
#include <stdexcept>

namespace SimpleConcurrentQueue
{
	template <typename T>
	class FixedSizeConcurrentQueue {
	private:
		int size_;
		std::atomic<int> emptySpace_;
		std::atomic<int> count_;
		std::atomic<uint64_t> enqueueIndexGenerator_;
		std::atomic<uint64_t> dequeueIndexGenerator_;
		std::atomic<T*>* queue_;

	public:
		FixedSizeConcurrentQueue(int size)
		{
			if (size <= 0) {
				throw std::invalid_argument("constructor invalid size parameter");
			}

			size_ = size;
			emptySpace_ = size;
			count_ = 0;
			enqueueIndexGenerator_ = size - 1;
			dequeueIndexGenerator_ = size - 1;
			queue_ = new std::atomic<T*>[size];
			
			for (int i = 0; i < size; ++i) {
				queue_[i] = nullptr;
			}
		}
		
		~FixedSizeConcurrentQueue()
		{
			delete [] queue_;
		}
		
		int Size()
		{
			return size_;
		}
		
		int Count()
		{
			return count_;
		}

		bool TryEnqueue(T* item)
		{
			if (--emptySpace_ < 0) {
				++emptySpace_;
				return false;
			}

			uint64_t index = 0;
			T* expected = nullptr;

			do {
				index = (++enqueueIndexGenerator_ % size_);
			} while (!queue_[index].compare_exchange_weak(expected, item));

			++count_;

			return true;
		}

		T* TryDequeue()
		{
			if (--count_ < 0) {
				++count_;
				return nullptr;
			}

			uint64_t index = 0;
			T* expected = nullptr;
			T* desired = nullptr;

			do {
				index = (++dequeueIndexGenerator_ % size_);
				expected = queue_[index];
				if (!expected) {
					continue;
				}
			} while (!queue_[index].compare_exchange_weak(expected, desired));

			++emptySpace_;

			return expected;
		}
	};
}