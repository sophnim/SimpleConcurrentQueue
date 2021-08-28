#include <thread>
#include "SimpleConcurrentQueue.h"

using namespace std;
using namespace SimpleConcurrentQueue;

struct Message {
	char context[128];
};

FixedSizeConcurrentQueue<Message> queue(100);

void producer()
{
	while (true) {
		Person* message = new Message();
		
		// create new message and enqueue
		if (!queue.TryEnqueue(message)) {
			// queue full
			delete message;
		}
	}
}

void consumer()
{
	while (true) {
		Message* message = queue.TryDequeue();
		if (!message) {
			// queue has no message
			continue;
		}
		
		// do something with message
		
		delete message;
	}
}

int main()
{
	thread t[10];

	for (int i = 0; i < 5; ++i) {
		t[i] = thread(producer);
	}

	for (int i = 5; i < 10; ++i) {
		t[i] = thread(consumer);
	}

	for (int i = 0; i < 10; ++i) {
		t[i].join();
	}

	return 0;
}