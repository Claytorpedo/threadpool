#include "Threadpool.hpp"

#include <queue>

class Threadpool::JobQueue {
public:
	void push(std::unique_ptr<Job> job) {
		std::lock_guard<std::mutex> lock(mutex_);
		queue_.push(std::move(job));
	}
	std::unique_ptr<Job> getJob() {
		std::lock_guard<std::mutex> latch(mutex_);
		if (queue_.empty())
			return nullptr;
		std::unique_ptr<Job> job = std::move(queue_.front());
		queue_.pop();
		return job;
	}
	void clear() {
		std::lock_guard<std::mutex> lock(mutex_);
		std::queue<std::unique_ptr<Job>>().swap(queue_);
	}

	std::size_t size() const {
		std::lock_guard<std::mutex> lock(mutex_);
		return queue_.size();
	}

	bool empty() const {
		std::lock_guard<std::mutex> lock(mutex_);
		return queue_.empty();
	}

private:
	std::queue<std::unique_ptr<Job>> queue_;
	mutable std::mutex mutex_;
};

Threadpool::Threadpool(thread_num initThreads, thread_num maxThreads, thread_num extendInc)
	: job_queue_(std::make_unique<JobQueue>()), num_extend_(extendInc), max_threads_(maxThreads), should_finish_(false)
{
	threads_.reserve(initThreads);
	for (thread_num i = 0; i < initThreads; ++i)
		threads_.emplace_back([this] { _thread_run(); });
}

Threadpool::~Threadpool() {
	should_finish_ = true;
	task_cond_.notify_all();

	for (auto& thread : threads_)
		thread.join();
}

void Threadpool::waitOnAllJobs() {
	std::unique_lock<std::mutex> latch(mutex_);
	finished_all_jobs_cond_.wait(latch, [this]() {
		return job_queue_->empty() && working_threads_ == 0;
	});
}

void Threadpool::clearPendingJobs() {
	job_queue_->clear();
}

std::size_t Threadpool::numPendingJobs() const {
	return job_queue_->size();
}

std::size_t Threadpool::numIdleThreads() const {
	return threads_.size() - static_cast<std::size_t>(working_threads_);
}

std::size_t Threadpool::numThreads() const {
	return threads_.size();
}

void Threadpool::_add(std::unique_ptr<Job> job) {
	job_queue_->push(std::move(job));
	task_cond_.notify_one();
	if (working_threads_ == static_cast<thread_num>(threads_.size()))
		_extend();
}

Threadpool::thread_num Threadpool::_extend() {
	if (should_finish_ || num_extend_ <= 0)
		return 0;

	const thread_num currentSize = static_cast<thread_num>(threads_.size());
	const thread_num targetSize = max_threads_ <= 0 ? currentSize + num_extend_ : std::min(currentSize + num_extend_, max_threads_);
	const thread_num sizeIncrease = targetSize - currentSize;

	for (thread_num i = 0; i < sizeIncrease; ++i)
		threads_.emplace_back([this] { _thread_run(); });

	return sizeIncrease;
}

void Threadpool::_thread_run() {
	while (true) {
		std::unique_lock<std::mutex> latch(mutex_);
		task_cond_.wait(latch, [this] {
			return should_finish_ || !job_queue_->empty();
		});
		if (job_queue_->empty() && should_finish_)
			return;
		std::unique_ptr<Job> job = job_queue_->getJob();
		++working_threads_;
		latch.unlock();

		if (job)
			(*job)();

		latch.lock();
		--working_threads_;
		latch.unlock();
		finished_all_jobs_cond_.notify_one();
	}
}
