#pragma once
#include <Windows.h>

template <class T>
class CCircularQueue {
private:
	size_t  size;
	size_t  count;
	size_t  front;
	size_t  rear;
	T      *buffer;
	SRWLOCK lock;
public:
	CCircularQueue(size_t size) {
		this->size   = size;
		this->count  = 0;
		this->front  = 0;
		this->rear   = 0;
		this->buffer = new T[size];
		InitializeSRWLock(&this->lock);
	}
	~CCircularQueue() {
		delete this->buffer;
	}

	size_t Size() {
		return this->size;
	}

	size_t Count() {
		size_t count;

		AcquireSRWLockShared(&this->lock);
		count = this->count;
		ReleaseSRWLockShared(&this->lock);

		return count;
	}

	size_t Push(size_t count, T *buffer) {
		size_t max_push;
		size_t first_size;
		size_t second_size;

		AcquireSRWLockExclusive(&this->lock);
		max_push    = min(this->size - this->count, count);
		first_size  = min(this->size - this->front, max_push);
		second_size = max_push - first_size;

		if (first_size) {
			memcpy(this->buffer + this->front, buffer, sizeof(T) * first_size);
		}

		if (second_size) {
			memcpy(this->buffer, buffer + first_size, sizeof(T) * second_size);
		}

		this->count += max_push;
		this->front = (this->front + max_push) % this->size;

		ReleaseSRWLockExclusive(&this->lock);

		return max_push;
	}

	size_t Pop(size_t count, T *buffer) {
		size_t max_pop;
		size_t first_size;
		size_t second_size;

		AcquireSRWLockExclusive(&this->lock);
		max_pop    = min(this->count, count);
		first_size = min(this->size - this->rear, max_pop);
		second_size = max_pop - first_size;

		if (first_size) {
			memcpy(buffer, this->buffer + this->rear, sizeof(T) * first_size);
		}

		if (second_size) {
			memcpy(buffer + first_size, this->buffer, sizeof(T) * second_size);
		}

		this->count -= max_pop;
		this->rear = (this->rear + max_pop) % this->size;

		ReleaseSRWLockExclusive(&this->lock);

		return max_pop;
	}
};