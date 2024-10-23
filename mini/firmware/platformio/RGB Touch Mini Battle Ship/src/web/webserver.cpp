#include "settings.h"
#include "webserver.h"

// HTML Templates
#include "www/www_general.h"

String WebServer::processor(const String &var)
{
	if (var == "META")
	{
		return String(meta);
	}
	if (var == "FOOTER")
	{
		return String(footer);
	}
	else if (var == "THEME")
	{
		return "dark";
	}
	else if (var == "CSS")
	{
		return String(css_dark);
	}

	return "";
}

void WebServer::start(String _name)
{
	_running = true;
	info_println("Starting webserver");

	MDNS.begin(_name.c_str());

	web_server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send_P(200, "text/html", index_html, processor);
	});

	web_server.on("/index.htm", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send_P(200, "text/html", index_html, processor);
	});

	web_server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request) {
		request->send_P(200, "text/html", index_html, processor);
	});

	web_server.onNotFound([](AsyncWebServerRequest *request) {
		request->send(404, "text/plain", "Not found");
	});

	web_server.on("/update_settings", HTTP_POST, [](AsyncWebServerRequest *request) {
		// settings.save(true);

		request->send(200, "text/plain", "Settings Saved!");
	});

	info_println("web_server.begin();");
	web_server.begin();
	_running = true;
}

void WebServer::stop(bool restart)
{
	info_println("Webserver stop");
	web_server.end();
	_running = false;
}

void WebServer::process() {}

bool WebServer::is_running() { return _running; }

WebServer web_server;