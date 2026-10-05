#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>

// ============================================================
// WIFI
// ============================================================

const char* WIFI_SSID = "Airtel_yash_5430";
const char* WIFI_PASSWORD = "air81283";

// ============================================================
// SPOTIFY BRIDGE
// ============================================================

const char* SERVER = "192.168.1.11";
const int SERVER_PORT = 8765;

// ============================================================
// T-Display V1.1 BUTTONS
// ============================================================

#define BUTTON_PREVIOUS 35
#define BUTTON_NEXT       0

// ============================================================
// DISPLAY
// ============================================================

TFT_eSPI tft = TFT_eSPI();

// Vertical orientation
#define SCREEN_W 135
#define SCREEN_H 240

// ============================================================
// ALBUM ART
// ============================================================

// We'll deal with exact album-art alignment later.
// These are deliberately simple for now.

#define ART_X 7
#define ART_Y 5

#define ART_SIZE 125

// ============================================================
// SPOTIFY DATA
// ============================================================

String currentSong = "";
String currentArtist = "";
String currentAlbumArtURL = "";

String lastSong = "";
String lastAlbumArtURL = "";

int currentProgress = 0;
int currentDuration = 0;

bool albumArtLoaded = false;

// ============================================================
// BUTTON TIMING
// ============================================================

unsigned long lastPreviousPress = 0;
unsigned long lastNextPress = 0;

const unsigned long BUTTON_DELAY = 350;

// ============================================================
// UPDATE TIMING
// ============================================================

unsigned long lastSpotifyUpdate = 0;

const unsigned long SPOTIFY_UPDATE_INTERVAL = 3000;

// ============================================================
// FORWARD DECLARATIONS
// ============================================================

void getSpotifyData();
bool loadAlbumArt();

void drawSpotifyUI();
void drawAlbumPlaceholder();

void drawSongTitle();
void drawArtist();
void drawProgressBar();

void previousTrack();
void nextTrack();


// ============================================================
// JPEG CALLBACK
// ============================================================
//
// Safe clipping for TJpg_Decoder.
//
// This prevents JPEG blocks from being pushed outside
// the physical TFT boundaries.
// ============================================================

bool tftOutput(
  int16_t x,
  int16_t y,
  uint16_t w,
  uint16_t h,
  uint16_t* bitmap
)
{
  // Completely outside screen
  if (
    x >= SCREEN_W ||
    y >= SCREEN_H ||
    x + (int16_t)w <= 0 ||
    y + (int16_t)h <= 0
  )
  {
    return true;
  }

  // Visible region
  int16_t visibleX = max((int16_t)0, x);
  int16_t visibleY = max((int16_t)0, y);

  int16_t right = min(
    (int16_t)SCREEN_W,
    (int16_t)(x + w)
  );

  int16_t bottom = min(
    (int16_t)SCREEN_H,
    (int16_t)(y + h)
  );

  int16_t visibleW = right - visibleX;
  int16_t visibleH = bottom - visibleY;

  if (
    visibleW <= 0 ||
    visibleH <= 0
  )
  {
    return true;
  }

  // ----------------------------------------------------------
  // Normal case - no clipping required
  // ----------------------------------------------------------

  if (
    visibleX == x &&
    visibleY == y &&
    visibleW == w &&
    visibleH == h
  )
  {
    tft.pushImage(
      x,
      y,
      w,
      h,
      bitmap
    );

    return true;
  }

  // ----------------------------------------------------------
  // Clipping required
  // ----------------------------------------------------------

  uint16_t* clippedBitmap =
    (uint16_t*)malloc(
      visibleW *
      visibleH *
      sizeof(uint16_t)
    );

  if (clippedBitmap == nullptr)
  {
    Serial.println(
      "JPEG clipping memory allocation failed!"
    );

    return false;
  }

  int16_t sourceX =
    visibleX - x;

  int16_t sourceY =
    visibleY - y;

  for (
    int16_t row = 0;
    row < visibleH;
    row++
  )
  {
    for (
      int16_t col = 0;
      col < visibleW;
      col++
    )
    {
      clippedBitmap[
        row * visibleW + col
      ] =
        bitmap[
          (sourceY + row) * w +
          (sourceX + col)
        ];
    }
  }

  tft.pushImage(
    visibleX,
    visibleY,
    visibleW,
    visibleH,
    clippedBitmap
  );

  free(clippedBitmap);

  return true;
}


// ============================================================
// SETUP
// ============================================================

void setup()
{
  Serial.begin(115200);

  delay(500);

  Serial.println();
  Serial.println("==============================");
  Serial.println(" TTGO SPOTIFY REMOTE");
  Serial.println(" VERTICAL MODE");
  Serial.println("==============================");

  // ----------------------------------------------------------
  // BUTTONS
  // ----------------------------------------------------------

  // GPIO35 has NO internal pull-up/pull-down.
  pinMode(
    BUTTON_PREVIOUS,
    INPUT
  );

  // GPIO0 can use internal pull-up.
  pinMode(
    BUTTON_NEXT,
    INPUT_PULLUP
  );

  // ----------------------------------------------------------
  // TFT
  // ----------------------------------------------------------

  tft.init();

  // Vertical
  tft.setRotation(0);

  tft.setTextWrap(false);

  // RGB565 byte order
  tft.setSwapBytes(true);

  tft.fillScreen(TFT_BLACK);

  // ----------------------------------------------------------
  // JPEG
  // ----------------------------------------------------------

  TJpgDec.setCallback(
    tftOutput
  );

  // Scale 4
  TJpgDec.setJpgScale(1);

  // ----------------------------------------------------------
  // Initial screen
  // ----------------------------------------------------------

  drawAlbumPlaceholder();

  // ----------------------------------------------------------
  // WIFI
  // ----------------------------------------------------------

  WiFi.mode(WIFI_STA);

  WiFi.begin(
    WIFI_SSID,
    WIFI_PASSWORD
  );

  Serial.print(
    "Connecting to WiFi"
  );

  unsigned long wifiStart =
    millis();

  while (
    WiFi.status() != WL_CONNECTED &&
    millis() - wifiStart < 20000
  )
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println();

  if (
    WiFi.status() == WL_CONNECTED
  )
  {
    Serial.println(
      "WiFi CONNECTED!"
    );

    Serial.print(
      "ESP32 IP: "
    );

    Serial.println(
      WiFi.localIP()
    );
  }
  else
  {
    Serial.println(
      "WiFi FAILED!"
    );

    Serial.print(
      "WiFi status: "
    );

    Serial.println(
      WiFi.status()
    );
  }

  // ----------------------------------------------------------
  // Get current Spotify track
  // ----------------------------------------------------------

  if (
    WiFi.status() == WL_CONNECTED
  )
  {
    getSpotifyData();
  }
}


// ============================================================
// LOOP
// ============================================================

void loop()
{
  // ==========================================================
  // WIFI RECONNECT
  // ==========================================================

  if (
    WiFi.status() != WL_CONNECTED
  )
  {
    static unsigned long lastReconnect =
      0;

    if (
      millis() - lastReconnect > 5000
    )
    {
      lastReconnect = millis();

      Serial.println(
        "WiFi disconnected."
      );

      Serial.println(
        "Reconnecting..."
      );

      WiFi.disconnect();

      WiFi.begin(
        WIFI_SSID,
        WIFI_PASSWORD
      );
    }

    delay(20);

    return;
  }

  // ==========================================================
  // PREVIOUS BUTTON - GPIO35
  // ==========================================================

  int previousState =
    digitalRead(
      BUTTON_PREVIOUS
    );

  if (
    previousState == LOW &&
    millis() - lastPreviousPress >
      BUTTON_DELAY
  )
  {
    lastPreviousPress =
      millis();

    Serial.println(
      "PREVIOUS BUTTON"
    );

    previousTrack();

    delay(500);

    getSpotifyData();
  }

  // ==========================================================
  // NEXT BUTTON - GPIO0
  // ==========================================================

  int nextState =
    digitalRead(
      BUTTON_NEXT
    );

  if (
    nextState == LOW &&
    millis() - lastNextPress >
      BUTTON_DELAY
  )
  {
    lastNextPress =
      millis();

    Serial.println(
      "NEXT BUTTON"
    );

    nextTrack();

    delay(500);

    getSpotifyData();
  }

  // ==========================================================
  // SPOTIFY UPDATE
  // ==========================================================

  if (
    millis() - lastSpotifyUpdate >
      SPOTIFY_UPDATE_INTERVAL
  )
  {
    lastSpotifyUpdate =
      millis();

    getSpotifyData();
  }

  delay(10);
}


// ============================================================
// PREVIOUS TRACK
// ============================================================

void previousTrack()
{
  HTTPClient http;

  String url =
    "http://" +
    String(SERVER) +
    ":" +
    String(SERVER_PORT) +
    "/previous";

  Serial.print(
    "Previous URL: "
  );

  Serial.println(
    url
  );

  http.begin(url);

  http.setTimeout(5000);

  int httpCode =
    http.POST("");

  Serial.print(
    "Previous HTTP: "
  );

  Serial.println(
    httpCode
  );

  http.end();
}


// ============================================================
// NEXT TRACK
// ============================================================

void nextTrack()
{
  HTTPClient http;

  String url =
    "http://" +
    String(SERVER) +
    ":" +
    String(SERVER_PORT) +
    "/next";

  Serial.print(
    "Next URL: "
  );

  Serial.println(
    url
  );

  http.begin(url);

  http.setTimeout(5000);

  int httpCode =
    http.POST("");

  Serial.print(
    "Next HTTP: "
  );

  Serial.println(
    httpCode
  );

  http.end();
}


// ============================================================
// GET SPOTIFY DATA
// ============================================================

void getSpotifyData()
{
  if (
    WiFi.status() != WL_CONNECTED
  )
  {
    return;
  }

  HTTPClient http;

  String url =
    "http://" +
    String(SERVER) +
    ":" +
    String(SERVER_PORT) +
    "/now-playing";

  http.begin(url);

  http.setTimeout(5000);

  int httpCode =
    http.GET();

  if (
    httpCode != 200
  )
  {
    Serial.print(
      "Spotify HTTP error: "
    );

    Serial.println(
      httpCode
    );

    http.end();

    return;
  }

  String payload =
    http.getString();

  http.end();

  // ----------------------------------------------------------
  // JSON
  // ----------------------------------------------------------

  DynamicJsonDocument doc(
    4096
  );

  DeserializationError error =
    deserializeJson(
      doc,
      payload
    );

  if (error)
  {
    Serial.print(
      "JSON error: "
    );

    Serial.println(
      error.c_str()
    );

    return;
  }

  // ----------------------------------------------------------
  // Read Spotify information
  // ----------------------------------------------------------

  String newSong =
    doc["song"] | "";

  String newArtist =
    doc["artist"] | "";

  String newAlbumArt =
    doc["album_art"] | "";

  int newProgress =
    doc["progress"] | 0;

  int newDuration =
    doc["duration"] | 0;

  // ----------------------------------------------------------
  // Detect new artwork
  // ----------------------------------------------------------

  bool artworkChanged =
    newAlbumArt.length() > 0 &&
    newAlbumArt != lastAlbumArtURL;

  bool trackChanged =
    newSong != lastSong;

  // ----------------------------------------------------------
  // Save Spotify data
  // ----------------------------------------------------------

  currentSong =
    newSong;

  currentArtist =
    newArtist;

  currentAlbumArtURL =
    newAlbumArt;

  currentProgress =
    newProgress;

  currentDuration =
    newDuration;

  // ----------------------------------------------------------
  // New artwork
  // ----------------------------------------------------------

  if (artworkChanged)
  {
    Serial.println(
      "NEW ALBUM ART"
    );

    albumArtLoaded =
      false;

    lastAlbumArtURL =
      newAlbumArt;

    drawAlbumPlaceholder();

    if (
      loadAlbumArt()
    )
    {
      albumArtLoaded =
        true;
    }
  }

  // ----------------------------------------------------------
  // Redraw text/progress
  // ----------------------------------------------------------

  if (
    trackChanged ||
    artworkChanged
  )
  {
    drawSpotifyUI();
  }
  else
  {
    // Update progress without touching album art
    drawProgressBar();
  }

  lastSong =
    newSong;
}


// ============================================================
// ALBUM PLACEHOLDER
// ============================================================

void drawAlbumPlaceholder()
{
  tft.fillRect(
    0,
    0,
    SCREEN_W,
    135,
    TFT_BLACK
  );
}


// ============================================================
// LOAD ALBUM ART
// ============================================================

bool loadAlbumArt()
{
  if (
    WiFi.status() != WL_CONNECTED
  )
  {
    return false;
  }

  Serial.println(
    "Downloading album art..."
  );

  HTTPClient http;

  String url =
    "http://" +
    String(SERVER) +
    ":" +
    String(SERVER_PORT) +
    "/album-art";

  http.begin(url);

  http.setTimeout(10000);

  int httpCode =
    http.GET();

  Serial.print(
    "Album art HTTP: "
  );

  Serial.println(
    httpCode
  );

  if (
    httpCode != 200
  )
  {
    http.end();

    return false;
  }

  int contentLength =
    http.getSize();

  Serial.print(
    "Album art size: "
  );

  Serial.println(
    contentLength
  );

  if (
    contentLength <= 0
  )
  {
    http.end();

    return false;
  }

  // ----------------------------------------------------------
  // Allocate JPEG buffer
  // ----------------------------------------------------------

  uint8_t* buffer =
    (uint8_t*)malloc(
      contentLength
    );

  if (
    buffer == nullptr
  )
  {
    Serial.println(
      "Not enough RAM!"
    );

    http.end();

    return false;
  }

  // ----------------------------------------------------------
  // Download
  // ----------------------------------------------------------

  WiFiClient* stream =
    http.getStreamPtr();

  size_t totalRead =
    0;

  unsigned long startTime =
    millis();

  while (
    totalRead <
      (size_t)contentLength &&
    millis() - startTime <
      15000
  )
  {
    size_t available =
      stream->available();

    if (
      available > 0
    )
    {
      size_t remaining =
        contentLength -
        totalRead;

      size_t toRead =
        min(
          available,
          remaining
        );

      size_t readBytes =
        stream->readBytes(
          buffer + totalRead,
          toRead
        );

      totalRead +=
        readBytes;
    }

    delay(1);
  }

  http.end();

  Serial.print(
    "Downloaded: "
  );

  Serial.println(
    totalRead
  );

  // ----------------------------------------------------------
  // Verify
  // ----------------------------------------------------------

  if (
    totalRead !=
      (size_t)contentLength
  )
  {
    Serial.println(
      "Incomplete download!"
    );

    free(buffer);

    return false;
  }

  // ----------------------------------------------------------
  // Decode
  // ----------------------------------------------------------

  Serial.println(
    "Decoding album art..."
  );

  // Vertical display.
  //
  // For now the album image starts at 0,0.
  // We'll tune the exact alignment later.
  //
  // Scale 4 is retained.
  // ----------------------------------------------------------

  bool result =
    TJpgDec.drawJpg(
      ART_X,
      ART_Y,
      buffer,
      totalRead
    );

  // IMPORTANT:
  // Free ONLY after decoding.
  free(buffer);

  buffer = nullptr;

  if (result)
  {
    Serial.println(
      "Album art displayed"
    );
  }
  else
  {
    Serial.println(
      "Album art decode FAILED"
    );
  }

  return result;
}


// ============================================================
// DRAW UI
// ============================================================

void drawSpotifyUI()
{
  // ----------------------------------------------------------
  // Clear everything BELOW album artwork
  // ----------------------------------------------------------

  tft.fillRect(
    0,
    135,
    SCREEN_W,
    SCREEN_H - 135,
    TFT_BLACK
  );

  // ----------------------------------------------------------
  // Song
  // ----------------------------------------------------------

  drawSongTitle();

  // ----------------------------------------------------------
  // Artist
  // ----------------------------------------------------------

  drawArtist();

  // ----------------------------------------------------------
  // Progress
  // ----------------------------------------------------------

  drawProgressBar();
}


// ============================================================
// SONG TITLE
// ============================================================

void drawSongTitle()
{
  tft.setTextColor(
    TFT_WHITE
  );

  tft.setTextSize(1);

  tft.setTextWrap(false);

  String title =
    currentSong;

  if (
    title.length() == 0
  )
  {
    title =
      "Nothing playing";
  }

  // ----------------------------------------------------------
  // Clear title region
  // ----------------------------------------------------------

  tft.fillRect(
    0,
    140,
    SCREEN_W,
    35,
    TFT_BLACK
  );

  // ----------------------------------------------------------
  // Try one line
  // ----------------------------------------------------------

  if (
    tft.textWidth(title) <
      SCREEN_W - 10
  )
  {
    tft.setCursor(
      5,
      143
    );

    tft.print(
      title
    );

    return;
  }

  // ----------------------------------------------------------
  // Split into two lines
  // ----------------------------------------------------------

  int bestSplit =
    -1;

  for (
    int i = 0;
    i < title.length();
    i++
  )
  {
    if (
      title[i] == ' '
    )
    {
      String first =
        title.substring(
          0,
          i
        );

      if (
        tft.textWidth(first) <
          SCREEN_W - 10
      )
      {
        bestSplit =
          i;
      }
    }
  }

  if (
    bestSplit > 0
  )
  {
    String line1 =
      title.substring(
        0,
        bestSplit
      );

    String line2 =
      title.substring(
        bestSplit + 1
      );

    tft.setCursor(
      5,
      140
    );

    tft.print(
      line1
    );

    // --------------------------------------------------------
    // Second line
    // --------------------------------------------------------

    tft.setCursor(
      5,
      153
    );

    // Fit second line
    if (
      tft.textWidth(line2) <
        SCREEN_W - 10
    )
    {
      tft.print(
        line2
      );
    }
    else
    {
      String shortened =
        "";

      for (
        int i = 0;
        i < line2.length();
        i++
      )
      {
        String test =
          shortened +
          line2[i] +
          "...";

        if (
          tft.textWidth(test) <
            SCREEN_W - 10
        )
        {
          shortened +=
            line2[i];
        }
        else
        {
          break;
        }
      }

      tft.print(
        shortened
      );

      tft.print(
        "..."
      );
    }

    return;
  }

  // ----------------------------------------------------------
  // No space
  // ----------------------------------------------------------

  String shortened =
    "";

  for (
    int i = 0;
    i < title.length();
    i++
  )
  {
    String test =
      shortened +
      title[i] +
      "...";

    if (
      tft.textWidth(test) <
        SCREEN_W - 10
    )
    {
      shortened +=
        title[i];
    }
    else
    {
      break;
    }
  }

  tft.setCursor(
    5,
    145
  );

  tft.print(
    shortened
  );

  tft.print(
    "..."
  );
}


// ============================================================
// ARTIST
// ============================================================

void drawArtist()
{
  tft.setTextColor(
    TFT_LIGHTGREY
  );

  tft.setTextSize(1);

  tft.setTextWrap(false);

  String artist =
    currentArtist;

  if (
    artist.length() == 0
  )
  {
    artist =
      "Unknown artist";
  }

  // ----------------------------------------------------------
  // Clear artist area
  // ----------------------------------------------------------

  tft.fillRect(
    0,
    175,
    SCREEN_W,
    18,
    TFT_BLACK
  );

  // ----------------------------------------------------------
  // Trim long artist
  // ----------------------------------------------------------

  if (
    tft.textWidth(artist) >
      SCREEN_W - 10
  )
  {
    String shortened =
      "";

    for (
      int i = 0;
      i < artist.length();
      i++
    )
    {
      String test =
        shortened +
        artist[i] +
        "...";

      if (
        tft.textWidth(test) <
          SCREEN_W - 10
      )
      {
        shortened +=
          artist[i];
      }
      else
      {
        break;
      }
    }

    artist =
      shortened +
      "...";
  }

  tft.setCursor(
    5,
    177
  );

  tft.print(
    artist
  );
}


// ============================================================
// PROGRESS BAR
// ============================================================

void drawProgressBar()
{
  const int x = 5;
  const int y = 205;
  const int width = 125;

  // ----------------------------------------------------------
  // Clear progress region
  // ----------------------------------------------------------

  tft.fillRect(
    0,
    195,
    SCREEN_W,
    35,
    TFT_BLACK
  );

  // ----------------------------------------------------------
  // Convert milliseconds to seconds
  // ----------------------------------------------------------

  int progressSeconds =
    currentProgress / 1000;

  int durationSeconds =
    currentDuration / 1000;

  // ----------------------------------------------------------
  // Progress time
  // ----------------------------------------------------------

  int progressMin =
    progressSeconds / 60;

  int progressSec =
    progressSeconds % 60;

  int durationMin =
    durationSeconds / 60;

  int durationSec =
    durationSeconds % 60;

  char progressText[10];

  snprintf(
    progressText,
    sizeof(progressText),
    "%d:%02d",
    progressMin,
    progressSec
  );

  char durationText[10];

  snprintf(
    durationText,
    sizeof(durationText),
    "%d:%02d",
    durationMin,
    durationSec
  );

  // ----------------------------------------------------------
  // Time
  // ----------------------------------------------------------

  tft.setTextSize(1);

  tft.setTextColor(
    TFT_LIGHTGREY
  );

  tft.setCursor(
    5,
    195
  );

  tft.print(
    progressText
  );

  int durationWidth =
    tft.textWidth(
      durationText
    );

  tft.setCursor(
    SCREEN_W -
      durationWidth -
      5,
    195
  );

  tft.print(
    durationText
  );

  // ----------------------------------------------------------
  // Bar
  // ----------------------------------------------------------

  tft.fillRect(
    x,
    y,
    width,
    3,
    TFT_DARKGREY
  );

  // ----------------------------------------------------------
  // Progress
  // ----------------------------------------------------------

  if (
    currentDuration > 0
  )
  {
    float percentage =
      (float)currentProgress /
      (float)currentDuration;

    percentage =
      constrain(
        percentage,
        0.0,
        1.0
      );

    int progressWidth =
      width *
      percentage;

    if (
      progressWidth > 0
    )
    {
      tft.fillRect(
        x,
        y,
        progressWidth,
        3,
        TFT_WHITE
      );
    }

    // Progress dot

    int dotX =
      x +
      progressWidth;

    dotX =
      constrain(
        dotX,
        x,
        x + width
      );

    tft.fillCircle(
      dotX,
      y + 1,
      3,
      TFT_WHITE
    );
  }
}
