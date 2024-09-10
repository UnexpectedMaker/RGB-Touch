#include "share/share.h"
#include "base64.h"
#include <rBase64.h>
#include <string.h>

// Call begin with the default port
bool Share_UDP::begin(IPAddress addr)
{
	return begin(addr, ListenPort);
}

// Call begin with a custom port
bool Share_UDP::begin(IPAddress addr, ESPAsyncPortId UdpPortId)
{
	bool success = false;
	ListenPort = UdpPortId;
	target_ip = addr;
	delay(100);
	if (udp.listen(ListenPort))
	{
		success = true;
		udp.onPacket(std::bind(&Share_UDP::parsePacket, this, std::placeholders::_1));
	}
	return success;
}

void Share_UDP::parsePacket(AsyncUDPPacket _packet)
{
	share.temp_buffer = reinterpret_cast<chunk_t *>(_packet.data());

	do // once
	{
		if (PacketCallback)
		{
			(*PacketCallback)(share.temp_buffer, UserInfo);
		}

		chunk_t new_packet = *share.temp_buffer;

		// Add the pointer to the vector
		share.send_queue.push_back(new_packet);

		share.stats.num_packets++;
		share.stats.last_clientIP = _packet.remoteIP();
		share.stats.last_clientPort = _packet.remotePort();
		share.stats.last_seen = millis();

		info_printf("num_packets: %d\n", share.stats.num_packets);
	}
	while (false);
}

bool Share_UDP::send(chunk_t chunk)
{
	uint8_t *raw_ptr = chunk.raw;
	size_t result = udp.writeTo(raw_ptr, 240, target_ip, ListenPort);

	return (result > 0);
}

Share_UDP share_udp;
