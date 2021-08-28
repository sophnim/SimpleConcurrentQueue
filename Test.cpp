#include <thread>
#include "SimpleConcurrentQueue.h"

using namespace std;
using namespace SimpleConcurrentQueue;

struct Message {
	char context[128];
};

FixedSizeConcurrentQueue<Message> queue(100);

void enqueueFunc()
{
	while (true) {
		Person* message = new Message();
		if (!queue.TryEnqueue(message)) {
			// queue full
			delete message;
		}
	}
}

void dequeueFunc()
{
	while (true) {
		Message* message = queue.TryDequeue();
		if (!message) {
			// queue is empty
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
		t[i] = thread(enqueueFunc);
	}

	for (int i = 5; i < 10; ++i) {
		t[i] = thread(dequeueFunc);
	}

	for (int i = 0; i < 10; ++i) {
		t[i].join();
	}

	return 0;
}