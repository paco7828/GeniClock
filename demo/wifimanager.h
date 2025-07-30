#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <DNSServer.h>
#include <html-pages.h>

class WiFiManager {
private:
  WebServer server;
  DNSServer dnsServer;
  Preferences preferences;
  bool apRunning;
  bool credentialsChanged;
  String storedSSID;
  String storedPassword;

public:
  WiFiManager()
    : server(80), apRunning(false), credentialsChanged(false) {}

  void begin() {
    // Initialize preferences
    preferences.begin("wifi", false);

    // Load stored credentials
    storedSSID = preferences.getString("ssid", "");
    storedPassword = preferences.getString("password", "");

    Serial.println("WiFiManager initialized");
    if (hasStoredCredentials()) {
      Serial.println("Found stored WiFi credentials");
      Serial.println("SSID: " + storedSSID);
      Serial.println("PASSW: " + storedPassword);
    } else {
      Serial.println("No stored WiFi credentials found");
    }
  }

  bool hasStoredCredentials() {
    return (storedSSID.length() > 0);
  }

  void getStoredCredentials(String& ssid, String& password) {
    ssid = storedSSID;
    password = storedPassword;
  }

  void startAP() {
    if (apRunning) return;

    Serial.println("Starting Access Point...");

    // Stop any existing WiFi connection
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);

    // Start AP with password
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    WiFi.softAP("CLOCK", "clock123");  // Change to const SSID and PASSW from constants.h

    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());

    // Start DNS server for captive portal
    dnsServer.start(53, "*", WiFi.softAPIP());

    // Setup web server routes
    server.on("/", HTTP_GET, [this]() {
      handleRoot();
    });

    server.on("/save", HTTP_POST, [this]() {
      handleSave();
    });

    // Captive portal - redirect all requests to root
    server.onNotFound([this]() {
      server.sendHeader("Location", "http://192.168.4.1/", true);
      server.send(302, "text/plain", "");
    });

    server.begin();
    apRunning = true;
    credentialsChanged = false;

    Serial.println("Web server started with captive portal");
    Serial.println("Connect to 'CLOCK' WiFi network with password 'clock123'");
    Serial.println("Captive portal will automatically open or go to 192.168.4.1");
  }

  void stopAP() {
    if (!apRunning) return;

    Serial.println("Stopping Access Point...");
    dnsServer.stop();
    server.stop();
    WiFi.softAPdisconnect(true);
    apRunning = false;
  }

  void handleClient() {
    if (apRunning) {
      dnsServer.processNextRequest();
      server.handleClient();
    }
  }

  bool credentialsUpdated() {
    return credentialsChanged;
  }

private:
  void handleRoot() {
    Serial.println("Serving config page");

    String html = String(configHTML);

    // Replace placeholders
    html.replace("%SSID%", storedSSID);
    html.replace("%PASSWORD%", storedPassword);
    html.replace("%MESSAGE%", "");  // No message on initial load

    server.send(200, "text/html", html);
  }

  void handleSave() {
    Serial.println("Processing save request");

    if (server.hasArg("ssid")) {
      String newSSID = server.arg("ssid");
      String newPassword = server.arg("password");

      Serial.println("Received credentials:");
      Serial.println("SSID: " + newSSID);
      Serial.println("Password: [" + String(newPassword.length()) + " characters]");

      // Validate SSID
      if (newSSID.length() == 0) {
        String html = String(configHTML);
        html.replace("%SSID%", storedSSID);
        html.replace("%PASSWORD%", storedPassword);
        html.replace("%MESSAGE%", "<div class=\"error\"><strong>Error:</strong> SSID cannot be empty!</div>");

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

      Serial.println("Credentials saved to flash memory");

      // Send success page
      String successPage = String(successHTML);
      successPage.replace("%SSID%", newSSID);
      server.send(200, "text/html", successPage);

    } else {
      server.send(400, "text/plain", "Missing SSID parameter");
    }
  }
};