#pragma once

// HTML page for WiFi configuration
const char *configHTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>GeniClock WiFi Setup</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=yes">
    <style>
        * {
            box-sizing: border-box;
        }
        
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 10px;
            background-color: #f0f0f0;
            min-height: 100vh;
            overflow-x: hidden;
        }
        
        .container {
            max-width: 400px;
            margin: 0 auto;
            background-color: white;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            min-height: calc(100vh - 40px);
        }
        
        h1 {
            text-align: center;
            color: #333;
            margin-bottom: 20px;
            font-size: 24px;
        }
        
        .form-group {
            margin-bottom: 20px;
        }
        
        label {
            display: block;
            margin-bottom: 8px;
            color: #555;
            font-weight: bold;
            font-size: 14px;
        }
        
        input[type="text"] {
            width: 100%;
            padding: 15px;
            border: 2px solid #ddd;
            border-radius: 8px;
            font-size: 16px;
            background-color: #fff;
            transition: border-color 0.3s ease;
            -webkit-appearance: none;
            -moz-appearance: none;
            appearance: none;
        }
        
        input[type="text"]:focus {
            border-color: #4CAF50;
            outline: none;
            box-shadow: 0 0 5px rgba(76, 175, 80, 0.3);
        }
        
        .button {
            width: 100%;
            background-color: #4CAF50;
            color: white;
            padding: 15px;
            border: none;
            border-radius: 8px;
            font-size: 16px;
            font-weight: bold;
            cursor: pointer;
            margin-top: 10px;
            transition: background-color 0.3s ease;
            -webkit-appearance: none;
            -moz-appearance: none;
            appearance: none;
        }
        
        .button:hover {
            background-color: #45a049;
        }
        
        .button:active {
            background-color: #3d8b40;
        }
        
        .info {
            background-color: #e7f3ff;
            border: 1px solid #b3d9ff;
            padding: 15px;
            border-radius: 8px;
            margin-bottom: 20px;
            color: #31708f;
            font-size: 14px;
            line-height: 1.4;
        }
        
        .success {
            background-color: #d4edda;
            border: 1px solid #c3e6cb;
            padding: 15px;
            border-radius: 8px;
            margin-bottom: 20px;
            color: #155724;
        }
        
        .error {
            background-color: #f8d7da;
            border: 1px solid #f5c6cb;
            padding: 15px;
            border-radius: 8px;
            margin-bottom: 20px;
            color: #721c24;
        }
        
        .footer-info {
            margin-top: 25px;
            text-align: center;
            color: #666;
            font-size: 13px;
            line-height: 1.4;
            padding-bottom: 20px;
        }
        
        /* Mobile specific improvements */
        @media screen and (max-width: 480px) {
            body {
                padding: 5px;
            }
            
            .container {
                padding: 15px;
                border-radius: 5px;
                margin: 0;
                min-height: calc(100vh - 10px);
            }
            
            h1 {
                font-size: 20px;
                margin-bottom: 15px;
            }
            
            input[type="text"] {
                padding: 12px;
                font-size: 16px; /* Prevents zoom on iOS */
            }
            
            .button {
                padding: 12px;
                font-size: 16px;
            }
            
            .form-group {
                margin-bottom: 15px;
            }
        }
        
        /* Ensure form is scrollable when keyboard appears */
        .form-container {
            overflow-y: auto;
            max-height: 100vh;
        }
    </style>
</head>
<body>
    <div class="form-container">
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
                    <input type="text" 
                           id="ssid" 
                           name="ssid" 
                           value="%SSID%" 
                           required 
                           autocomplete="off"
                           autocapitalize="none"
                           autocorrect="off"
                           spellcheck="false">
                </div>
                
                <div class="form-group">
                    <label for="password">WiFi Password:</label>
                    <input type="text" 
                           id="password" 
                           name="password" 
                           value="%PASSWORD%"
                           autocomplete="off"
                           autocapitalize="none"
                           autocorrect="off"
                           spellcheck="false"
                           placeholder="Leave empty if no password">
                </div>
                
                <button type="submit" class="button">Save WiFi Settings</button>
            </form>
            
            <div class="footer-info">
                <p>After saving, the clock will restart and connect to your WiFi network.</p>
                <p><strong>Note:</strong> Password is shown in plain text for easier entry.</p>
            </div>
        </div>
    </div>
</body>
</html>
)rawliteral";

// Success page HTML
const char *successHTML = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>GeniClock WiFi Setup - Success</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=yes">
    <meta http-equiv="refresh" content="10;url=/">
    <style>
        * {
            box-sizing: border-box;
        }
        
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 10px;
            background-color: #f0f0f0;
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
        }
        
        .container {
            max-width: 400px;
            width: 100%;
            background-color: white;
            padding: 30px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
            text-align: center;
        }
        
        h1 {
            color: #4CAF50;
            margin-bottom: 20px;
            font-size: 24px;
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
            font-size: 16px;
        }
        
        .ssid-display {
            background-color: #f8f9fa;
            padding: 10px;
            border-radius: 5px;
            margin: 10px 0;
            font-family: monospace;
            word-break: break-all;
        }
        
        .countdown {
            color: #666;
            font-size: 14px;
            margin-top: 20px;
        }
        
        @media screen and (max-width: 480px) {
            body {
                padding: 5px;
            }
            
            .container {
                padding: 20px;
                border-radius: 5px;
            }
            
            h1 {
                font-size: 20px;
            }
            
            .success-icon {
                font-size: 48px;
            }
            
            .message {
                font-size: 14px;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="success-icon">✓</div>
        <h1>Settings Saved!</h1>
        <div class="message">
            <p><strong>WiFi credentials have been saved successfully.</strong></p>
            <p>Network:</p>
            <div class="ssid-display">%SSID%</div>
            <p>The clock will now restart and connect to your WiFi network.</p>
        </div>
        <div class="countdown">
            This page will automatically refresh in 10 seconds...
        </div>
    </div>
</body>
</html>
)rawliteral";