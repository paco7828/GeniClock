#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <DNSServer.h>
#include <esp_wifi.h>
#include "html-pages.h"

class WiFiManager {
private:
  WebServer server;
  DNSServer dnsServer;
  Preferences preferences;
  bool apRunning;
  bool credentialsChanged;
  String storedSSID;
  String storedPassword;

  // Captive portal configuration
  static const byte WIFI_CHANNEL = 6;
  static const int DNS_INTERVAL = 30;
  const IPAddress localIP = IPAddress(4, 3, 2, 1);
  const IPAddress gatewayIP = IPAddress(4, 3, 2, 1);
  const IPAddress subnetMask = IPAddress(255, 255, 255, 0);
  const String localIPURL = "http://4.3.2.1";

public:
  WiFiManager()
    : server(80), apRunning(false), credentialsChanged(false) {}

  void begin() {
    // Initialize preferences
    preferences.begin("wifi", false);

    // Load stored credentials
    storedSSID = preferences.getString("ssid", "");
    storedPassword = preferences.getString("password", "");
  }

  void getStoredCredentials(String& ssid, String& password) {
    ssid = storedSSID;
    password = storedPassword;
  }

  void startAP() {
    if (apRunning) return;

    // Stop any existing WiFi connection
    WiFi.disconnect();

    // Start the soft access point with captive portal configuration
    startSoftAccessPoint();

    // Set up DNS server for captive portal
    setUpDNSServer();

    // Set up web server with captive portal endpoints
    setUpWebserver();

    server.begin();
    apRunning = true;
    credentialsChanged = false;
  }

  void stopAP() {
    if (!apRunning) return;

    dnsServer.stop();
    server.stop();
    WiFi.softAPdisconnect(true);
    apRunning = false;
  }

  void handleClient() {
    if (apRunning) {
      dnsServer.processNextRequest();
      server.handleClient();
      delay(DNS_INTERVAL);
    }
  }

  bool credentialsUpdated() {
    return credentialsChanged;
  }

  // Function to check if any client is connected to the AP
  bool hasConnectedClients() {
    return WiFi.softAPgetStationNum() > 0;
  }

private:
  void startSoftAccessPoint() {
    // Check if an access point is already running
    if (WiFi.getMode() == WIFI_MODE_AP) {
      WiFi.softAPdisconnect(true);
    }

    // Set the WiFi mode to access point
    WiFi.mode(WIFI_MODE_AP);

    // Configure the soft access point with specific IP and subnet mask
    WiFi.softAPConfig(localIP, gatewayIP, subnetMask);

    // Start the access point using constants from constants.h
    WiFi.softAP(AP_SSID, AP_PASSWORD, WIFI_CHANNEL, 0);

    // Disable AMPDU RX on the ESP32 WiFi to fix a bug on Android
    esp_wifi_stop();
    esp_wifi_deinit();
    wifi_init_config_t my_config = WIFI_INIT_CONFIG_DEFAULT();
    my_config.ampdu_rx_enable = false;
    esp_wifi_init(&my_config);
    esp_wifi_start();
    vTaskDelay(100 / portTICK_PERIOD_MS);  // Add a small delay
  }

  void setUpDNSServer() {
    dnsServer.setTTL(3600);
    dnsServer.start(53, "*", localIP);
  }

  void setUpWebserver() {
    // Windows 11 captive portal workaround
    server.on("/connecttest.txt", [this]() {
      server.sendHeader("Location", "http://logout.net", true);
      server.send(302, "text/plain", "");
    });

    // Prevents Windows 10 from repeatedly calling this
    server.on("/wpad.dat", [this]() {
      server.send(404, "text/plain", "");
    });

    // Common captive portal detection endpoints
    server.on("/generate_204", [this]() {
      server.sendHeader("Location", localIPURL, true);
      server.send(302, "text/plain", "");
    });  // android captive portal redirect

    server.on("/redirect", [this]() {
      server.sendHeader("Location", localIPURL, true);
      server.send(302, "text/plain", "");
    });  // microsoft redirect

    server.on("/hotspot-detect.html", [this]() {
      server.sendHeader("Location", localIPURL, true);
      server.send(302, "text/plain", "");
    });  // apple call home

    server.on("/canonical.html", [this]() {
      server.sendHeader("Location", localIPURL, true);
      server.send(302, "text/plain", "");
    });  // firefox captive portal call home

    server.on("/success.txt", [this]() {
      server.send(200, "text/plain", "Success");
    });  // firefox captive portal call home

    server.on("/ncsi.txt", [this]() {
      server.sendHeader("Location", localIPURL, true);
      server.send(302, "text/plain", "");
    });  // windows call home

    // Return 404 for favicon requests
    server.on("/favicon.ico", [this]() {
      server.send(404, "text/plain", "");
    });

    // Main configuration page
    server.on("/", HTTP_GET, [this]() {
      handleRoot();
    });

    // Handle form submission
    server.on("/save", HTTP_POST, [this]() {
      handleSave();
    });

    // Catch-all handler for other requests - redirect to main page
    server.onNotFound([this]() {
      server.sendHeader("Location", localIPURL, true);
      server.send(302, "text/plain", "");
    });
  }

  void handleRoot() {
    String html = String(configHTML);

    // Replace placeholders
    html.replace("%SSID%", storedSSID);
    html.replace("%PASSWORD%", storedPassword);
    html.replace("%MESSAGE%", "");  // No message on initial load

    // Add cache control headers to prevent caching
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "-1");

    server.send(200, "text/html", html);
  }

  void handleSave() {
    if (server.hasArg("ssid")) {
      String newSSID = server.arg("ssid");
      String newPassword = server.arg("password");

      // Validate SSID
      if (newSSID.length() == 0) {
        String html = String(configHTML);
        html.replace("%SSID%", storedSSID);
        html.replace("%PASSWORD%", storedPassword);
        html.replace("%MESSAGE%", "<div class=\"error\"><strong>Error:</strong> SSID cannot be empty!</div>");

        server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        server.sendHeader("Pragma", "no-cache");
        server.sendHeader("Expires", "-1");
        server.send(400, "text/html", html);
        return;
      }

      // Save to preferences
      preferences.putString("ssid", newSSID);
      preferences.putString("password", newPassword);

      // Update stored values
      storedSSID = newSSID;
      storedPassword = newPassword;
      credentialsChanged = true;

      // Send success page
      String successPage = String(successHTML);
      successPage.replace("%SSID%", newSSID);

      server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
      server.sendHeader("Pragma", "no-cache");
      server.sendHeader("Expires", "-1");
      server.send(200, "text/html", successPage);

    } else {
      server.send(400, "text/plain", "Missing SSID parameter");
    }
  }
};