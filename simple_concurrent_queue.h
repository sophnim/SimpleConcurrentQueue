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
	class ConcurrentQueueNode {
	public:
		T value_;
		std::atomic<ConcurrentQueueNode*> next_;

		ConcurrentQueueNode(T value)
		{
			this->value_ = value;
			this->next_ = nullptr;
		}
	};

	template <typename T>
	class ConcurrentQueue {
	private:
		std::atomic<ConcurrentQueueNode<T>*> head_ = nullptr;
		std::atomic<ConcurrentQueueNode<T>*> tail_ = nullptr;
		std::atomic<uint64_t> size_ = 0;

	public:
		ConcurrentQueue()
		{
			auto node = new ConcurrentQueueNode<T>(nullptr);
			this->head_.store(node);
			this->tail_.store(node);
		}

		void Enqueue(T value)
		{
			auto node = new ConcurrentQueueNode<T>(value);

			while (true) {
				auto tail = this->tail_.load(std::memory_order_acquire);
				ConcurrentQueueNode<T>* expected = nullptr;
				if (!tail->next_.compare_exchange_weak(expected, node, std::memory_order_acq_rel)) {
					continue;
				}

				this->tail_.store(node, std::memory_order_release);
				break;
			}

			this->size_.fetch_add(1, std::memory_order_relaxed);
		}

		T TryDequeue()
		{
			ConcurrentQueueNode<T>* null_node = nullptr;
			
			while (true) {
				auto head = this->head_.load(std::memory_order_acquire);
				auto tail = this->tail_.load(std::memory_order_acquire);

				if (head == tail) {
					return nullptr;
				}

				if (nullptr == head) {
					continue;
				}

				if (!this->head_.compare_exchange_weak(head, null_node, std::memory_order_acq_rel)) {
					continue;
				}

				auto head_next = head->next_.load(std::memory_order_acquire);
				if (nullptr == head_next) {
					continue;
				}

				T value = head_next->value_;

				if (!this->head_.compare_exchange_weak(null_node, head_next, std::memory_order_acq_rel)) {
					continue;
				}

				delete head;

				this->size_.fetch_sub(1, std::memory_order_relaxed);
				return value;
			}
		}

		uint64_t Size()
		{
			return this->size_;
		}

	};

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
