// Built-in HTTP backend for Windows, using WinHTTP synchronously on worker
// threads. WinHTTP uses the system certificate store and, where available,
// the system's automatic proxy configuration.

#include "../Transport.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winhttp.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <set>
#include <string>

namespace apex
{
namespace
{

constexpr const char* kShuttingDownError = "transport is shutting down";

// Closes a WinHTTP handle when its owning std::unique_ptr goes away.
class WinHttpHandleCloser
{
public:
	void operator()(HINTERNET handle) const
	{
		if (handle != nullptr)
		{
			WinHttpCloseHandle(handle);
		}
	}
};

using UniqueWinHttpHandle = std::unique_ptr<void, WinHttpHandleCloser>;

std::wstring ConvertUtf8ToWide(const std::string& utf8Text)
{
	if (utf8Text.empty())
	{
		return std::wstring();
	}

	const int utf8Length = static_cast<int>(utf8Text.size());
	const int wideLength = MultiByteToWideChar(CP_UTF8, 0, utf8Text.data(), utf8Length, nullptr, 0);
	std::wstring wideText(static_cast<size_t>(wideLength), L'\0');
	MultiByteToWideChar(CP_UTF8, 0, utf8Text.data(), utf8Length, &wideText[0], wideLength);
	return wideText;
}

const char* DescribeWinHttpErrorCode(DWORD errorCode)
{
	switch (errorCode)
	{
		case ERROR_WINHTTP_TIMEOUT:
			return "timed out";
		case ERROR_WINHTTP_NAME_NOT_RESOLVED:
			return "host name not resolved";
		case ERROR_WINHTTP_CANNOT_CONNECT:
			return "cannot connect";
		case ERROR_WINHTTP_CONNECTION_ERROR:
			return "connection reset";
		case ERROR_WINHTTP_SECURE_FAILURE:
			return "TLS certificate or handshake failure";
		case ERROR_WINHTTP_OPERATION_CANCELLED:
			return "cancelled";
		case ERROR_WINHTTP_UNRECOGNIZED_SCHEME:
			return "unsupported URL scheme";
		case ERROR_WINHTTP_INVALID_URL:
			return "invalid URL";
		default:
			return nullptr;
	}
}

std::string DescribeWinHttpFailure(const char* failedStep, DWORD errorCode)
{
	std::string failureMessage = std::string("WinHTTP ") + failedStep + " failed (" + std::to_string(errorCode) + ")";
	const char* errorDescription = DescribeWinHttpErrorCode(errorCode);
	if (errorDescription != nullptr)
	{
		failureMessage += ": ";
		failureMessage += errorDescription;
	}

	return failureMessage;
}

class WinHttpBackend final : public BlockingBackend
{
public:
	WinHttpBackend()
	{
#ifdef WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY
		m_session.reset(WinHttpOpen(L"apex-sdk-core", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
									WINHTTP_NO_PROXY_BYPASS, 0));
#endif
		// Automatic proxy needs Windows 8.1; fall back to the WinHTTP default.
		if (m_session == nullptr)
		{
			m_session.reset(WinHttpOpen(L"apex-sdk-core", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
										WINHTTP_NO_PROXY_BYPASS, 0));
		}

		if (m_session != nullptr)
		{
			DWORD secureProtocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
#ifdef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3
			secureProtocols |= WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
#endif
			if (!WinHttpSetOption(m_session.get(), WINHTTP_OPTION_SECURE_PROTOCOLS, &secureProtocols,
								  sizeof(secureProtocols)))
			{
				// Older systems reject TLS 1.3 in this mask; TLS 1.2 alone is enough.
				secureProtocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
				WinHttpSetOption(m_session.get(), WINHTTP_OPTION_SECURE_PROTOCOLS, &secureProtocols,
								 sizeof(secureProtocols));
			}
		}
	}

	HttpResponse Perform(const HttpRequest& request) override
	{
		HttpResponse response;
		if (m_aborted.load())
		{
			response.SetTransportError(kShuttingDownError);
			return response;
		}

		if (m_session == nullptr)
		{
			response.SetTransportError(DescribeWinHttpFailure("open", GetLastError()));
			return response;
		}

		const std::wstring wideUrl = ConvertUtf8ToWide(request.GetUrl());
		URL_COMPONENTS urlParts = {};
		urlParts.dwStructSize = sizeof(urlParts);
		urlParts.dwHostNameLength = static_cast<DWORD>(-1);
		urlParts.dwUrlPathLength = static_cast<DWORD>(-1);
		urlParts.dwExtraInfoLength = static_cast<DWORD>(-1);
		urlParts.dwSchemeLength = static_cast<DWORD>(-1);
		if (!WinHttpCrackUrl(wideUrl.c_str(), static_cast<DWORD>(wideUrl.size()), 0, &urlParts))
		{
			response.SetTransportError(DescribeWinHttpFailure("parsing the URL", GetLastError()));
			return response;
		}

		const std::wstring hostName(urlParts.lpszHostName, urlParts.dwHostNameLength);
		std::wstring pathAndQuery(urlParts.lpszUrlPath, urlParts.dwUrlPathLength);
		if (urlParts.lpszExtraInfo != nullptr)
		{
			pathAndQuery.append(urlParts.lpszExtraInfo, urlParts.dwExtraInfoLength);
		}

		if (pathAndQuery.empty())
		{
			pathAndQuery = L"/";
		}

		const bool useHttps = urlParts.nScheme == INTERNET_SCHEME_HTTPS;

		const UniqueWinHttpHandle connection(WinHttpConnect(m_session.get(), hostName.c_str(), urlParts.nPort, 0));
		if (connection == nullptr)
		{
			response.SetTransportError(DescribeWinHttpFailure("connect", GetLastError()));
			return response;
		}

		// The request handle is closed by whichever comes first: this function, or
		// AbortInFlight on another thread. TrackRequestHandle/UntrackRequestHandle
		// decide which, so it is not held by a smart pointer.
		const std::wstring wideMethod = ConvertUtf8ToWide(request.GetMethod());
		HINTERNET requestHandle =
			WinHttpOpenRequest(connection.get(), wideMethod.c_str(), pathAndQuery.c_str(), nullptr, WINHTTP_NO_REFERER,
							   WINHTTP_DEFAULT_ACCEPT_TYPES, useHttps ? WINHTTP_FLAG_SECURE : 0);
		if (requestHandle == nullptr)
		{
			response.SetTransportError(DescribeWinHttpFailure("opening the request", GetLastError()));
			return response;
		}

		if (!TrackRequestHandle(requestHandle))
		{
			WinHttpCloseHandle(requestHandle);
			response.SetTransportError(kShuttingDownError);
			return response;
		}

		SendAndReceive(request, requestHandle, response);

		if (UntrackRequestHandle(requestHandle))
		{
			WinHttpCloseHandle(requestHandle);
		}

		return response;
	}

	void AbortInFlight() override
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_aborted.store(true);
		// Closing a request handle makes a blocked synchronous call on another
		// thread return with ERROR_WINHTTP_OPERATION_CANCELLED.
		for (HINTERNET requestHandle : m_activeRequestHandles)
		{
			WinHttpCloseHandle(requestHandle);
		}

		m_activeRequestHandles.clear();
	}

private:
	void SendAndReceive(const HttpRequest& request, HINTERNET requestHandle, HttpResponse& response)
	{
		const int timeoutMilliseconds = static_cast<int>(request.GetTimeoutMilliseconds());
		WinHttpSetTimeouts(requestHandle, timeoutMilliseconds, timeoutMilliseconds, timeoutMilliseconds,
						   timeoutMilliseconds);
		// Waiting for the response headers has its own timeout, separate from
		// the per-read receive timeout above.
		DWORD responseHeadersTimeout = request.GetTimeoutMilliseconds();
		WinHttpSetOption(requestHandle, WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT, &responseHeadersTimeout,
						 sizeof(responseHeadersTimeout));

		std::wstring headerLines;
		for (const auto& [headerName, headerValue] : request.GetHeaders())
		{
			headerLines += ConvertUtf8ToWide(headerName);
			headerLines += L": ";
			headerLines += ConvertUtf8ToWide(headerValue);
			headerLines += L"\r\n";
		}

		const std::string& requestBody = request.GetBody();
		LPVOID requestBodyData = requestBody.empty() ? WINHTTP_NO_REQUEST_DATA : const_cast<char*>(requestBody.data());
		const DWORD requestBodyLength = static_cast<DWORD>(requestBody.size());
		bool succeeded =
			WinHttpSendRequest(requestHandle, headerLines.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : headerLines.c_str(),
							   headerLines.empty() ? 0 : static_cast<DWORD>(-1L), requestBodyData, requestBodyLength,
							   requestBodyLength, 0) != FALSE;
		const char* currentStep = "sending the request";
		if (succeeded)
		{
			succeeded = WinHttpReceiveResponse(requestHandle, nullptr) != FALSE;
			currentStep = "receiving the response";
		}

		if (succeeded)
		{
			DWORD statusCode = 0;
			DWORD statusCodeSize = sizeof(statusCode);
			succeeded = WinHttpQueryHeaders(requestHandle, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
											WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusCodeSize,
											WINHTTP_NO_HEADER_INDEX) != FALSE;
			currentStep = "reading the status";
			response.SetStatusCode(static_cast<int>(statusCode));
		}

		if (succeeded)
		{
			currentStep = "reading the body";
			succeeded = ReadResponseBody(requestHandle, response);
		}

		if (!succeeded)
		{
			const DWORD errorCode = GetLastError();
			response.SetStatusCode(0);
			response.SetBody(std::string());
			response.SetTransportError(m_aborted.load() ? std::string(kShuttingDownError)
														: DescribeWinHttpFailure(currentStep, errorCode));
		}
	}

	static bool ReadResponseBody(HINTERNET requestHandle, HttpResponse& response)
	{
		std::string readBuffer;
		for (;;)
		{
			DWORD availableByteCount = 0;
			if (!WinHttpQueryDataAvailable(requestHandle, &availableByteCount))
			{
				return false;
			}

			if (availableByteCount == 0)
			{
				return true;
			}

			readBuffer.resize(availableByteCount);
			DWORD readByteCount = 0;
			if (!WinHttpReadData(requestHandle, &readBuffer[0], availableByteCount, &readByteCount))
			{
				return false;
			}

			response.AppendToBody(readBuffer.data(), readByteCount);
		}
	}

	// False once shutdown began, so a request cannot start blocking after
	// AbortInFlight has already run.
	bool TrackRequestHandle(HINTERNET requestHandle)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_aborted.load())
		{
			return false;
		}

		m_activeRequestHandles.insert(requestHandle);
		return true;
	}

	// Returns true when the caller still owns the handle and must close it.
	bool UntrackRequestHandle(HINTERNET requestHandle)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_activeRequestHandles.erase(requestHandle) > 0;
	}

	UniqueWinHttpHandle m_session;
	std::mutex m_mutex;
	std::set<HINTERNET> m_activeRequestHandles;
	std::atomic<bool> m_aborted{false};
};

}  // namespace

std::unique_ptr<Transport> MakeBuiltinTransport()
{
	return std::make_unique<ThreadedTransport>(std::make_shared<WinHttpBackend>());
}

const char* BuiltinTransportName()
{
	return "winhttp";
}

}  // namespace apex
