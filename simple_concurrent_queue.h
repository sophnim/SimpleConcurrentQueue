/*
* SimpleConcurrentQueue
* created by sophnim. 2021-08-28
*/

#ifndef __SIMPLE_CONCURRENT_QUEUE__
#define __SIMPLE_CONCURRENT_QUEUE__

#include <atomic>
#include <stdexcept>
#include <iostream>
#include <cassert>

namespace simple_concurrent_queue
{
	template <typename T>
	class ConcurrentQueueNode {
	public:
		T value_;
		std::atomic<ConcurrentQueueNode<T>*> next_;

		ConcurrentQueueNode()
		{
			this->next_ = nullptr;
		}

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
			auto node = new ConcurrentQueueNode<T>();
			assert(nullptr != node);

			this->head_.store(node);
			this->tail_.store(node);
		}

		~ConcurrentQueue()
		{
			T value;
			while (TryDequeue(&value));

			auto node = this->head_.load();
			delete node;
		}

		bool TryEnqueue(T value)
		{
			auto node = new ConcurrentQueueNode<T>(value);
			if (nullptr == node) {
				return false;
			}

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
			return true;
		}

		bool TryDequeue(T* ret)
		{
			ConcurrentQueueNode<T>* null_node = nullptr;
			T value;

			while (true) {
				auto head = this->head_.load(std::memory_order_acquire);
				auto tail = this->tail_.load(std::memory_order_acquire);

				if (head == tail) {
					return false;
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

				value = head_next->value_;

				if (!this->head_.compare_exchange_weak(null_node, head_next, std::memory_order_acq_rel)) {
					continue;
				}

				delete head;
				break;
			}

			this->size_.fetch_sub(1, std::memory_order_relaxed);
			*ret = value;

			return true;
		}

		uint64_t Size()
		{
			return this->size_;
		}

	};

	template <typename T>
	class FixedSizeConcurrentQueueNode {
	public:
		T value_;

		FixedSizeConcurrentQueueNode(T value)
		{
			this->value_ = value;
		}
	};

	template <typename T>
	class FixedSizeConcurrentQueue {
	private:
		int capacity_;
		std::atomic<int> empty_space_;
		std::atomic<int> count_;
		std::atomic<uint64_t> enqueue_index_generator_;
		std::atomic<uint64_t> dequeue_index_generator_;
		std::atomic<FixedSizeConcurrentQueueNode<T>*>* queue_;

	public:
		FixedSizeConcurrentQueue(int capacity)
		{
			if (capacity <= 0) {
				throw std::invalid_argument("constructor invalid size parameter");
			}

			capacity_ = capacity;
			empty_space_ = capacity;
			count_ = 0;
			enqueue_index_generator_ = capacity - 1;
			dequeue_index_generator_ = capacity - 1;
			queue_ = new std::atomic<FixedSizeConcurrentQueueNode<T>*>[capacity];

			for (int i = 0; i < capacity; ++i) {
				queue_[i] = nullptr;
			}
		}

		~FixedSizeConcurrentQueue()
		{
			T value;
			while (TryDequeue(&value));

			delete[] queue_;
		}

		int Capacity()
		{
			return capacity_;
		}

		int Count()
		{
			return count_;
		}

		bool TryEnqueue(T value)
		{
			if (--empty_space_ < 0) {
				++empty_space_;
				return false;
			}

			uint64_t index = 0;
			FixedSizeConcurrentQueueNode<T>* expected = nullptr;
			FixedSizeConcurrentQueueNode<T>* node = new FixedSizeConcurrentQueueNode<T>(value);

			do {
				index = (++enqueue_index_generator_ % capacity_);
			} while (!queue_[index].compare_exchange_weak(expected, node));

			++count_;

			return true;
		}

		bool TryDequeue(T* value)
		{
			if (--count_ < 0) {
				++count_;
				return false;
			}

			uint64_t index = 0;
			FixedSizeConcurrentQueueNode<T>* expected = nullptr;
			FixedSizeConcurrentQueueNode<T>* desired = nullptr;

			do {
				index = (++dequeue_index_generator_ % capacity_);
				expected = queue_[index];
				if (!expected) {
					continue;
				}
			} while (!queue_[index].compare_exchange_weak(expected, desired));

			++empty_space_;

			*value = expected->value_;
			delete expected;

			return true;
		}
	};
}

#endif // __SIMPLE_CONCURRENT_QUEUE__