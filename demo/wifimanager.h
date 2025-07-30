#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

class WiFiManager {
private:
  WebServer server;
  Preferences preferences;
  bool apRunning;
  bool credentialsChanged;
  String storedSSID;
  String storedPassword;

  // HTML page for WiFi configuration
  const char* configHTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Clock WiFi Setup</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 20px;
            background-color: #f0f0f0;
        }
        .container {
            max-width: 400px;
            margin: 0 auto;
            background-color: white;
            padding: 30px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        h1 {
            text-align: center;
            color: #333;
            margin-bottom: 30px;
        }
        .form-group {
            margin-bottom: 20px;
        }
        label {
            display: block;
            margin-bottom: 5px;
            color: #555;
            font-weight: bold;
        }
        input[type="text"], input[type="password"] {
            width: 100%;
            padding: 12px;
            border: 2px solid #ddd;
            border-radius: 5px;
            font-size: 16px;
            box-sizing: border-box;
        }
        input[type="text"]:focus, input[type="password"]:focus {
            border-color: #4CAF50;
            outline: none;
        }
        .button {
            width: 100%;
            background-color: #4CAF50;
            color: white;
            padding: 14px;
            border: none;
            border-radius: 5px;
            font-size: 16px;
            cursor: pointer;
            margin-top: 10px;
        }
        .button:hover {
            background-color: #45a049;
        }
        .info {
            background-color: #e7f3ff;
            border: 1px solid #b3d9ff;
            padding: 15px;
            border-radius: 5px;
            margin-bottom: 20px;
            color: #31708f;
        }
        .success {
            background-color: #d4edda;
            border: 1px solid #c3e6cb;
            padding: 15px;
            border-radius: 5px;
            margin-bottom: 20px;
            color: #155724;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>GeniClock WiFi Setup</h1>
        
        <div class="info">
            <strong>Instructions:</strong><br>
            Enter your WiFi network credentials below. The clock will use these to connect to the internet for automatic time synchronization via NTP.
        </div>
        
        %MESSAGE%
        
        <form action="/save" method="POST">
            <div class="form-group">
                <label for="ssid">WiFi Network Name (SSID):</label>
                <input type="text" id="ssid" name="ssid" value="%SSID%" required>
            </div>
            
            <div class="form-group">
                <label for="password">WiFi Password:</label>
                <input type="password" id="password" name="password" value="%PASSWORD%">
            </div>
            
            <button type="submit" class="button">Save WiFi Settings</button>
        </form>
        
        <div style="margin-top: 30px; text-align: center; color: #666; font-size: 14px;">
            <p>After saving, the clock will restart and connect to your WiFi network.</p>
        </div>
    </div>
</body>
</html>
)rawliteral";

  // Success page HTML
  const char* successHTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>GeniClock WiFi Setup - Success</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta http-equiv="refresh" content="10;url=/">
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 20px;
            background-color: #f0f0f0;
        }
        .container {
            max-width: 400px;
            margin: 0 auto;
            background-color: white;
            padding: 30px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            text-align: center;
        }
        h1 {
            color: #4CAF50;
            margin-bottom: 20px;
        }
        .success-icon {
            font-size: 64px;
            color: #4CAF50;
            margin-bottom: 20px;
        }
        .message {
            color: #333;
            line-height: 1.6;
            margin-bottom: 20px;
        }
        .countdown {
            color: #666;
            font-size: 14px;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="success-icon">✓</div>
        <h1>Settings Saved!</h1>
        <div class="message">
            <p><strong>WiFi credentials have been saved successfully.</strong></p>
            <p>SSID: <strong>%SSID%</strong></p>
            <p>The clock will now restart and connect to your WiFi network.</p>
        </div>
        <div class="countdown">
            This page will automatically refresh in 10 seconds...
        </div>
    </div>
</body>
</html>
)rawliteral";

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

    // Start AP with fixed IP
    WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
    WiFi.softAP("CLOCK", "");  // Open AP, no password

    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());

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

    Serial.println("Web server started");
    Serial.println("Connect to 'CLOCK' WiFi network");
    Serial.println("Open browser and go to 192.168.4.1");
  }

  void stopAP() {
    if (!apRunning) return;

    Serial.println("Stopping Access Point...");
    server.stop();
    WiFi.softAPdisconnect(true);
    apRunning = false;
  }

  void handleClient() {
    if (apRunning) {
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
        html.replace("%MESSAGE%", "<div class=\"error\" style=\"background-color: #f8d7da; border: 1px solid #f5c6cb; color: #721c24;\"><strong>Error:</strong> SSID cannot be empty!</div>");

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