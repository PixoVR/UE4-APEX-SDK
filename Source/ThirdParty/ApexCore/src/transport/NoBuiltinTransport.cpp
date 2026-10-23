// Platforms without a built-in backend yet. Requests fail with
// APEX_ERR_NO_TRANSPORT unless the engine sets apex_config.http_send.

#include "../Transport.hpp"

namespace apex
{

std::unique_ptr<Transport> MakeBuiltinTransport()
{
	return nullptr;
}

const char* BuiltinTransportName()
{
	return "none";
}

}  // namespace apex
