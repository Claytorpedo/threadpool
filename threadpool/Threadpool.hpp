#pragma once

#include <cstdint>
#include <functional>
#include <future>
#include <mutex>
#include <utility>

class Threadpool {
public:
	using thread_num = int_fast16_t;
	using Job = std::function<void()>;

	static const thread_num DEFAULT_INITIAL_THREADS = 8;
	static const thread_num DEFAULT_MAX_THREADS = 0;
	static const thread_num DEFAULT_POOL_EXTEND_INCR = 4;
public:
	/* Create a new thread pool.
	   initThreads  The initial number of threads to be created.
	   maxThreads   Max number of threads that can be created (0 -> initThreads).
	   extendIncr   How many threads to extend the thread pool by when full, up to max threads.*/
	Threadpool(thread_num initThreads = DEFAULT_INITIAL_THREADS, thread_num maxThreads = DEFAULT_MAX_THREADS, thread_num extendIncr = DEFAULT_POOL_EXTEND_INCR);
	// Finishes all jobs first.
	~Threadpool();

	Threadpool(const Threadpool&) = delete;
	Threadpool(Threadpool&&) = delete;
	Threadpool& operator=(const Threadpool&) = delete;
	Threadpool& operator=(Threadpool&&) = delete;

	template<typename FuncType, typename... Args>
	auto add(FuncType&& func, Args&&... args) ->std::future<decltype(func(args...))> {
		const auto task = std::packaged_task<decltype(func(args...))>(std::bind (std::forward<FuncType>(func), std::forward<Args>(args)...));
		const auto future = task.get_future();

		_add(std::unique_ptr<Job>([task]() { task(); }));

		return future;
	}

	// Wait for all jobs to finish.
	void waitOnAllJobs();

	void clearPendingJobs();

	std::size_t numPendingJobs() const;
	std::size_t numIdleThreads() const;
	std::size_t numThreads() const;

private:
	void _add(std::unique_ptr<Job> job);
	thread_num _extend();

	void _thread_run();

	class JobQueue;
	std::unique_ptr<JobQueue> job_queue_;
	class Sema;
	std::unique_ptr<Sema> sema_; // Semaphore for worker threads.

	std::vector<std::thread> threads_;

	thread_num num_extend_;
	thread_num max_threads_;

	std::atomic<thread_num> available_threads_;
	std::atomic<bool> is_alive_;
};
