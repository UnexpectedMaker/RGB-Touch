#pragma once

#include <WiFi.h>
#include <esp_now.h>
#include <AsyncUDP.h>
#include <lwip/ip_addr.h>
#include <lwip/igmp.h>
#include <Arduino.h>
#include <vector>

#include "utilities/logging.h"
#include "frameworks/game_tictactoe.h"

#define DEFAULT_PORT 5568

// Recorder Touch Point Data Packet
struct touch_packet_t
{
		uint16_t index;
		uint16_t total;
		uint8_t x;
		uint8_t y;
		uint8_t col;
		// uint8_t retouched;
		// we are now embedding retouch into created as a signed value
		// - is retouched, + is normal
		// now limited to 30000 seconds of recording playback
		int16_t created;

} __attribute__((packed));

typedef union
{
		struct
		{
				touch_packet_t touches[20];
		} __attribute__((packed));

		uint8_t raw[180];
} chunk_t;

// Status structure
typedef struct
{
		uint32_t num_packets;
		uint32_t packet_errors;
		IPAddress last_clientIP;
		uint16_t last_clientPort;
		unsigned long last_seen;
} packet_stats_t;

typedef uint16_t ESPAsyncPortId;

class Share_UDP
{
	private:
		AsyncUDP udp; // AsyncUDP
		void *UserInfo = nullptr;

		// Packet parser callback
		void parsePacket(AsyncUDPPacket _packet);

		void (*PacketCallback)(chunk_t *ReceivedData, void *UserInfo) = nullptr;
		ESPAsyncPortId ListenPort = DEFAULT_PORT;

		IPAddress target_ip;

	public:
		Share_UDP(){};

		// Generic UDP listener, no physical or IP configuration
		bool begin(IPAddress addr);
		bool begin(IPAddress addr, ESPAsyncPortId UdpPortId);

		bool send(chunk_t chunk);

		// Callback support
		void registerCallback(void *_UserInfo, void (*cbFunction)(chunk_t *, void *))
		{
			PacketCallback = cbFunction;
			UserInfo = _UserInfo;
		}
};

extern Share_UDP share_udp;

class Share_ESPNOW
{
	public:
		Share_ESPNOW()
		{
			// init_esp_now();
		}

		void init_esp_now();
		void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
		void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len);
		void find_peers(bool force = false);
		bool send(chunk_t chunk);
		bool send(game_data_chunk_t data);

		uint32_t calc_mac_val(const uint8_t mac[6]);

		// Static method to get the instance of the manager
		static Share_ESPNOW &getInstance()
		{
			static Share_ESPNOW instance;
			return instance;
		}

		bool has_peer();

	private:
		// struct PeerInfo
		// {
		// 		uint8_t peer_addr[6];
		// 		esp_now_peer_info_t peerInfo;
		// };

		std::vector<esp_now_peer_info_t> peers;
		uint8_t broadcastAddress[6];

		uint32_t my_calc_value = 0;

		unsigned long next_peer_broadcast = 0;

		// Wrapper for the send callback
		static void onDataSentWrapper(const uint8_t *mac_addr, esp_now_send_status_t status);

		// Wrapper for the receive callback
		static void onDataRecvWrapper(const uint8_t *mac_addr, const uint8_t *data, int data_len);
};

extern Share_ESPNOW share_espnow;

class Share
{
	public:
		Share();

		chunk_t *temp_buffer;			 // Pointer to scratch packet buffer
		std::vector<chunk_t> send_queue; // storage for incoming packets

		packet_stats_t stats; // Statistics tracker

		touch_packet_t create(uint16_t index, uint16_t points, uint8_t x, uint8_t y, uint8_t col, bool retouch, int16_t tstamp);

		bool send(chunk_t chunk);

		const char *convert_to_base64(chunk_t chunk);
		uint8_t *convert_from_base64(const char *data);

		void clear_stats();

		// Vector access
		inline bool isEmpty()
		{
			return send_queue.empty();
		}

		inline chunk_t pull()
		{
			if (!send_queue.empty())
			{
				chunk_t packet = send_queue.front();
				send_queue.erase(send_queue.begin());
				return packet;
			}
		}

		inline chunk_t last()
		{
			return send_queue.back();
		}

		inline chunk_t get(uint8_t index)
		{
			return send_queue[index];
		}

		inline void clear()
		{
			send_queue.clear();
			send_queue.shrink_to_fit();
		}

		inline uint8_t count() { return send_queue.size(); }
};

extern Share share;
