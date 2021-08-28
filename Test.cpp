#include <thread>
#include "SimpleConcurrentQueue.h"

using namespace std;
using namespace SimpleConcurrentQueue;

struct Person {
	int age;
	char name[12];
};

FixedSizeConcurrentQueue<Person> queue(100);

void enqueueFunc()
{
	int count = 0;
	while (true) {
		Person* person = new Person();
		queue.Enqueue(person);
	}
}

void dequeueFunc()
{
	while (true) {
		Person* person = queue.Dequeue();
		if (person != nullptr) {
			delete person;
		}
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