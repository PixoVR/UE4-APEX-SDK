#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace apex
{

using HttpHeaders = std::vector<std::pair<std::string, std::string>>;

class HttpRequest
{
public:
	// Unique per client; matches the request to its completion.
	uint64_t GetRequestId() const
	{
		return m_requestId;
	}
	void SetRequestId(uint64_t requestId)
	{
		m_requestId = requestId;
	}

	// "GET" or "POST".
	const std::string& GetMethod() const
	{
		return m_method;
	}
	void SetMethod(std::string method)
	{
		m_method = std::move(method);
	}

	const std::string& GetUrl() const
	{
		return m_url;
	}
	void SetUrl(std::string url)
	{
		m_url = std::move(url);
	}

	const HttpHeaders& GetHeaders() const
	{
		return m_headers;
	}
	void AddHeader(std::string headerName, std::string headerValue)
	{
		m_headers.emplace_back(std::move(headerName), std::move(headerValue));
	}

	const std::string& GetBody() const
	{
		return m_body;
	}
	void SetBody(std::string body)
	{
		m_body = std::move(body);
	}

	uint32_t GetTimeoutMilliseconds() const
	{
		return m_timeoutMilliseconds;
	}
	void SetTimeoutMilliseconds(uint32_t timeoutMilliseconds)
	{
		m_timeoutMilliseconds = timeoutMilliseconds;
	}

private:
	uint64_t m_requestId = 0;
	std::string m_method;
	std::string m_url;
	HttpHeaders m_headers;
	std::string m_body;
	uint32_t m_timeoutMilliseconds = 30000;
};

class HttpResponse
{
public:
	// 0 when no response arrived.
	int GetStatusCode() const
	{
		return m_statusCode;
	}
	void SetStatusCode(int statusCode)
	{
		m_statusCode = statusCode;
	}

	const std::string& GetBody() const
	{
		return m_body;
	}
	void SetBody(std::string body)
	{
		m_body = std::move(body);
	}
	void AppendToBody(const char* bodyBytes, size_t bodyByteCount)
	{
		m_body.append(bodyBytes, bodyByteCount);
	}

	// Non-empty when no response arrived: why the request failed.
	const std::string& GetTransportError() const
	{
		return m_transportError;
	}
	void SetTransportError(std::string transportError)
	{
		m_transportError = std::move(transportError);
	}

private:
	int m_statusCode = 0;
	std::string m_body;
	std::string m_transportError;
};

// Called exactly once per request, from any thread.
using HttpCompletion = std::function<void(uint64_t requestId, HttpResponse response)>;

class Transport
{
public:
	virtual ~Transport() = default;
	// Starts the request and returns at once. The completion may run on any
	// thread, possibly before send returns.
	virtual void Send(HttpRequest request, HttpCompletion completion) = 0;
};

// The platform's built-in backend, or nullptr when none is compiled in.
std::unique_ptr<Transport> MakeBuiltinTransport();
const char* BuiltinTransportName();

// A backend built on a blocking OS HTTP API. ThreadedTransport runs it on
// worker threads.
class BlockingBackend
{
public:
	virtual ~BlockingBackend() = default;
	// Runs on a worker thread. Must not throw.
	virtual HttpResponse Perform(const HttpRequest& request) = 0;
	// Run on each worker thread as it starts and stops (JNI attach/detach).
	virtual void OnWorkerStart()
	{
	}
	virtual void OnWorkerStop()
	{
	}
	// Called from the destroying thread so blocked perform() calls return
	// early where the OS allows it.
	virtual void AbortInFlight()
	{
	}
};

// Runs a BlockingBackend on a small pool of worker threads. Workers share
// ownership of the backend, so destruction never waits long for a request the
// OS cannot interrupt: after a short grace period, remaining workers are
// detached and finish on their own (their completions still run).
class ThreadedTransport final : public Transport
{
public:
	explicit ThreadedTransport(std::shared_ptr<BlockingBackend> backend, int workerCount = 2);
	~ThreadedTransport() override;

	void Send(HttpRequest request, HttpCompletion completion) override;

	class WorkerPool;  // defined in ThreadedTransport.cpp

private:
	std::shared_ptr<WorkerPool> m_workerPool;
};

}  // namespace apex
