#pragma once

#include <cstdint>
#include <functional>
#include <future>
#include <mutex>
#include <utility>
#include <type_traits>
#include <vector>

class Threadpool {
public:
	using thread_num = int_fast16_t;

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
	auto add(FuncType&& func, Args&&... args) ->std::future<std::invoke_result_t<FuncType&&, Args&&...>> {
		auto task = std::packaged_task<std::invoke_result_t<FuncType&&, Args&&...>()>(std::bind(std::forward<FuncType>(func), std::forward<Args>(args)...));
		auto future = task.get_future();

		_add(std::make_unique<PackagedJob<std::invoke_result_t<FuncType&&, Args&&...>()>>(std::move(task)));

		return future;
	}

	// Wait for all current jobs to finish.
	void waitOnAllJobs();

	void clearPendingJobs();

	std::size_t numPendingJobs() const;
	std::size_t numIdleThreads() const;
	std::size_t numThreads() const;

private:

	struct Job {
		virtual ~Job() = default;
		virtual void operator()() = 0;
		Job() = default;
		Job(const Job&) = delete;
		Job& operator=(const Job&) = delete;
	};

	template <typename FuncType, typename... Args>
	struct PackagedJob : public Job {
		explicit PackagedJob(std::packaged_task<std::invoke_result_t<FuncType&&, Args&&...>()>&& task) : task_(std::move(task)) {}
		~PackagedJob() override = default;
		inline void operator()() override { task_(); }
	private:
		std::packaged_task <std::invoke_result_t<FuncType&&, Args&&...>()> task_;
	};

	void _add(std::unique_ptr<Job> job);
	thread_num _extend();

	void _thread_run();

	class JobQueue;
	std::unique_ptr<JobQueue> job_queue_;

	std::vector<std::thread> threads_;

	thread_num num_extend_;
	thread_num max_threads_;

	std::mutex mutex_;
	std::condition_variable finished_all_jobs_cond_;
	std::condition_variable task_cond_;

	thread_num working_threads_;
	std::atomic<bool> should_finish_;
};
