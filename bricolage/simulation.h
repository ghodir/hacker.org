#ifndef __SIMULATION_H__
#define __SIMULATION_H__

#include "common.h"

#include <list>
#include <thread>
#include "state.h"
#include "mutex.h"
#include "heap.h"

class simulation {
private:
	mutex heap_lock;
	heap<state*> heap;
	std::list<std::thread> threads;

	size_t samples;
	size_t possibilities;
	const state* result;
	volatile bool running;
	void thread();
	void resolve(const state* state);
	static void thread_wrapper(simulation* pContext);

public:
	simulation(const board& board);
	~simulation();
	void run(std::string& path);
};


#endif