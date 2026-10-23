#include "SessionInfo.hpp"

#include <cstdio>
#include <random>
#include <string>

namespace apex
{
namespace
{

constexpr const char* kBase64Alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
constexpr uint32_t kSixBitMask = 63;

// The base64 character for the six bits of threeBytes that start shiftBits from the right.
char Base64Character(uint32_t threeBytes, int shiftBits)
{
	return kBase64Alphabet[(threeBytes >> shiftBits) & kSixBitMask];
}

uint32_t ByteAt(const std::string& bytes, size_t byteIndex)
{
	return static_cast<uint8_t>(bytes[byteIndex]);
}

}  // namespace

Json MakeEventBody(const char* eventType, const SessionInfo& session, const std::string& moduleData)
{
	Json eventBody = {{"uuid", session.GetUuid()}, {"eventType", eventType}};
	if (std::string(eventType) == event_types::kJoined)
	{
		eventBody["moduleId"] = session.GetModuleId();
	}

	if (!moduleData.empty())
	{
		eventBody["data"] = Base64Encode(moduleData);
	}

	return eventBody;
}

std::string Base64Encode(const std::string& bytes)
{
	std::string encodedText;
	encodedText.reserve((bytes.size() + 2) / 3 * 4);
	size_t byteIndex = 0;
	for (; byteIndex + 2 < bytes.size(); byteIndex += 3)
	{
		const uint32_t threeBytes =
			(ByteAt(bytes, byteIndex) << 16) | (ByteAt(bytes, byteIndex + 1) << 8) | ByteAt(bytes, byteIndex + 2);
		encodedText += Base64Character(threeBytes, 18);
		encodedText += Base64Character(threeBytes, 12);
		encodedText += Base64Character(threeBytes, 6);
		encodedText += Base64Character(threeBytes, 0);
	}

	const size_t remainingByteCount = bytes.size() - byteIndex;
	if (remainingByteCount > 0)
	{
		uint32_t lastBytes = ByteAt(bytes, byteIndex) << 16;
		if (remainingByteCount == 2)
		{
			lastBytes |= ByteAt(bytes, byteIndex + 1) << 8;
		}

		encodedText += Base64Character(lastBytes, 18);
		encodedText += Base64Character(lastBytes, 12);
		encodedText += remainingByteCount == 2 ? Base64Character(lastBytes, 6) : '=';
		encodedText += '=';
	}

	return encodedText;
}

std::string MakeUuid()
{
	thread_local std::mt19937_64 randomEngine{std::random_device{}()};
	uint64_t highBits = randomEngine();
	uint64_t lowBits = randomEngine();
	highBits = (highBits & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;  // version 4
	lowBits = (lowBits & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;    // RFC 4122 variant
	char uuidText[37];
	std::snprintf(uuidText, sizeof(uuidText), "%08x-%04x-%04x-%04x-%012llx", static_cast<unsigned>(highBits >> 32),
				  static_cast<unsigned>((highBits >> 16) & 0xFFFF), static_cast<unsigned>(highBits & 0xFFFF),
				  static_cast<unsigned>(lowBits >> 48), static_cast<unsigned long long>(lowBits & 0xFFFFFFFFFFFFULL));
	return uuidText;
}

}  // namespace apex
