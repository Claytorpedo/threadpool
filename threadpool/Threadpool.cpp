#include "Threadpool.hpp"

#include <queue>
#include <optional>

class Threadpool::Sema {
public:
	void up(unsigned int n = 1) {
		{
			std::lock_guard<std::mutex> lock(mutex_);
			value_ += n;
		}
		for (unsigned int i = 0; i < n; ++i)
			cond_.notify_one();
	}
	// Wake all threads and set an optional value.
	void notifyAll(std::optional<int> newValue) {
		if (newValue) {
			std::lock_guard<std::mutex> lock(mutex_);
			value_ = *newValue;
		}
		cond_.notify_all();
	}
	void wait() {
		_wait([this]() { return value_ <= 0; });
	}
	void waitZero() {
		_wait([this]() { return value_ == 0; });
	}
	void reset() {
		std::lock_guard<std::mutex> lock(mutex_);
		value_ = 0;
	}
private:
	void _wait(std::function<bool()> waitCond) {
		std::unique_lock<std::mutex> lock(mutex_);
		cond_.wait(lock, [this, waitCond]() {
			if (waitCond())
				return false;
			--value_;
			return true;
		});
	}

	std::mutex mutex_;
	std::condition_variable cond_;
	int value_ = 0;
};

class Threadpool::JobQueue {
public:
	void push(std::unique_ptr<Job> job) {
		std::lock_guard<std::mutex> lock(mutex_);
		queue_.push(std::move(job));
	}
	std::unique_ptr<Job> getJob()
	{
		std::lock_guard<std::mutex> lock(mutex_);
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

private:
	std::queue<std::unique_ptr<Job>> queue_;
	mutable std::mutex mutex_;
};

Threadpool::Threadpool(thread_num initThreads, thread_num maxThreads, thread_num extendInc)
	: num_extend_(extendInc), max_threads_(maxThreads), is_alive_(true) {
	threads_.reserve(initThreads);
	for (thread_num i = 0; i < initThreads; ++i)
		threads_.emplace_back([this] { _thread_run(); });
}

Threadpool::~Threadpool() {
	is_alive_ = false;
	// Notify all our threads to finish all jobs.
	sema_->notifyAll(static_cast<int>(job_queue_->size() + threads_.size()));

	for (auto& thread : threads_)
		thread.join();
}

void Threadpool::waitOnAllJobs() {
	sema_->waitZero(); // Wait until there are no longer any pending jobs.
}

void Threadpool::clearPendingJobs() {
	job_queue_->clear();
	sema_->reset();
}

std::size_t Threadpool::numPendingJobs() const {
	return job_queue_->size();
}

std::size_t Threadpool::numIdleThreads() const {
	return static_cast<std::size_t>(available_threads_);
}

std::size_t Threadpool::numThreads() const {
	return threads_.size();
}

void Threadpool::_add(std::unique_ptr<Job> job) {
	job_queue_->push(std::move(job));
	sema_->up();
	if (available_threads_ == 0)
		_extend();
}

Threadpool::thread_num Threadpool::_extend() {
	if (!is_alive_ || num_extend_ <= 0)
		return 0;

	const thread_num currentSize = static_cast<thread_num>(threads_.size());
	const thread_num targetSize = max_threads_ <= 0 ? currentSize + num_extend_ : std::min(currentSize + num_extend_, max_threads_);
	const thread_num sizeIncrease = targetSize - currentSize;

	for (thread_num i = 0; i < sizeIncrease; ++i)
		threads_.emplace_back([this] { _thread_run(); });

	available_threads_ += sizeIncrease;
	return sizeIncrease;
}

void Threadpool::_thread_run() {
	while (is_alive_) {
		sema_->wait();
		std::unique_ptr<Job> job = job_queue_->getJob();
		if (!is_alive_)
			break;
		if (job) {
			--available_threads_;
			(*job)();
			++available_threads_;
		}
	}
	--available_threads_;
}
