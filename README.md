# 🎵 TTGO Spotify Remote

A compact **Spotify controller and now-playing display** built around the **LILYGO TTGO T-Display V1.1 (ESP32)**.

The ESP32 connects over Wi-Fi to a lightweight Python bridge running on a computer. The bridge communicates with the Spotify Web API, retrieves the currently playing track, downloads and resizes the album artwork, and exposes simple HTTP endpoints for the ESP32.

The TTGO displays the current song, artist, album artwork, and playback progress while two physical buttons control the previous and next tracks.

---

## ✨ Features

- 🎵 Display the currently playing Spotify track
- 👤 Display artist name
- 🖼️ Display Spotify album artwork
- ⏱️ Display playback progress
- ⏮️ Physical previous-track button
- ⏭️ Physical next-track button
- 📡 Wi-Fi connectivity
- 🐍 Python Spotify bridge
- 🔐 Spotify OAuth token support
- 🔄 Automatic Spotify access-token refresh
- 🧠 Album artwork resized on the computer to reduce ESP32 memory usage
- ⚡ Lightweight HTTP communication between ESP32 and PC
- 📱 Designed for the 135 × 240 TTGO T-Display V1.1
- 💾 No Spotify credentials are stored on the ESP32

---

# 📸 Project Overview

The project consists of two main components:

```text
┌──────────────────────┐
│       Spotify       │
│                      │
│   Spotify Web API    │
└──────────┬───────────┘
           │
           │ HTTPS
           ▼
┌─────────────────────────────┐
│       Python Bridge         │
│                             │
│  OAuth Token Management     │
│  Now Playing API            │
│  Previous / Next            │
│  Album Art Download         │
│  Album Art Resize           │
└─────────────┬───────────────┘
              │
              │ HTTP / Wi-Fi
              ▼
┌─────────────────────────────┐
│     TTGO T-Display         │
│          ESP32              │
│                             │
│  Album Artwork              │
│  Song Title                 │
│  Artist                     │
│  Progress                   │
│                             │
│  [Previous]    [Next]       │
└─────────────────────────────┘

🧰 Hardware
Required
- LILYGO / TTGO T-Display V1.1
- Computer running the Python Spotify bridge
- Wi-Fi network
- Spotify account
The project uses the buttons already present on the TTGO T-Display.
Button GPIOs
Function	GPIO
Previous Track	GPIO 35
Next Track	GPIO 0


⚠️ GPIO 0 is an ESP32 boot-strapping pin. Do not hold the GPIO 0 button while powering on, resetting, or uploading firmware.

💻 Software
ESP32
The firmware is written in:
- Arduino C++
- Arduino IDE
- ESP32 Arduino Core
Libraries
The firmware uses:
- WiFi
- HTTPClient
- ArduinoJson
- TFT_eSPI
- TJpg_Decoder
Arduino Libraries
Install the following libraries using the Arduino IDE Library Manager:
ArduinoJson
TFT_eSPI
TJpg_Decoder

🐍 Python Bridge
The PC-side bridge is written in Python.
It handles:
- Spotify authentication
- Spotify API requests
- Access-token refresh
- Currently playing information
- Previous track
- Next track
- Album artwork downloading
- Album artwork resizing
- HTTP communication with the ESP32
Python dependencies
Install Pillow:
pip install Pillow

The bridge otherwise uses Python's standard library for HTTP and JSON handling.
📁 Project Structure
A recommended repository structure is:
TTGO-Spotify-Remote/
│
├── README.md
│
├── ESP32/
│   └── TTGO_Spotify_Remote/
│       └── TTGO_Spotify_Remote.ino
│
├── Python/
│   └── spotify_bridge.py
│
├── images/
│   └── ...
│
└── .gitignore

🔑 Spotify Developer Setup
The project uses the Spotify Web API.
You need to create your own Spotify Developer application.
1. Create a Spotify Developer App
Go to the Spotify Developer Dashboard:
https://developer.spotify.com/dashboard
Create a new application.
2. Get your Client ID
After creating the application, Spotify will provide a:
Client ID

Do not publish your Client Secret or personal OAuth tokens.
3. Configure the Redirect URI
The project uses:
http://127.0.0.1:8888/callback

Add this exact redirect URI to your Spotify Developer application's Redirect URIs.
🔐 Authentication
The Python bridge uses Spotify OAuth authentication.
The bridge stores authentication information in:
spotify_token.json

The token file should never be committed to GitHub.
Add it to .gitignore:
spotify_token.json

If you accidentally publish a Spotify token or client secret, revoke it immediately through the Spotify Developer Dashboard.
⚙️ Python Bridge Configuration
Open:
Python/spotify_bridge.py

Configure your Spotify Client ID:
CLIENT_ID = "YOUR_SPOTIFY_CLIENT_ID"


The bridge listens on:
0.0.0.0:8765

The default port is:
8765

🌐 Network Configuration
The ESP32 communicates with the computer running the Python bridge over the local network.
For example:
Computer IP:
192.168.1.11

Bridge:
192.168.1.11:8765

In the ESP32 firmware:
const char* SERVER = "192.168.1.11";
const int SERVER_PORT = 8765;

Change the IP address to the local IP address of the computer running the bridge.
Finding your computer's IP
On Windows:
ipconfig

Look for:
IPv4 Address

For example:
IPv4 Address. . . . . . : 192.168.1.11

🐍 Running the Spotify Bridge
Navigate to the Python directory:
cd Python

Run:
python spotify_bridge.py

The bridge should start on:
http://localhost:8765

The ESP32 connects to the computer using the computer's LAN IP address.
📡 API Endpoints
The Python bridge exposes the following endpoints.
Currently Playing
GET /now-playing

Example:
http://192.168.1.11:8765/now-playing

Example response:
{
    "playing": true,
    "song": "Example Song",
    "artist": "Example Artist",
    "album": "Example Album",
    "album_art": "https://...",
    "progress": 42000,
    "duration": 210000
}

Previous Track
POST /previous

Example:
http://192.168.1.11:8765/previous

The bridge sends the corresponding request to Spotify.
Next Track
POST /next

Example:
http://192.168.1.11:8765/next

Album Artwork
GET /album-art

Example:
http://192.168.1.11:8765/album-art

The bridge downloads the current Spotify artwork and resizes it before sending it to the ESP32.
🖼️ Why Album Artwork Is Resized
Spotify album artwork can be relatively large.
Sending the original artwork directly to the ESP32 can cause memory problems.
For example:
Spotify artwork
      │
      │ 640 × 640
      │
      ▼
Python Bridge
      │
      │ resize
      ▼
120 × 120 JPEG
      │
      ▼
ESP32

This dramatically reduces the amount of memory required by the ESP32.
The bridge uses Pillow to resize the image:
image = image.resize(    (120, 120),    Image.Resampling.LANCZOS)


The resulting image is encoded as JPEG before being sent to the ESP32.
📟 ESP32 Display
The TTGO T-Display is configured in vertical orientation.
Display resolution:
135 × 240

The firmware uses:
tft.setRotation(0);

The display contains:
┌───────────────────┐
│                   │
│   ┌─────────────┐ │
│   │             │ │
│   │   ALBUM     │ │
│   │    ART      │ │
│   │             │ │
│   └─────────────┘ │
│                   │
│ Song Title        │
│ Artist             │
│                   │
│ 0:42        3:24  │
│ ━━━━━━━━━━━━━━━   │
│                   │
└───────────────────┘

The physical buttons are used for track control and are not displayed as on-screen buttons.
🎛️ Button Configuration
The TTGO buttons are configured as:
#define BUTTON_PREVIOUS 35
#define BUTTON_NEXT 0

Previous
GPIO 35

Next
GPIO 0

GPIO 0 is configured with the ESP32's internal pull-up.
GPIO 35 does not have an internal pull-up on the ESP32, so its electrical behavior depends on the T-Display's existing button circuitry.
🔄 How It Works
When the ESP32 starts:
1. ESP32 powers on
       ↓
2. Connects to Wi-Fi
       ↓
3. Requests /now-playing
       ↓
4. Python bridge contacts Spotify
       ↓
5. Spotify returns current track
       ↓
6. ESP32 receives song information
       ↓
7. ESP32 requests /album-art
       ↓
8. Python downloads Spotify artwork
       ↓
9. Python resizes artwork to 120×120
       ↓
10. ESP32 displays artwork

The ESP32 periodically requests the currently playing information.
⏭️ Changing Tracks
When the Next button is pressed:
TTGO GPIO 0
      ↓
ESP32
      ↓
POST /next
      ↓
Python Bridge
      ↓
Spotify Web API
      ↓
Next Track

For Previous:
TTGO GPIO 35
      ↓
ESP32
      ↓
POST /previous
      ↓
Python Bridge
      ↓
Spotify Web API
      ↓
Previous Track

🔄 Spotify Token Refresh
Spotify access tokens expire.
The Python bridge automatically refreshes the access token using the stored refresh token.
The bridge detects an expired token when Spotify returns:
401 Unauthorized

It then:
Expired access token
        ↓
Refresh token
        ↓
Spotify token endpoint
        ↓
New access token
        ↓
Retry original request

This allows the bridge to continue working without requiring manual authentication every time the access token expires.
🛠️ Arduino IDE Setup
1. Install Arduino IDE
Download Arduino IDE from:
https://www.arduino.cc/en/software/
2. Install ESP32 Board Support
In Arduino IDE:
File
→ Preferences
→ Additional Boards Manager URLs

Add the ESP32 board package URL if it isn't already installed.
Then:
Tools
→ Board
→ Boards Manager

Search for:
esp32

Install the ESP32 package.
🧩 Select the Board
For the TTGO T-Display V1.1, use:
ESP32 Dev Module

Example:
Tools
→ Board
→ ESP32 Arduino
→ ESP32 Dev Module

📚 Configure TFT_eSPI
Install:
TFT_eSPI

The TTGO T-Display requires the correct TFT_eSPI configuration for the board.
Make sure the TFT pins and display driver match your T-Display V1.1 configuration.
If the display is blank or shows incorrect colors, check the TFT_eSPI setup before troubleshooting the Spotify code.
📤 Uploading the Firmware
Connect the TTGO T-Display to your computer using USB.
Select:
Tools → Board → ESP32 Dev Module

Select the correct COM port:
Tools → Port

Then click:
Upload

Open Serial Monitor at:
115200 baud

You should see something similar to:
TTGO SPOTIFY REMOTE
VERTICAL MODE

Connecting to WiFi....
WiFi CONNECTED!

ESP32 IP: 192.168.1.18

Getting Spotify data...

🐛 Troubleshooting
ESP32 cannot connect to Wi-Fi
Check:
- Wi-Fi SSID
- Wi-Fi password
- ESP32 is within Wi-Fi range
- Computer and ESP32 are on the same local network
The ESP32 should print:
WiFi CONNECTED!

ESP32 cannot connect to Python bridge
Check that the Python bridge is running:
python spotify_bridge.py

Then open:
http://192.168.1.11:8765/now-playing

from another device on the same network.
Also verify:
const char* SERVER = "192.168.1.11";

matches the computer running the bridge.
Next / Previous buttons don't work
Open the ESP32 Serial Monitor.
When pressing Next you should see:
NEXT BUTTON
Next URL: http://192.168.1.11:8765/next
Next HTTP: 200

or:
Next HTTP: 204

If you see:
Next HTTP: 501

make sure the Python bridge contains the do_POST() handler.
Album artwork doesn't display
Check:
/album-art

in a browser:
http://192.168.1.11:8765/album-art

If an image appears, the Python bridge is successfully retrieving Spotify artwork.
If the ESP32 reports:
Not enough RAM!

make sure the Python bridge is resizing the image to:
120 × 120

and the ESP32 is using:
TJpgDec.setJpgScale(1);

Spotify authentication fails
Check:
- Spotify Client ID
- Redirect URI
- spotify_token.json
- Spotify Developer Dashboard
- Internet connection
The redirect URI used by this project is:
http://127.0.0.1:8888/callback

🔒 Security
Do not commit these files to GitHub:
spotify_token.json

Do not publish:
Spotify Client Secret
Spotify access token
Spotify refresh token

Your .gitignore should contain at least:
spotify_token.json
__pycache__/
*.pyc
.env
.vscode/

If you use a .env file for credentials, do not commit it.
📄 Example .gitignore
# Spotify authentication
spotify_token.json

# Python
__pycache__/
*.py[cod]

# Virtual environments
venv/
.venv/
env/

# Environment variables
.env

# IDE
.vscode/
.idea/

# OS files
.DS_Store
Thumbs.db

# Arduino
*.bin
*.elf

🚀 Future Improvements
Possible future additions:
- [ ] Better album-art positioning
- [ ] Smooth album-art transitions
- [ ] Animated progress bar
- [ ] Play / pause button
- [ ] Volume control
- [ ] Shuffle control
- [ ] Repeat control
- [ ] Track scrolling / marquee
- [ ] Multiple-line song titles
- [ ] Album name display
- [ ] Spotify device selection
- [ ] Wi-Fi configuration screen
- [ ] Connection status screen
- [ ] Offline/error UI
- [ ] Custom animations
- [ ] Touch controls
- [ ] Battery-powered version
- [ ] 3D-printed enclosure
🧪 Development Notes
This project intentionally separates Spotify API communication from the ESP32.
The ESP32 does not directly authenticate with Spotify.
Instead:
ESP32
  ↓
Local Python Bridge
  ↓
Spotify Web API

This keeps Spotify authentication and token management on the computer while keeping the ESP32 firmware relatively lightweight.
The Python bridge also handles album-art processing because the ESP32 has significantly less available memory than a desktop computer.
📜 License
Choose a license appropriate for your project.
For example, MIT:
MIT License

Copyright (c) 2026 Yashas Padmashali

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files, to deal in the Software
without restriction, including without limitation the rights to use, copy,
modify, merge, publish, distribute, sublicense, and/or sell copies of the
Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

🙌 Acknowledgements
This project uses:
- ESP32
- LILYGO / TTGO T-Display
- Arduino
- TFT_eSPI
- TJpg_Decoder
- ArduinoJson
- Python
- Pillow
- Spotify Web API
Spotify is a trademark of Spotify AB. This project is an independent hobby project and is not affiliated with or endorsed by Spotify.
⭐ Project
A small physical Spotify remote built to explore:
- Embedded systems
- ESP32 development
- Wi-Fi communication
- REST APIs
- OAuth authentication
- Python backend development
- Display graphics
- JPEG decoding
- Hardware button interfaces
- Embedded memory optimization
Built with an ESP32, a tiny TFT display, and a lot of experimentation.

### One thing I'd change before you publish

Don't put your actual Spotify Client ID in the GitHub version of the Python file if you can avoid it. Your current bridge contains a Client ID in the configuration, and the token file must definitely stay private. :chatgpt-content-reference{index="0"}

For a polished repository, I'd also make the Python bridge read the Client ID from an environment variable, so the GitHub repo contains:

```python
CLIENT_ID = os.getenv("SPOTIFY_CLIENT_ID")
