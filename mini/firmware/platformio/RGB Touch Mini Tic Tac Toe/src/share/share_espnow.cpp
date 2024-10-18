
#include <share/share.h>
#include <audio/audio.h>
#include <recorder.h>

// Initialize ESP-NOW
void Share_ESPNOW::init_esp_now()
{

	// Set device as a Wi-Fi Station
	WiFi.mode(WIFI_STA);
	uint8_t mac[6];
	WiFi.macAddress(mac);

	WiFi.disconnect();

	// Init ESP-NOW
	esp_err_t initResult = esp_now_init();
	if (initResult != ESP_OK)
	{
		info_print("Error initializing ESP-NOW: ");
		info_println(initResult);
		return;
	}
	info_println("ESP-NOW initialized successfully");

	// Register send callback
	esp_err_t sendCbResult = esp_now_register_send_cb(&Share_ESPNOW::onDataSentWrapper);
	if (sendCbResult != ESP_OK)
	{
		info_print("Error registering send callback: ");
		info_println(sendCbResult);
		return;
	}

	// Register receive callback
	esp_err_t recvCbResult = esp_now_register_recv_cb(&Share_ESPNOW::onDataRecvWrapper);
	if (recvCbResult != ESP_OK)
	{
		info_print("Error registering receive callback: ");
		info_println(recvCbResult);
		return;
	}

	// Initialize the broadcast address
	memset(broadcastAddress, 0xFF, 6);

	// Add broadcast peer
	esp_now_peer_info_t peerInfo;
	memset(&peerInfo, 0, sizeof(peerInfo));
	memcpy(peerInfo.peer_addr, broadcastAddress, 6);
	peerInfo.channel = 0;
	peerInfo.encrypt = false;

	my_calc_value = calc_mac_val(mac);

	esp_err_t addPeerResult = esp_now_add_peer(&peerInfo);
	if (addPeerResult != ESP_OK)
	{
		info_print("Failed to add broadcast peer: ");
		info_println(addPeerResult);
		return;
	}
	info_println("Broadcast peer added successfully");
}

// Callback when data is sent
void Share_ESPNOW::onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
	info_print("Last Packet Sent to: ");
	for (int i = 0; i < 6; i++)
	{
		info_printf("%02X", mac_addr[i]);
		if (i < 5)
			info_print(":");
	}
	info_print(" | Status: ");
	info_println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// Callback when data is received
void Share_ESPNOW::onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{
	info_print("Last Packet Received from: ");
	for (int i = 0; i < 6; i++)
	{
		info_printf("%02X", mac_addr[i]);
		if (i < 5)
			info_print(":");
	}
	info_println();

	// This is a discovery message
	if (data_len == 10 && strncmp((const char *)data, "RGBT_PING!", 10) == 0)
	{
		// Check if the peer is already known
		bool known_peer = false;
		for (auto &peer : peers)
		{
			if (memcmp(peer.peer_addr, mac_addr, 6) == 0)
			{
				known_peer = true;
				break;
			}
		}

		// If the peer is new, add it to the vector
		if (!known_peer)
		{
			esp_now_peer_info_t new_peer;
			// memcpy(new_peer.peer_addr, mac_addr, 6);
			memset(&new_peer, 0, sizeof(new_peer));
			memcpy(new_peer.peer_addr, mac_addr, 6);
			new_peer.channel = 0; // Use default channel
			new_peer.encrypt = false;

			if (esp_now_add_peer(&new_peer) == ESP_OK)
			{
				info_println("New peer added!");
				peers.push_back(new_peer);
				find_peers(true);
				uint32_t peer_calc_value = calc_mac_val(new_peer.peer_addr);

				if ( game == nullptr ) {
					info_printf("Game not found - my calc? %d, peer calc? %d\n", my_calc_value, peer_calc_value);
					audio_player.play_note(11, 11, 1.0, 1);
				} 
				else 
				{
					audio_player.play_wav("found_peer");
					info_printf("my calc? %d, peer calc? %d\n", my_calc_value, peer_calc_value);
					game->set_hosting(my_calc_value > peer_calc_value);
				}

			}
			else
			{
				info_println("Failed to add peer.");
			}
		}
	}
	else
	{
		// TODO : Should we check its going to the right game ?
		if ( game != nullptr && game->onDataRecv(mac_addr,data,data_len) ) {
			return;
		}

//			info_println("Game failed to accept data recieved.");

		// info_print("Data: ");
		// for (int i = 0; i < data_len; i++)
		// {
		// 	info_printf("%d ", (uint8_t)data[i]);
		// 	if (i % 9 == 0)
		// 		info_println();
		// }

		// info_println();

		// At the moment, we are only accepting shared packets from people we know
		share.temp_buffer = reinterpret_cast<chunk_t *>((uint8_t *)data);

		chunk_t new_packet = *share.temp_buffer;

		// Add the pointer to the vector
		share.send_queue.push_back(new_packet);

		share.stats.num_packets++;
		// share.stats.last_clientIP = _packet.remoteIP();
		// share.stats.last_clientPort = _packet.remotePort();
		share.stats.last_seen = millis();

		info_printf("num_packets: %d\n", share.stats.num_packets);
	}
}

uint32_t Share_ESPNOW::calc_mac_val(const uint8_t mac[6])
{
	uint32_t val = 0;
	for (size_t i = 0; i < 6; i++)
	{
		val += mac[i];
	}

	return val;
}

// Send a broadcast message to discover peers
void Share_ESPNOW::find_peers(bool force)
{
	if (force || millis() - next_peer_broadcast > 10000)
	{
		next_peer_broadcast = millis();

		next_peer_broadcast = millis();
		const char *message = "RGBT_PING!";
		esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)message, strlen(message));

		if (result == ESP_OK)
		{
			for (auto &peer : peers)
			{

				//peers.data
			}
			
			info_printf("Broadcast message sent successfully : %d\n",peers.size());
		}
		else
		{
			info_print("Error sending broadcast message: ");
			info_println(result);
		}
	}
}

bool Share_ESPNOW::has_peer()
{
	// info_printf("num peers: %d\n", peers.size());
	return (!peers.empty());
}

bool Share_ESPNOW::send(chunk_t chunk)
{
	uint8_t *raw_ptr = chunk.raw;
	esp_err_t result = ESP_FAIL;
	uint8_t attemps = 0;

	while (result != ESP_OK && attemps++ < 5)
	{
		delay(20);
		result = esp_now_send(peers[0].peer_addr, raw_ptr, Elements(chunk.raw));
	}

	return (result == ESP_OK);
}

bool Share_ESPNOW::send_gamedata(uint8_t *raw_ptr, uint8_t data_size)
{
	esp_err_t result = ESP_FAIL;
	uint8_t attemps = 0;

	while (result != ESP_OK && attemps++ < 5)
	{
		delay(20);
		result = esp_now_send(peers[0].peer_addr, raw_ptr, data_size) ;//Elements(raw_ptr));
	}

	return (result == ESP_OK);
}

// Wrapper for the send callback
void Share_ESPNOW::onDataSentWrapper(const uint8_t *mac_addr, esp_now_send_status_t status)
{
	getInstance().onDataSent(mac_addr, status);
}

// Wrapper for the receive callback
void Share_ESPNOW::onDataRecvWrapper(const uint8_t *mac_addr, const uint8_t *data, int data_len)
{
	getInstance().onDataRecv(mac_addr, data, data_len);
}

Share_ESPNOW share_espnow;
