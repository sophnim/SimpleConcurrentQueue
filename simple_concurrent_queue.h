/*
* SimpleConcurrentQueue
* created by sophnim. 2021-08-28
*/

#ifndef __SIMPLE_CONCURRENT_QUEUE__
#define __SIMPLE_CONCURRENT_QUEUE__

#include <atomic>
#include <stdexcept>

namespace simple_concurrent_queue
{
	template <typename T>
	class FixedSizeConcurrentQueue {
	private:
		int size_;
		std::atomic<int> empty_space_;
		std::atomic<int> count_;
		std::atomic<uint64_t> enqueue_index_generator_;
		std::atomic<uint64_t> dequeue_index_generator_;
		std::atomic<T*>* queue_;

	public:
		FixedSizeConcurrentQueue(int size)
		{
			if (size <= 0) {
				throw std::invalid_argument("constructor invalid size parameter");
			}

			size_ = size;
			empty_space_ = size;
			count_ = 0;
			enqueue_index_generator_ = size - 1;
			dequeue_index_generator_ = size - 1;
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
			if (--empty_space_ < 0) {
				++empty_space_;
				return false;
			}

			uint64_t index = 0;
			T* expected = nullptr;

			do {
				index = (++enqueue_index_generator_ % size_);
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
				index = (++dequeue_index_generator_ % size_);
				expected = queue_[index];
				if (!expected) {
					continue;
				}
			} while (!queue_[index].compare_exchange_weak(expected, desired));

			++empty_space_;

			return expected;
		}
	};
}

#endif // __SIMPLE_CONCURRENT_QUEUE__
