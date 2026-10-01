// File: the destructors of the event-sink interfaces that Red Alert implements.

#include "ra/wol/wolapi.h"

namespace ra {

IChatEvent::~IChatEvent() = default;
IDownloadEvent::~IDownloadEvent() = default;
INetUtilEvent::~INetUtilEvent() = default;

}  // namespace ra
