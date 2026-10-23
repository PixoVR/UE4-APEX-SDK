#pragma once

// Session telemetry: records from the engine's analytics components, batched
// into packets in the format the Unity SDK 2.0 PixoVR analytics provider
// sends (spec/schema/telemetry_packet.json). The engine captures; this class
// numbers, times, quantizes, de-duplicates and decides when to flush.

#include "JsonHelpers.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace apex
{

class TelemetryOptions
{
public:
	// While a session is active, buffered records are sent this often.
	double GetFlushIntervalSeconds() const
	{
		return m_flushIntervalSeconds;
	}
	void SetFlushIntervalSeconds(double flushIntervalSeconds)
	{
		m_flushIntervalSeconds = flushIntervalSeconds;
	}

	// How often PoseSampleDue returns true.
	double GetPoseSamplesPerSecond() const
	{
		return m_poseSamplesPerSecond;
	}
	void SetPoseSamplesPerSecond(double poseSamplesPerSecond)
	{
		m_poseSamplesPerSecond = poseSamplesPerSecond;
	}

	// A packet is sent as soon as it holds this many records.
	int GetMaximumRecordsPerPacket() const
	{
		return m_maximumRecordsPerPacket;
	}
	void SetMaximumRecordsPerPacket(int maximumRecordsPerPacket)
	{
		m_maximumRecordsPerPacket = maximumRecordsPerPacket;
	}

private:
	double m_flushIntervalSeconds = 10.0;
	double m_poseSamplesPerSecond = 2.0;
	int m_maximumRecordsPerPacket = 500;
};

// Who a packet belongs to, read when the packet is built.
class TelemetryContext
{
public:
	int64_t GetSessionId() const
	{
		return m_sessionId;
	}
	void SetSessionId(int64_t sessionId)
	{
		m_sessionId = sessionId;
	}

	// The session event uuid without dashes; empty outside a session. The
	// platform links packets to the session, and to anything the module put
	// in the join's data (such as a scenario ID), through it.
	const std::string& GetRegistration() const
	{
		return m_registration;
	}
	void SetRegistration(std::string registration)
	{
		m_registration = std::move(registration);
	}

	int64_t GetUserId() const
	{
		return m_userId;
	}
	void SetUserId(int64_t userId)
	{
		m_userId = userId;
	}

	int64_t GetModuleId() const
	{
		return m_moduleId;
	}
	void SetModuleId(int64_t moduleId)
	{
		m_moduleId = moduleId;
	}

private:
	int64_t m_sessionId = 0;
	std::string m_registration;
	int64_t m_userId = 0;
	int64_t m_moduleId = 0;
};

using Vector3 = std::array<double, 3>;

// Position in meters and rotation in Euler degrees, in Unity's axes (x right,
// y up, z forward), as the 2.0 packets carry them.
class Pose
{
public:
	Pose() = default;
	Pose(const Vector3& position, const Vector3& rotation) : m_position(position), m_rotation(rotation)
	{
	}

	const Vector3& GetPosition() const
	{
		return m_position;
	}
	void SetPosition(const Vector3& position)
	{
		m_position = position;
	}

	const Vector3& GetRotation() const
	{
		return m_rotation;
	}
	void SetRotation(const Vector3& rotation)
	{
		m_rotation = rotation;
	}

private:
	Vector3 m_position = {0.0, 0.0, 0.0};
	Vector3 m_rotation = {0.0, 0.0, 0.0};
};

// A pose rounded like the 2.0 provider: position in whole centimeters (x, y, z),
// then rotation in whole degrees (x, y, z).
using QuantizedPose = std::array<int, 6>;

class TelemetryBuffer
{
public:
	using MonotonicSecondsSource = std::function<double()>;
	using UnixMillisecondsSource = std::function<int64_t()>;
	using ContextSource = std::function<TelemetryContext()>;
	using PacketSink = std::function<void(const Json& packet)>;

	TelemetryBuffer(TelemetryOptions options, MonotonicSecondsSource monotonicSeconds,
					UnixMillisecondsSource unixMilliseconds, ContextSource contextSource, PacketSink packetSink);

	// Records ("k" in the packet). Object records name an object by the
	// engine's tracked ID; register it first to give it a name and mesh.
	void ObjectRegistered(const std::string& trackedId, const std::string& objectName, const std::string& meshName,
						  const Pose& pose);                                                // "r"
	void ObjectUnregistered(const std::string& trackedId, const Pose& pose);                // "u"
	void EngagementBegin(const std::string& trackedId, const std::string& engagementName);  // "e"
	void EngagementEnd(const std::string& trackedId, const std::string& engagementName);    // "x"
	void Interaction(const std::string& trackedId, const std::string& actionName);          // "i"
	void StepBegin(const std::string& stepName);                                            // "sb"
	void StepEnd(const std::string& stepName, bool succeeded, double score);                // "se"
	// A pose sample ("p"): the head when trackedId is empty. Dropped when it
	// rounds to the same values as the last sample for that object.
	void RecordPose(const std::string& trackedId, const Pose& pose);

	// True at most GetPoseSamplesPerSecond times a second; each true starts a sample.
	bool PoseSampleDue();

	// Sends the buffered records, if any.
	void Flush();
	// Sends the session's final packet, marked "end", even when empty.
	void FlushSessionEnd();
	// Called regularly by the client; flushes on the interval while
	// sessionActive is true.
	void Tick(bool sessionActive);
	// Restarts the flush interval, e.g. when a session starts.
	void RestartFlushInterval();

	size_t GetBufferedRecordCount() const
	{
		return m_bufferedRecords.size();
	}

private:
	class RegisteredObject
	{
	public:
		const std::string& GetObjectName() const
		{
			return m_objectName;
		}
		const std::string& GetMeshName() const
		{
			return m_meshName;
		}
		void SetNames(const std::string& objectName, const std::string& meshName)
		{
			m_objectName = objectName;
			m_meshName = meshName;
		}

		// True when quantizedPose equals the last pose recorded for the object.
		bool HasSamePose(const QuantizedPose& quantizedPose) const
		{
			return m_lastQuantizedPose && *m_lastQuantizedPose == quantizedPose;
		}
		void SetLastPose(const QuantizedPose& quantizedPose)
		{
			m_lastQuantizedPose = quantizedPose;
		}

	private:
		std::string m_objectName;
		std::string m_meshName;
		std::optional<QuantizedPose> m_lastQuantizedPose;
	};

	Json StartRecord(const char* recordKind, double nowSeconds);
	void AppendRecord(Json record, double nowSeconds);
	void AppendObjectRecord(const char* recordKind, const std::string& trackedId, const char* nameField,
							const std::string& nameValue);
	int GetPacketObjectIndex(const std::string& trackedId);
	static QuantizedPose QuantizePose(const Pose& pose);
	static void WritePoseFields(Json& record, const QuantizedPose& quantizedPose);
	void SendPacket(bool sessionEnded, double nowSeconds);

	TelemetryOptions m_options;
	MonotonicSecondsSource m_monotonicSeconds;
	UnixMillisecondsSource m_unixMilliseconds;
	ContextSource m_contextSource;
	PacketSink m_packetSink;

	// Per packet.
	std::vector<Json> m_bufferedRecords;
	Json m_packetObjects = Json::array();
	std::unordered_map<std::string, int> m_packetObjectIndexes;
	double m_packetStartSeconds = 0.0;
	int64_t m_packetStartUnixMilliseconds = 0;

	// Across packets.
	std::unordered_map<std::string, RegisteredObject> m_registeredObjects;
	std::optional<QuantizedPose> m_lastHeadPose;
	int64_t m_packetSequenceNumber = 0;
	double m_lastFlushSeconds = 0.0;
	double m_lastPoseSampleSeconds = 0.0;
	bool m_poseSampleTaken = false;
};

}  // namespace apex
