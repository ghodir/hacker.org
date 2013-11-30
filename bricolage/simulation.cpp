#include "simulation.h"
#include <iostream>
#include <windows.h>
#include <process.h>
#include <algorithm>
#include "stats.h"
#include "monitor.h"

#include <stack>

template<class T>
class StackSize : public Variable {
	std::stack<T>* stack;
public:
	StackSize(std::stack<T>* stack) {
		this->stack = stack;
	}

	std::string get() {
		std::stringstream stream;
		stream << stack->size();
		return stream.str();
	}
};

Simulation::Simulation(Board& board):
	heap(4096){
	samples = 0;
	possibilities = 0;

	board._areas = board.areas();
	heap.insert(new state(board) );
}

Simulation::~Simulation() {
	//StatsManager::Get().RemoveVar("stack");
}

void Simulation::resolve(std::list<std::pair<int, int>> path) {
	if ( running ) 
		this->path = path;
	running = false;
}

void Simulation::thread(Simulation* self) {
	try {
		state* state;
		while (self->running) {
			self->stack_lock.lock();
			//monitor::_emit(monitor::event("stack") << self->stack.size());

			if (self->heap.empty()) {
				self->stack_lock.unlock();
				Sleep(1);
				continue;
			}
			else {
				self->heap.pop(state);
				self->samples++;
				self->possibilities += static_cast<unsigned long long>(state->board._areas.size());
				self->stack_lock.unlock();
			}

#ifdef _DEBUG
			//monitor::_emit(monitor::event("state") << state.board.width() << state.board.height() << state.board.serialize() << std::thread::id() << state.path());
#endif
			if (state->board._areas.size() == 0 && !state->board.empty())
				continue; // 

			for (auto it = state->board._areas.begin(); it != state->board._areas.end(); it++) {
				if (it->size() < 3)
					continue;

				::state* test = new ::state(*state);
				test->click(*it);
				test->board._areas = test->board.areas();

				if (test->board.empty()) {
					// SOLUTION
					self->resolve(test->clicks);
					delete test;
					return;
				}
				else {
					self->stack_lock.lock();
					self->heap.insert(test);
					self->stack_lock.unlock();
				}
			}

			delete state;
		}
	}
	catch (std::exception e) {
		std::cerr << "Uncaught exception in thread: " << e.what() << std::endl;
		_CrtDumpMemoryLeaks();
		terminate();
	}
}

std::list<std::pair<int,int>> Simulation::run() {
	running = true;
	timer t;
	for (unsigned int i = 0; i < std::thread::hardware_concurrency(); i++)
		threads.push_back(std::thread(thread, this));

	for (auto it = threads.begin(); it != threads.end(); it++)
		it->join();

	monitor::_emit(monitor::event("samples") << samples << t.get());
	monitor::_emit(monitor::event("possibilities") << possibilities << t.get());

	return path;
}


std::list<Board> Simulation::run(Board& b, std::list<std::pair<int,int>> path) {
	state state(b);
	std::cout << (std::string)state.board << std::endl;
	for( auto click: path) {
		state.click(click);
		std::cout << (std::string)state.board << std::endl;
	}

	return std::list<Board>();
#if 0
	while(!queue.empty()) {
		State state = (State)queue.front();
		queue.pop();

		auto areas = state.board.areas();

		// fetch all available areas from board
		// if len(areas) == 0 and not empty:
		//	invalid state -> don't process
		//  continue
		//
		if( areas.size() == 0 && !state.board.empty())
			continue; // 
			
		// for each area
		//		simulate click on area
		//		if empty:
		//			SOLUTION FOUND!!!
		//          return its path
		//		else:
		//			append to queue
		//
		for(auto it = areas.begin(); it != areas.end(); it++) {
			if( it->size() < 3)
				continue;

			State test = State(state);
			test.click(*it);
			std::cout << (std::string)test.board << std::endl;

			if( test.board.empty() ) {
				// SOLUTION
				return test.clicks;
			} else {
				queue.push(test);
			}
		}
	}

	throw std::logic_error("Invalid State");
#endif
}