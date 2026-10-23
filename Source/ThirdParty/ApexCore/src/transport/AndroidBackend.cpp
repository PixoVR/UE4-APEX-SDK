// Built-in HTTP backend for Android, calling java.net.HttpURLConnection through
// JNI on worker threads. HttpURLConnection uses the system trust store and the
// device's proxy settings. Only java.* classes are used, so the same code also
// runs in a desktop JVM, which is how the tests exercise it.

#include "../Transport.hpp"
#include "apex/Apex.h"

#include <jni.h>

#include <atomic>
#include <mutex>
#include <set>
#include <string>

namespace apex
{
namespace
{

constexpr const char* kShuttingDownError = "transport is shutting down";
constexpr jint kResponseReadChunkSize = 16 * 1024;
constexpr jint kLocalReferenceCapacity = 32;

// Set by JNI_OnLoad or ApexAndroidSetJavaVm; read by every worker.
std::atomic<JavaVM*> javaVirtualMachine{nullptr};

// The JNI headers in the Android NDK and the JDK disagree on the type of
// AttachCurrentThread's first parameter.
#if defined(__ANDROID__)
using AttachEnvironmentPointer = JNIEnv**;
#else
using AttachEnvironmentPointer = void**;
#endif

// The java.net and java.io calls the backend makes, resolved once. Every method
// leaves a thrown Java exception pending for the caller to collect.
class JavaHttpApi
{
public:
	// Looks up the classes and methods. Returns false when any is missing.
	bool Resolve(JNIEnv* jniEnvironment)
	{
		m_urlClass = MakeGlobalClass(jniEnvironment, "java/net/URL");
		m_httpConnectionClass = MakeGlobalClass(jniEnvironment, "java/net/HttpURLConnection");
		m_outputStreamClass = MakeGlobalClass(jniEnvironment, "java/io/OutputStream");
		m_inputStreamClass = MakeGlobalClass(jniEnvironment, "java/io/InputStream");
		m_stringClass = MakeGlobalClass(jniEnvironment, "java/lang/String");
		m_objectClass = MakeGlobalClass(jniEnvironment, "java/lang/Object");
		if (m_urlClass == nullptr || m_httpConnectionClass == nullptr || m_outputStreamClass == nullptr ||
			m_inputStreamClass == nullptr || m_stringClass == nullptr || m_objectClass == nullptr)
		{
			return false;
		}

		MethodResolver resolver(jniEnvironment);
		m_urlConstructor = resolver.Find(m_urlClass, "<init>", "(Ljava/lang/String;)V");
		m_openConnection = resolver.Find(m_urlClass, "openConnection", "()Ljava/net/URLConnection;");
		m_setConnectTimeout = resolver.Find(m_httpConnectionClass, "setConnectTimeout", "(I)V");
		m_setReadTimeout = resolver.Find(m_httpConnectionClass, "setReadTimeout", "(I)V");
		m_setRequestMethod = resolver.Find(m_httpConnectionClass, "setRequestMethod", "(Ljava/lang/String;)V");
		m_setRequestProperty =
			resolver.Find(m_httpConnectionClass, "setRequestProperty", "(Ljava/lang/String;Ljava/lang/String;)V");
		m_setDoOutput = resolver.Find(m_httpConnectionClass, "setDoOutput", "(Z)V");
		m_setFixedLengthStreamingMode = resolver.Find(m_httpConnectionClass, "setFixedLengthStreamingMode", "(I)V");
		m_setInstanceFollowRedirects = resolver.Find(m_httpConnectionClass, "setInstanceFollowRedirects", "(Z)V");
		m_setUseCaches = resolver.Find(m_httpConnectionClass, "setUseCaches", "(Z)V");
		m_getOutputStream = resolver.Find(m_httpConnectionClass, "getOutputStream", "()Ljava/io/OutputStream;");
		m_getResponseCode = resolver.Find(m_httpConnectionClass, "getResponseCode", "()I");
		m_getInputStream = resolver.Find(m_httpConnectionClass, "getInputStream", "()Ljava/io/InputStream;");
		m_getErrorStream = resolver.Find(m_httpConnectionClass, "getErrorStream", "()Ljava/io/InputStream;");
		m_disconnect = resolver.Find(m_httpConnectionClass, "disconnect", "()V");
		m_outputStreamWrite = resolver.Find(m_outputStreamClass, "write", "([B)V");
		m_outputStreamClose = resolver.Find(m_outputStreamClass, "close", "()V");
		m_inputStreamRead = resolver.Find(m_inputStreamClass, "read", "([B)I");
		m_inputStreamClose = resolver.Find(m_inputStreamClass, "close", "()V");
		m_stringFromBytes = resolver.Find(m_stringClass, "<init>", "([BLjava/lang/String;)V");
		m_objectToString = resolver.Find(m_objectClass, "toString", "()Ljava/lang/String;");
		return resolver.AllFound();
	}

	// Builds a java.lang.String from UTF-8 bytes. NewStringUTF expects modified
	// UTF-8, which differs for characters outside the BMP and for NUL.
	jstring MakeJavaString(JNIEnv* jniEnvironment, const std::string& utf8Text) const
	{
		jbyteArray utf8Bytes = MakeByteArray(jniEnvironment, utf8Text);
		if (utf8Bytes == nullptr)
		{
			return nullptr;
		}

		jstring charsetName = jniEnvironment->NewStringUTF("UTF-8");
		jstring javaString =
			static_cast<jstring>(jniEnvironment->NewObject(m_stringClass, m_stringFromBytes, utf8Bytes, charsetName));
		jniEnvironment->DeleteLocalRef(charsetName);
		jniEnvironment->DeleteLocalRef(utf8Bytes);
		return javaString;
	}

	// An HttpURLConnection for the URL, or nullptr (with or without a pending
	// exception) when the URL does not open one.
	jobject OpenHttpConnection(JNIEnv* jniEnvironment, const std::string& url) const
	{
		jstring javaUrlText = MakeJavaString(jniEnvironment, url);
		if (jniEnvironment->ExceptionCheck())
		{
			return nullptr;
		}

		jobject javaUrl = jniEnvironment->NewObject(m_urlClass, m_urlConstructor, javaUrlText);
		if (jniEnvironment->ExceptionCheck())
		{
			return nullptr;
		}

		jobject connection = jniEnvironment->CallObjectMethod(javaUrl, m_openConnection);
		if (jniEnvironment->ExceptionCheck())
		{
			return nullptr;
		}

		// IsInstanceOf is true for null, so check that separately.
		if (connection == nullptr || !jniEnvironment->IsInstanceOf(connection, m_httpConnectionClass))
		{
			return nullptr;
		}

		return connection;
	}

	void Configure(JNIEnv* jniEnvironment, jobject connection, jint timeoutMilliseconds) const
	{
		jniEnvironment->CallVoidMethod(connection, m_setConnectTimeout, timeoutMilliseconds);
		if (jniEnvironment->ExceptionCheck())
		{
			return;
		}

		jniEnvironment->CallVoidMethod(connection, m_setReadTimeout, timeoutMilliseconds);
		if (jniEnvironment->ExceptionCheck())
		{
			return;
		}

		jniEnvironment->CallVoidMethod(connection, m_setUseCaches, JNI_FALSE);
		if (jniEnvironment->ExceptionCheck())
		{
			return;
		}

		jniEnvironment->CallVoidMethod(connection, m_setInstanceFollowRedirects, JNI_TRUE);
	}

	void SetRequestMethod(JNIEnv* jniEnvironment, jobject connection, const std::string& method) const
	{
		jstring javaMethod = MakeJavaString(jniEnvironment, method);
		if (jniEnvironment->ExceptionCheck())
		{
			return;
		}

		jniEnvironment->CallVoidMethod(connection, m_setRequestMethod, javaMethod);
		jniEnvironment->DeleteLocalRef(javaMethod);
	}

	void SetRequestHeader(JNIEnv* jniEnvironment, jobject connection, const std::string& headerName,
						  const std::string& headerValue) const
	{
		jstring javaHeaderName = MakeJavaString(jniEnvironment, headerName);
		if (jniEnvironment->ExceptionCheck())
		{
			return;
		}

		jstring javaHeaderValue = MakeJavaString(jniEnvironment, headerValue);
		if (jniEnvironment->ExceptionCheck())
		{
			return;
		}

		jniEnvironment->CallVoidMethod(connection, m_setRequestProperty, javaHeaderName, javaHeaderValue);
		jniEnvironment->DeleteLocalRef(javaHeaderName);
		jniEnvironment->DeleteLocalRef(javaHeaderValue);
	}

	void WriteRequestBody(JNIEnv* jniEnvironment, jobject connection, const std::string& requestBody) const
	{
		jniEnvironment->CallVoidMethod(connection, m_setDoOutput, JNI_TRUE);
		if (jniEnvironment->ExceptionCheck())
		{
			return;
		}

		jniEnvironment->CallVoidMethod(connection, m_setFixedLengthStreamingMode,
									   static_cast<jint>(requestBody.size()));
		if (jniEnvironment->ExceptionCheck())
		{
			return;
		}

		jbyteArray bodyBytes = MakeByteArray(jniEnvironment, requestBody);
		if (jniEnvironment->ExceptionCheck())
		{
			return;
		}

		jobject outputStream = jniEnvironment->CallObjectMethod(connection, m_getOutputStream);
		if (jniEnvironment->ExceptionCheck())
		{
			return;
		}

		jniEnvironment->CallVoidMethod(outputStream, m_outputStreamWrite, bodyBytes);
		if (jniEnvironment->ExceptionCheck())
		{
			return;
		}

		jniEnvironment->CallVoidMethod(outputStream, m_outputStreamClose);
		jniEnvironment->DeleteLocalRef(bodyBytes);
		jniEnvironment->DeleteLocalRef(outputStream);
	}

	jint GetResponseCode(JNIEnv* jniEnvironment, jobject connection) const
	{
		return jniEnvironment->CallIntMethod(connection, m_getResponseCode);
	}

	// The body stream for statusCode, or nullptr when there is none. Never leaves
	// an exception pending: getInputStream throws for some error statuses, and
	// the status alone is then the whole response.
	jobject OpenResponseStream(JNIEnv* jniEnvironment, jobject connection, jint statusCode) const
	{
		jobject responseStream =
			jniEnvironment->CallObjectMethod(connection, statusCode >= 400 ? m_getErrorStream : m_getInputStream);
		if (jniEnvironment->ExceptionCheck())
		{
			jniEnvironment->ExceptionClear();
			return nullptr;
		}

		return responseStream;
	}

	// Appends the whole stream to response's body.
	void ReadStreamIntoBody(JNIEnv* jniEnvironment, jobject responseStream, HttpResponse& response) const
	{
		jbyteArray readChunk = jniEnvironment->NewByteArray(kResponseReadChunkSize);
		if (jniEnvironment->ExceptionCheck())
		{
			return;
		}

		std::string chunkBytes;
		for (;;)
		{
			const jint readByteCount = jniEnvironment->CallIntMethod(responseStream, m_inputStreamRead, readChunk);
			if (jniEnvironment->ExceptionCheck() || readByteCount < 0)
			{
				break;
			}

			chunkBytes.resize(static_cast<size_t>(readByteCount));
			jniEnvironment->GetByteArrayRegion(readChunk, 0, readByteCount, reinterpret_cast<jbyte*>(&chunkBytes[0]));
			response.AppendToBody(chunkBytes.data(), chunkBytes.size());
		}

		jniEnvironment->DeleteLocalRef(readChunk);
	}

	// Closes a stream or disconnects a connection, ignoring any exception.
	void CloseQuietly(JNIEnv* jniEnvironment, jobject streamOrConnection, bool isConnection) const
	{
		jniEnvironment->CallVoidMethod(streamOrConnection, isConnection ? m_disconnect : m_inputStreamClose);
		if (jniEnvironment->ExceptionCheck())
		{
			jniEnvironment->ExceptionClear();
		}
	}

	// Clears the pending Java exception and returns its description.
	std::string TakePendingException(JNIEnv* jniEnvironment) const
	{
		jthrowable pendingException = jniEnvironment->ExceptionOccurred();
		jniEnvironment->ExceptionClear();
		if (pendingException == nullptr)
		{
			return "unknown Java exception";
		}

		std::string exceptionDescription = "Java exception";
		jstring javaDescription =
			static_cast<jstring>(jniEnvironment->CallObjectMethod(pendingException, m_objectToString));
		if (jniEnvironment->ExceptionCheck())
		{
			jniEnvironment->ExceptionClear();
		}
		else if (javaDescription != nullptr)
		{
			const char* descriptionCharacters = jniEnvironment->GetStringUTFChars(javaDescription, nullptr);
			if (descriptionCharacters != nullptr)
			{
				exceptionDescription = descriptionCharacters;
				jniEnvironment->ReleaseStringUTFChars(javaDescription, descriptionCharacters);
			}

			jniEnvironment->DeleteLocalRef(javaDescription);
		}

		jniEnvironment->DeleteLocalRef(pendingException);
		return exceptionDescription;
	}

private:
	// Looks methods up one at a time, stopping at the first failure: JNI must
	// not be called with an exception pending.
	class MethodResolver
	{
	public:
		explicit MethodResolver(JNIEnv* jniEnvironment) : m_jniEnvironment(jniEnvironment)
		{
		}

		jmethodID Find(jclass javaClass, const char* methodName, const char* methodSignature)
		{
			if (!m_allFound)
			{
				return nullptr;
			}

			jmethodID methodId = m_jniEnvironment->GetMethodID(javaClass, methodName, methodSignature);
			if (methodId == nullptr || m_jniEnvironment->ExceptionCheck())
			{
				m_jniEnvironment->ExceptionClear();
				m_allFound = false;
			}

			return methodId;
		}

		bool AllFound() const
		{
			return m_allFound;
		}

	private:
		JNIEnv* m_jniEnvironment;
		bool m_allFound = true;
	};

	static jclass MakeGlobalClass(JNIEnv* jniEnvironment, const char* className)
	{
		jclass localClass = jniEnvironment->FindClass(className);
		if (localClass == nullptr)
		{
			jniEnvironment->ExceptionClear();
			return nullptr;
		}

		jclass globalClass = static_cast<jclass>(jniEnvironment->NewGlobalRef(localClass));
		jniEnvironment->DeleteLocalRef(localClass);
		return globalClass;
	}

	static jbyteArray MakeByteArray(JNIEnv* jniEnvironment, const std::string& bytes)
	{
		jbyteArray byteArray = jniEnvironment->NewByteArray(static_cast<jsize>(bytes.size()));
		if (byteArray == nullptr)
		{
			return nullptr;
		}

		jniEnvironment->SetByteArrayRegion(byteArray, 0, static_cast<jsize>(bytes.size()),
										   reinterpret_cast<const jbyte*>(bytes.data()));
		return byteArray;
	}

	jclass m_urlClass = nullptr;
	jclass m_httpConnectionClass = nullptr;
	jclass m_outputStreamClass = nullptr;
	jclass m_inputStreamClass = nullptr;
	jclass m_stringClass = nullptr;
	jclass m_objectClass = nullptr;
	jmethodID m_urlConstructor = nullptr;
	jmethodID m_openConnection = nullptr;
	jmethodID m_setConnectTimeout = nullptr;
	jmethodID m_setReadTimeout = nullptr;
	jmethodID m_setRequestMethod = nullptr;
	jmethodID m_setRequestProperty = nullptr;
	jmethodID m_setDoOutput = nullptr;
	jmethodID m_setFixedLengthStreamingMode = nullptr;
	jmethodID m_setInstanceFollowRedirects = nullptr;
	jmethodID m_setUseCaches = nullptr;
	jmethodID m_getOutputStream = nullptr;
	jmethodID m_getResponseCode = nullptr;
	jmethodID m_getInputStream = nullptr;
	jmethodID m_getErrorStream = nullptr;
	jmethodID m_disconnect = nullptr;
	jmethodID m_outputStreamWrite = nullptr;
	jmethodID m_outputStreamClose = nullptr;
	jmethodID m_inputStreamRead = nullptr;
	jmethodID m_inputStreamClose = nullptr;
	jmethodID m_stringFromBytes = nullptr;
	jmethodID m_objectToString = nullptr;
};

// Resolved once, on the first worker that makes a request.
JavaHttpApi javaHttpApi;
bool javaHttpApiResolved = false;
std::once_flag javaHttpApiResolveOnce;

// Each worker thread's JNI environment, and whether the backend attached the
// thread (and so must detach it).
thread_local JNIEnv* workerJniEnvironment = nullptr;
thread_local bool workerAttachedByBackend = false;

class AndroidBackend final : public BlockingBackend
{
public:
	void OnWorkerStart() override
	{
		JavaVM* virtualMachine = javaVirtualMachine.load();
		if (virtualMachine == nullptr)
		{
			return;
		}

		JNIEnv* jniEnvironment = nullptr;
		if (virtualMachine->GetEnv(reinterpret_cast<void**>(&jniEnvironment), JNI_VERSION_1_6) == JNI_OK)
		{
			workerJniEnvironment = jniEnvironment;
			return;
		}

		JavaVMAttachArgs attachArguments = {JNI_VERSION_1_6, const_cast<char*>("apex-http"), nullptr};
		if (virtualMachine->AttachCurrentThread(reinterpret_cast<AttachEnvironmentPointer>(&jniEnvironment),
												&attachArguments) == JNI_OK)
		{
			workerJniEnvironment = jniEnvironment;
			workerAttachedByBackend = true;
		}
	}

	void OnWorkerStop() override
	{
		if (workerAttachedByBackend)
		{
			javaVirtualMachine.load()->DetachCurrentThread();
		}

		workerJniEnvironment = nullptr;
		workerAttachedByBackend = false;
	}

	HttpResponse Perform(const HttpRequest& request) override
	{
		HttpResponse response;
		if (workerJniEnvironment == nullptr)
		{
			OnWorkerStart();  // the VM may have been set after this worker started
		}

		JNIEnv* jniEnvironment = workerJniEnvironment;
		if (jniEnvironment == nullptr)
		{
			response.SetTransportError("no Java VM: load the core as a shared library or call ApexAndroidSetJavaVm");
			return response;
		}

		std::call_once(javaHttpApiResolveOnce, [jniEnvironment]
		{
			javaHttpApiResolved = javaHttpApi.Resolve(jniEnvironment);
		});
		if (!javaHttpApiResolved)
		{
			response.SetTransportError("java.net.HttpURLConnection is unavailable");
			return response;
		}

		if (m_aborted.load())
		{
			response.SetTransportError(kShuttingDownError);
			return response;
		}

		if (jniEnvironment->PushLocalFrame(kLocalReferenceCapacity) != 0)
		{
			jniEnvironment->ExceptionClear();
			response.SetTransportError("out of JNI local references");
			return response;
		}

		response = PerformInLocalFrame(jniEnvironment, request);
		jniEnvironment->PopLocalFrame(nullptr);
		return response;
	}

	void AbortInFlight() override
	{
		JavaVM* virtualMachine = javaVirtualMachine.load();
		std::lock_guard<std::mutex> lock(m_mutex);
		m_aborted.store(true);
		if (virtualMachine == nullptr || m_activeConnections.empty())
		{
			return;
		}

		JNIEnv* jniEnvironment = nullptr;
		bool attachedHere = false;
		if (virtualMachine->GetEnv(reinterpret_cast<void**>(&jniEnvironment), JNI_VERSION_1_6) != JNI_OK)
		{
			if (virtualMachine->AttachCurrentThread(reinterpret_cast<AttachEnvironmentPointer>(&jniEnvironment),
													nullptr) != JNI_OK)
			{
				return;
			}

			attachedHere = true;
		}

		// disconnect() from another thread makes the blocked worker's read
		// fail with an IOException.
		for (jobject activeConnection : m_activeConnections)
		{
			javaHttpApi.CloseQuietly(jniEnvironment, activeConnection, true);
		}

		if (attachedHere)
		{
			virtualMachine->DetachCurrentThread();
		}
	}

private:
	// When a Java exception is pending, clears it, records it in response and returns true.
	bool TakeFailure(JNIEnv* jniEnvironment, HttpResponse& response) const
	{
		if (!jniEnvironment->ExceptionCheck())
		{
			return false;
		}

		const std::string exceptionDescription = javaHttpApi.TakePendingException(jniEnvironment);
		response.SetStatusCode(0);
		response.SetBody(std::string());
		response.SetTransportError(m_aborted.load() ? std::string(kShuttingDownError) : exceptionDescription);
		return true;
	}

	HttpResponse PerformInLocalFrame(JNIEnv* jniEnvironment, const HttpRequest& request)
	{
		HttpResponse response;
		jobject connection = javaHttpApi.OpenHttpConnection(jniEnvironment, request.GetUrl());
		if (TakeFailure(jniEnvironment, response))
		{
			return response;
		}

		if (connection == nullptr)
		{
			response.SetTransportError("URL did not open an HTTP connection");
			return response;
		}

		// The global reference lets AbortInFlight disconnect from another thread.
		// It is tracked in m_activeConnections, so it is released by hand below.
		jobject globalConnection = jniEnvironment->NewGlobalRef(connection);
		{
			// Checked under the lock so a request cannot start after
			// AbortInFlight has already run.
			std::lock_guard<std::mutex> lock(m_mutex);
			if (m_aborted.load())
			{
				jniEnvironment->DeleteGlobalRef(globalConnection);
				response.SetTransportError(kShuttingDownError);
				return response;
			}

			m_activeConnections.insert(globalConnection);
		}

		ExchangeOverConnection(jniEnvironment, request, connection, response);

		javaHttpApi.CloseQuietly(jniEnvironment, connection, true);
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_activeConnections.erase(globalConnection);
		}
		jniEnvironment->DeleteGlobalRef(globalConnection);
		return response;
	}

	void ExchangeOverConnection(JNIEnv* jniEnvironment, const HttpRequest& request, jobject connection,
								HttpResponse& response)
	{
		javaHttpApi.Configure(jniEnvironment, connection, static_cast<jint>(request.GetTimeoutMilliseconds()));
		if (TakeFailure(jniEnvironment, response))
		{
			return;
		}

		javaHttpApi.SetRequestMethod(jniEnvironment, connection, request.GetMethod());
		if (TakeFailure(jniEnvironment, response))
		{
			return;
		}

		for (const auto& [headerName, headerValue] : request.GetHeaders())
		{
			javaHttpApi.SetRequestHeader(jniEnvironment, connection, headerName, headerValue);
			if (TakeFailure(jniEnvironment, response))
			{
				return;
			}
		}

		if (!request.GetBody().empty())
		{
			javaHttpApi.WriteRequestBody(jniEnvironment, connection, request.GetBody());
			if (TakeFailure(jniEnvironment, response))
			{
				return;
			}
		}

		const jint statusCode = javaHttpApi.GetResponseCode(jniEnvironment, connection);
		if (TakeFailure(jniEnvironment, response))
		{
			return;
		}

		response.SetStatusCode(statusCode);

		jobject responseStream = javaHttpApi.OpenResponseStream(jniEnvironment, connection, statusCode);
		if (responseStream == nullptr)
		{
			return;
		}

		javaHttpApi.ReadStreamIntoBody(jniEnvironment, responseStream, response);
		if (TakeFailure(jniEnvironment, response))
		{
			return;
		}

		javaHttpApi.CloseQuietly(jniEnvironment, responseStream, false);
	}

	mutable std::mutex m_mutex;
	std::set<jobject> m_activeConnections;
	std::atomic<bool> m_aborted{false};
};

}  // namespace

std::unique_ptr<Transport> MakeBuiltinTransport()
{
	return std::make_unique<ThreadedTransport>(std::make_shared<AndroidBackend>());
}

const char* BuiltinTransportName()
{
	return "android";
}

}  // namespace apex

extern "C" APEX_API void APEX_CALL ApexAndroidSetJavaVm(void* javaVm)
{
	apex::javaVirtualMachine.store(static_cast<JavaVM*>(javaVm));
}

#if defined(APEX_DEFINE_JNI_ONLOAD)
extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* virtualMachine, void* /*reserved*/)
{
	apex::javaVirtualMachine.store(virtualMachine);
	return JNI_VERSION_1_6;
}
#endif
