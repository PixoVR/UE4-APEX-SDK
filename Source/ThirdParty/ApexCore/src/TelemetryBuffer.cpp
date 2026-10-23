#include "TelemetryBuffer.hpp"

#include <cmath>
#include <limits>
#include <utility>

namespace apex
{
namespace
{

constexpr int kAxisCount = 3;
constexpr float kCentimetersPerMeter = 100.0f;

// Unity's Mathf.RoundToInt rounds halves to even; nearbyint does the same in
// the default rounding mode, so values match packets from the Unity SDK.
int RoundToInt(double value)
{
	if (!std::isfinite(value))
	{
		return 0;
	}

	const double roundedValue = std::nearbyint(value);
	if (roundedValue > static_cast<double>(std::numeric_limits<int>::max()))
	{
		return std::numeric_limits<int>::max();
	}

	if (roundedValue < static_cast<double>(std::numeric_limits<int>::min()))
	{
		return std::numeric_limits<int>::min();
	}

	return static_cast<int>(roundedValue);
}

}  // namespace

TelemetryBuffer::TelemetryBuffer(TelemetryOptions options, MonotonicSecondsSource monotonicSeconds,
								 UnixMillisecondsSource unixMilliseconds, ContextSource contextSource,
								 PacketSink packetSink)
	: m_options(options),
	  m_monotonicSeconds(std::move(monotonicSeconds)),
	  m_unixMilliseconds(std::move(unixMilliseconds)),
	  m_contextSource(std::move(contextSource)),
	  m_packetSink(std::move(packetSink))
{
	m_lastFlushSeconds = m_monotonicSeconds();
}

QuantizedPose TelemetryBuffer::QuantizePose(const Pose& pose)
{
	QuantizedPose quantizedPose = {};
	for (int axis = 0; axis < kAxisCount; ++axis)
	{
		// The 2.0 provider multiplies in single precision before rounding.
		quantizedPose[axis] = RoundToInt(static_cast<float>(pose.GetPosition()[axis]) * kCentimetersPerMeter);
		quantizedPose[kAxisCount + axis] = RoundToInt(static_cast<float>(pose.GetRotation()[axis]));
	}

	return quantizedPose;
}

void TelemetryBuffer::WritePoseFields(Json& record, const QuantizedPose& quantizedPose)
{
	record["p"] = {quantizedPose[0], quantizedPose[1], quantizedPose[2]};
	record["q"] = {quantizedPose[3], quantizedPose[4], quantizedPose[5]};
}

Json TelemetryBuffer::StartRecord(const char* recordKind, double nowSeconds)
{
	if (m_bufferedRecords.empty())
	{
		m_packetStartSeconds = nowSeconds;
		m_packetStartUnixMilliseconds = m_unixMilliseconds();
	}

	const float millisecondsSincePacketStart = static_cast<float>((nowSeconds - m_packetStartSeconds) * 1000.0);
	const int recordTimeMilliseconds = RoundToInt(millisecondsSincePacketStart);
	return {{"t", recordTimeMilliseconds < 0 ? 0 : recordTimeMilliseconds}, {"k", recordKind}};
}

void TelemetryBuffer::AppendRecord(Json record, double nowSeconds)
{
	m_bufferedRecords.push_back(std::move(record));
	const int maximumRecords = m_options.GetMaximumRecordsPerPacket();
	if (maximumRecords > 0 && m_bufferedRecords.size() >= static_cast<size_t>(maximumRecords))
	{
		SendPacket(false, nowSeconds);
	}
}

void TelemetryBuffer::AppendObjectRecord(const char* recordKind, const std::string& trackedId, const char* nameField,
										 const std::string& nameValue)
{
	const double nowSeconds = m_monotonicSeconds();
	Json record = StartRecord(recordKind, nowSeconds);
	record["o"] = GetPacketObjectIndex(trackedId);
	if (!nameValue.empty())
	{
		record[nameField] = nameValue;
	}

	AppendRecord(std::move(record), nowSeconds);
}

int TelemetryBuffer::GetPacketObjectIndex(const std::string& trackedId)
{
	const auto knownObject = m_packetObjectIndexes.find(trackedId);
	if (knownObject != m_packetObjectIndexes.end())
	{
		return knownObject->second;
	}

	const int objectIndex = static_cast<int>(m_packetObjects.size());
	m_packetObjectIndexes.emplace(trackedId, objectIndex);
	Json packetObject = {{"i", objectIndex}, {"id", trackedId}};
	const auto registeredObject = m_registeredObjects.find(trackedId);
	if (registeredObject != m_registeredObjects.end())
	{
		if (!registeredObject->second.GetObjectName().empty())
		{
			packetObject["n"] = registeredObject->second.GetObjectName();
		}

		if (!registeredObject->second.GetMeshName().empty())
		{
			packetObject["mesh"] = registeredObject->second.GetMeshName();
		}
	}

	m_packetObjects.push_back(std::move(packetObject));
	return objectIndex;
}

void TelemetryBuffer::ObjectRegistered(const std::string& trackedId, const std::string& objectName,
									   const std::string& meshName, const Pose& pose)
{
	const QuantizedPose quantizedPose = QuantizePose(pose);
	RegisteredObject& registeredObject = m_registeredObjects[trackedId];
	registeredObject.SetNames(objectName, meshName);
	registeredObject.SetLastPose(quantizedPose);

	const double nowSeconds = m_monotonicSeconds();
	Json record = StartRecord("r", nowSeconds);
	record["o"] = GetPacketObjectIndex(trackedId);
	WritePoseFields(record, quantizedPose);
	AppendRecord(std::move(record), nowSeconds);
}

void TelemetryBuffer::ObjectUnregistered(const std::string& trackedId, const Pose& pose)
{
	const double nowSeconds = m_monotonicSeconds();
	Json record = StartRecord("u", nowSeconds);
	record["o"] = GetPacketObjectIndex(trackedId);
	WritePoseFields(record, QuantizePose(pose));
	AppendRecord(std::move(record), nowSeconds);
	m_registeredObjects.erase(trackedId);
}

void TelemetryBuffer::EngagementBegin(const std::string& trackedId, const std::string& engagementName)
{
	AppendObjectRecord("e", trackedId, "n", engagementName);
}

void TelemetryBuffer::EngagementEnd(const std::string& trackedId, const std::string& engagementName)
{
	AppendObjectRecord("x", trackedId, "n", engagementName);
}

void TelemetryBuffer::Interaction(const std::string& trackedId, const std::string& actionName)
{
	AppendObjectRecord("i", trackedId, "n", actionName);
}

void TelemetryBuffer::StepBegin(const std::string& stepName)
{
	const double nowSeconds = m_monotonicSeconds();
	Json record = StartRecord("sb", nowSeconds);
	if (!stepName.empty())
	{
		record["n"] = stepName;
	}

	AppendRecord(std::move(record), nowSeconds);
}

void TelemetryBuffer::StepEnd(const std::string& stepName, bool succeeded, double score)
{
	const double nowSeconds = m_monotonicSeconds();
	Json record = StartRecord("se", nowSeconds);
	if (!stepName.empty())
	{
		record["n"] = stepName;
	}

	record["ok"] = succeeded;
	// Unity sends the score as a float; round-trip through float so the
	// values match what 2.0 produced for the same input.
	record["sc"] = std::isfinite(score) ? static_cast<double>(static_cast<float>(score)) : 0.0;
	AppendRecord(std::move(record), nowSeconds);
}

void TelemetryBuffer::RecordPose(const std::string& trackedId, const Pose& pose)
{
	const QuantizedPose quantizedPose = QuantizePose(pose);
	const bool isHeadPose = trackedId.empty();
	if (isHeadPose)
	{
		if (m_lastHeadPose && *m_lastHeadPose == quantizedPose)
		{
			return;
		}

		m_lastHeadPose = quantizedPose;
	}
	else
	{
		RegisteredObject& trackedObject = m_registeredObjects[trackedId];
		if (trackedObject.HasSamePose(quantizedPose))
		{
			return;
		}

		trackedObject.SetLastPose(quantizedPose);
	}

	const double nowSeconds = m_monotonicSeconds();
	Json record = StartRecord("p", nowSeconds);
	if (!isHeadPose)
	{
		record["o"] = GetPacketObjectIndex(trackedId);
	}

	WritePoseFields(record, quantizedPose);
	AppendRecord(std::move(record), nowSeconds);
}

bool TelemetryBuffer::PoseSampleDue()
{
	const double nowSeconds = m_monotonicSeconds();
	const double samplesPerSecond = m_options.GetPoseSamplesPerSecond();
	const double sampleIntervalSeconds = samplesPerSecond > 0.0 ? 1.0 / samplesPerSecond : 0.0;
	if (m_poseSampleTaken && nowSeconds - m_lastPoseSampleSeconds < sampleIntervalSeconds)
	{
		return false;
	}

	m_poseSampleTaken = true;
	m_lastPoseSampleSeconds = nowSeconds;
	return true;
}

void TelemetryBuffer::Flush()
{
	if (!m_bufferedRecords.empty())
	{
		SendPacket(false, m_monotonicSeconds());
	}
}

void TelemetryBuffer::FlushSessionEnd()
{
	SendPacket(true, m_monotonicSeconds());
}

void TelemetryBuffer::Tick(bool sessionActive)
{
	if (!sessionActive || m_bufferedRecords.empty())
	{
		return;
	}

	const double nowSeconds = m_monotonicSeconds();
	if (nowSeconds - m_lastFlushSeconds >= m_options.GetFlushIntervalSeconds())
	{
		SendPacket(false, nowSeconds);
	}
}

void TelemetryBuffer::RestartFlushInterval()
{
	m_lastFlushSeconds = m_monotonicSeconds();
}

void TelemetryBuffer::SendPacket(bool sessionEnded, double nowSeconds)
{
	if (m_bufferedRecords.empty() && !sessionEnded)
	{
		return;
	}

	const TelemetryContext packetContext = m_contextSource();
	Json packet = {
		{"v", 1},
		{"seq", ++m_packetSequenceNumber},
		{"t0", m_bufferedRecords.empty() ? m_unixMilliseconds() : m_packetStartUnixMilliseconds},
		{"s", packetContext.GetSessionId()},
		{"u", packetContext.GetUserId()},
		{"m", packetContext.GetModuleId()},
		{"ob", std::move(m_packetObjects)},
		{"ev", Json(std::move(m_bufferedRecords))},
	};
	if (!packetContext.GetRegistration().empty())
	{
		packet["r"] = packetContext.GetRegistration();
	}

	if (sessionEnded)
	{
		packet["end"] = true;
	}

	m_bufferedRecords.clear();
	m_packetObjects = Json::array();
	m_packetObjectIndexes.clear();
	m_packetStartSeconds = 0.0;
	m_packetStartUnixMilliseconds = 0;
	m_lastFlushSeconds = nowSeconds;

	m_packetSink(packet);
}

}  // namespace apex
