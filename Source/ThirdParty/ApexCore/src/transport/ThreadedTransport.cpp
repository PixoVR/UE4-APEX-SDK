#include "../Transport.hpp"

#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

namespace apex
{
namespace
{

// How long destruction waits for workers after asking the backend to abort.
constexpr std::chrono::milliseconds kShutdownGracePeriod{1000};
constexpr const char* kShuttingDownError = "transport is shutting down";

// A request waiting for a worker, with the completion that receives its response.
class QueuedRequest
{
public:
	QueuedRequest() = default;
	QueuedRequest(HttpRequest request, HttpCompletion completion)
		: m_request(std::move(request)), m_completion(std::move(completion))
	{
	}

	const HttpRequest& GetRequest() const
	{
		return m_request;
	}

	void Complete(HttpResponse response)
	{
		m_completion(m_request.GetRequestId(), std::move(response));
	}

	void CompleteAsShuttingDown()
	{
		HttpResponse shuttingDownResponse;
		shuttingDownResponse.SetTransportError(kShuttingDownError);
		Complete(std::move(shuttingDownResponse));
	}

private:
	HttpRequest m_request;
	HttpCompletion m_completion;
};

}  // namespace

// The queue, the worker threads and the backend. Each worker holds the pool,
// and through it the backend, so a detached worker can outlive the transport.
class ThreadedTransport::WorkerPool
{
public:
	WorkerPool(std::shared_ptr<BlockingBackend> backend, int workerCount)
		: m_backend(std::move(backend)), m_workerCount(workerCount < 1 ? 1 : workerCount)
	{
	}

	BlockingBackend& GetBackend()
	{
		return *m_backend;
	}

	// Queues the request and starts the workers on first use. Returns false,
	// leaving the request with the caller, once shutdown has begun.
	bool TryEnqueue(QueuedRequest& queuedRequest, const std::shared_ptr<WorkerPool>& self)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_stopping)
		{
			return false;
		}
		while (static_cast<int>(m_workerThreads.size()) < m_workerCount)
		{
			m_workerThreads.emplace_back(RunWorker, self);
			++m_runningWorkerCount;  // only once the thread exists
		}

		m_queue.push_back(std::move(queuedRequest));
		m_queueChanged.notify_one();
		return true;
	}

	// Blocks until a request is queued or shutdown begins. Returns false on shutdown.
	bool WaitForNextRequest(QueuedRequest& nextRequest)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_queueChanged.wait(lock, [this]
		{
			return m_stopping || !m_queue.empty();
		});
		if (m_stopping)
		{
			return false;
		}

		nextRequest = std::move(m_queue.front());
		m_queue.pop_front();
		return true;
	}

	// Stops accepting work and hands back the requests and threads still held.
	void BeginShutdown(std::deque<QueuedRequest>& abandonedRequests, std::vector<std::thread>& workerThreads)
	{
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_stopping = true;
			abandonedRequests.swap(m_queue);
			workerThreads.swap(m_workerThreads);
		}
		m_queueChanged.notify_all();
	}

	bool WaitForWorkersToExit(std::chrono::milliseconds timeout)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		return m_workerExited.wait_for(lock, timeout, [this]
		{
			return m_runningWorkerCount == 0;
		});
	}

private:
	static void RunWorker(std::shared_ptr<WorkerPool> workerPool)
	{
		workerPool->GetBackend().OnWorkerStart();
		QueuedRequest nextRequest;
		while (workerPool->WaitForNextRequest(nextRequest))
		{
			nextRequest.Complete(workerPool->GetBackend().Perform(nextRequest.GetRequest()));
		}

		workerPool->GetBackend().OnWorkerStop();
		workerPool->OnWorkerExited();
	}

	void OnWorkerExited()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		--m_runningWorkerCount;
		m_workerExited.notify_all();
	}

	std::shared_ptr<BlockingBackend> m_backend;
	const int m_workerCount;
	std::mutex m_mutex;
	std::condition_variable m_queueChanged;
	std::condition_variable m_workerExited;
	std::deque<QueuedRequest> m_queue;
	std::vector<std::thread> m_workerThreads;
	int m_runningWorkerCount = 0;
	bool m_stopping = false;
};

ThreadedTransport::ThreadedTransport(std::shared_ptr<BlockingBackend> backend, int workerCount)
	: m_workerPool(std::make_shared<WorkerPool>(std::move(backend), workerCount))
{
}

void ThreadedTransport::Send(HttpRequest request, HttpCompletion completion)
{
	QueuedRequest queuedRequest(std::move(request), std::move(completion));
	if (!m_workerPool->TryEnqueue(queuedRequest, m_workerPool))
	{
		queuedRequest.CompleteAsShuttingDown();
	}
}

ThreadedTransport::~ThreadedTransport()
{
	std::deque<QueuedRequest> abandonedRequests;
	std::vector<std::thread> workerThreads;
	m_workerPool->BeginShutdown(abandonedRequests, workerThreads);
	m_workerPool->GetBackend().AbortInFlight();

	const bool allWorkersExited = m_workerPool->WaitForWorkersToExit(kShutdownGracePeriod);
	for (std::thread& workerThread : workerThreads)
	{
		if (allWorkersExited)
		{
			workerThread.join();
		}
		else
		{
			workerThread.detach();
		}
	}

	for (QueuedRequest& abandonedRequest : abandonedRequests)
	{
		abandonedRequest.CompleteAsShuttingDown();
	}
}

}  // namespace apex
