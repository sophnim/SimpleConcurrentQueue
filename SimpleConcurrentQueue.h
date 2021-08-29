/*
* SimpleConcurrentQueue
* created by sophnim. 2021-08-28
*/

#include <atomic>

using namespace std;

namespace SimpleConcurrentQueue
{
	template <typename T>
	class FixedSizeConcurrentQueue {
	private:
		int size_;
		atomic<int> emptySpace_;
		atomic<int> count_;
		atomic<uint32_t> enqueueIndexGenerator_;
		atomic<uint32_t> dequeueIndexGenerator_;
		atomic<T*>* queue_;

	public:
		FixedSizeConcurrentQueue(int size)
		{
			size_ = size;
			emptySpace_ = size;
			count_ = 0;
			enqueueIndexGenerator_ = size - 1;
			dequeueIndexGenerator_ = size - 1;
			queue_ = new atomic<T*>[size];
			
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

			uint32_t index = 0;
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

			uint32_t index = 0;
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

		void Enqueue(T* item)
		{
			int tryCount = 0;
			
			while (!TryEnqueue(item)) {
				std::this_thread::yield();
			}
		}

		T* Dequeue()
		{
			T* item = nullptr;
			int tryCount = 0;
			
			while (true) {
				item = TryDequeue();
				if (item) {
					break;
				}

				std::this_thread::yield();
			}

			return item;
		}
	};
}