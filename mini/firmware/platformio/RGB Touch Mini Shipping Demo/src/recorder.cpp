#include "recorder.h"
#include "touch/touch.h"
#include "audio/audio.h"
#include <share/share.h>

void Recorder::save() {}
void Recorder::load() {}

bool Recorder::share_recording()
{
	// If for some reason we are no longer connected to the internet, we bail sending
	if ((WiFi.status() != WL_CONNECTED) && !share_espnow.getInstance().has_peer())
		return false;

	if (rec.touch_recording.size() == 0)
		return false;

	// Notify the user sending has begun with amn audio note
	audio_player.play_note(11, 0);

	// Some local vars for tracking
	bool sent = false;
	uint16_t chunk = 0;
	uint16_t errors = 0;
	uint8_t num_packets = 0;

	share.clear_stats();

	// buffer to hold the chunk data we are assembling
	chunk_t temp_buffer;

	uint16_t num_touches = rec.touch_recording.size();
	for (size_t i = 0; i < num_touches; i++)
	{
		// If we've filled a chunk with data (20 touches)
		// We send it off and start a new chunk
		if (chunk == max_chunks)
		{
			chunk = 0;
			sent = true;
			if (share.send(temp_buffer))
			{
				num_packets++;
				info_printf("Sending packet %d\n", num_packets);
			}
			else
				errors++;

			// delay(10);
			chunk_t temp_buffer;
		}

		if (chunk < max_chunks)
		{
			sent = false;
			RecordedPoint &rp = rec.touch_recording[i];
			temp_buffer.touches[chunk] = share.create(i, num_touches, rp.x, rp.y, rp.color, rp.touched, (int16_t)rp.created);
			// info_printf("creating pint into slot %d\n", chunk);
			chunk++;
		}
	}

	// if the last chunk was not filled, we have to send the final partial chunk
	if (!sent)
	{
		if (share.send(temp_buffer))
		{
			num_packets++;
			info_printf("Sending final packet %d\n", num_packets);
		}
		else
			errors++;
	}

	info_printf("Sent %d points in %d packets with %d errors!\n", rec.touch_recording.size(), num_packets, errors);

	if (errors > 0)
		audio_player.play_wav("error_send");
	// display.show_text(0, 5, String(num_packets) + " " + String(errors));

	// Clear the recorded touch data
	rec.touch_recording.clear();

	return true;
}

bool Recorder::recieve_recording()
{
	/*
	* This code is legacy UDP code that requires connecting to the internet

	if ((WiFi.status() != WL_CONNECTED))
	{
		// if (!audio_player.queue_is_empty())
		// 	return false;

		wifi_controller.connect();

		while (WiFi.status() != WL_CONNECTED)
			delay(10);

		IPAddress ip = WiFi.localIP();
		info_print("IP Address: ");
		info_println(ip);

		audio_player.play_menu_beep(11, 0);

		String ip_str = ip.toString();

		if (ip_str == "192.168.1.17")
			target_ip = IPAddress(192, 168, 1, 229);
		else if (ip_str == "192.168.1.229")
			target_ip = IPAddress(192, 168, 1, 17);
		else if (ip_str == "192.168.1.83")
			target_ip = IPAddress(192, 168, 1, 89);
		else if (ip_str == "192.168.1.89")
			target_ip = IPAddress(192, 168, 1, 83);
		else
			target_ip = ip;

		if (share_udp.begin(target_ip))
			info_printf("Listening for data, Targeting %s\n", target_ip.toString());
		else
			info_println("*** share.begin failed ***");
	}
	*/

	//
	// Did we get a new packet
	//

	if (!share.isEmpty())
	{
		// If this the first packer we are receiving from this message?
		// If so, set some tuff up and notify the user a new message is coming in via an audio tone
		if (!is_receiving)
		{
			is_receiving = true;
			touch.clear_all_touches();
			rec.touch_recording.clear();
			receive_timout = millis();
			audio_player.play_note(0, 0, 1.0, 1);
			info_println("rec-ving");
		}

		// Grab the chunk
		chunk_t chunk = share.pull();

		for (int i = 0; i < 20; i++)
		{
			touch_packet_t &packet = chunk.touches[i];

			if (packet.total > 0)
			{
				// update the timeout time as we just got some valid data
				receive_timout = millis();

				// info_printf("packet %d/%d (%d,%d) col %d\n", packet.index, packet.total, packet.x, packet.y, packet.col);

				RecordedPoint rp = RecordedPoint(packet.x, packet.y, packet.col, (packet.created < 0), (uint16_t)abs(packet.created));
				rec.touch_recording.push_back(rp);

				if (is_receiving && packet.index == (packet.total - 1))
				{
					is_receiving = false;
					is_playing = true;
					rec.playback_start_time = millis();
					rec.playback_index = 0;
					// info_printf("Finished - got %d touched\n", packet.index);
					audio_player.play_note(6, 11, 1.0, 1);
					return true;
				}
			}
		}

		return true;
	}
	else if (is_receiving && millis() - receive_timout > 5000)
	{
		// If it's been more than 5 seconds since we got the last packet, and the finished state
		// has not triggered, we'll cancel the recieving process and let the user know there was an error
		is_receiving = false;
		share.clear();
		rec.touch_recording.clear();
		audio_player.play_wav("error_recv");
	}

	return false;
}

Recorder recorder;