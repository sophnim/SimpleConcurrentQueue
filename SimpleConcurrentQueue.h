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
		int capacity_;
		atomic<int> emptySpace_;
		atomic<int> itemCount_;
		atomic<uint32_t> enqueueIndexGenerator_;
		atomic<uint32_t> dequeueIndexGenerator_;
		atomic<T*>* queue_;

	public:
		FixedSizeConcurrentQueue(int capacity)
		{
			capacity_ = capacity;
			emptySpace_ = capacity;
			itemCount_ = 0;
			enqueueIndexGenerator_ = capacity - 1;
			dequeueIndexGenerator_ = capacity - 1;
			queue_ = new atomic<T*>[capacity];
			
			for (int i = 0; i < capacity; ++i) {
				queue_[i] = nullptr;
			}
		}

		bool TryEnqueue(T* item)
		{
			int space = --emptySpace_;
			
			if (space < 0) {
				++emptySpace_;
				return false;
			}

			uint32_t index = 0;
			T* expected = nullptr;

			do {
				uint32_t temp = ++enqueueIndexGenerator_;
				index = (temp % capacity_);
			} while (!queue_[index].compare_exchange_strong(expected, item));

			++itemCount_;

			return true;
		}

		T* TryDequeue()
		{
			int count = --itemCount_;
			
			if (count < 0) {
				++itemCount_;
				return nullptr;
			}

			uint32_t index = 0;
			T* expected = nullptr;
			T* desired = nullptr;

			do {
				uint32_t temp = ++dequeueIndexGenerator_;
				index = (temp % capacity_);
				expected = queue_[index];
				if (!expected) {
					continue;
				}
			} while (!queue_[index].compare_exchange_strong(expected, desired));

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