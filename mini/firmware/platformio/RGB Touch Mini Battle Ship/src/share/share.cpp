#include "share/share.h"
#include "base64.h"
#include <rBase64.h>
#include <string.h>

// Constructor
Share::Share()
{
	stats.num_packets = 0;
	stats.packet_errors = 0;
}

touch_packet_t Share::create(uint16_t index, uint16_t points, uint8_t x, uint8_t y, uint8_t col, bool retouch, int16_t tstamp)
{
	touch_packet_t new_packet;
	new_packet.index = index;
	new_packet.total = points;
	new_packet.x = x;
	new_packet.y = y;
	new_packet.col = col;
	new_packet.created = (retouch ? -tstamp : tstamp);

	return new_packet;
}

bool Share::send(chunk_t chunk)
{
	return (share_espnow.getInstance().send(chunk));
	// return (share_udp.send(chunk));
}

const char *Share::convert_to_base64(chunk_t chunk)
{
	uint8_t *raw_ptr = chunk.raw;

	info_println("before encode");

	for (int i = 0; i < 240; i++)
		info_printf("%d", raw_ptr[i]);

	info_println();

	size_t rbase64_result = rbase64.encode(raw_ptr, 240);
	if (rbase64_result == RBASE64_STATUS_OK)
	{
		info_println("\nConverted the String to Base64 : ");
		info_println(rbase64.result());
		return rbase64.result();
	}
	else
	{
		info_printf("rbase64 Error: %d\n", rbase64_result);
	}
	return nullptr;
}

uint8_t *Share::convert_from_base64(const char *data)
{
	size_t rbase64_result = rbase64.decode(data);
	if (rbase64_result == RBASE64_STATUS_OK)
	{
		info_println("\nConverted the data back from Base64 : ");

		for (int i = 0; i < 240; i++)
			info_printf("%d", (uint8_t)(rbase64.result()[i]));

		info_println();

		return (uint8_t *)rbase64.result();
	}
	else
	{
		info_printf("rbase64 Error: %d\n", rbase64_result);
	}

	return nullptr;
}

void Share::clear_stats()
{
	stats.num_packets = 0;
	stats.packet_errors = 0;
	stats.last_seen = 0;
}

Share share;
