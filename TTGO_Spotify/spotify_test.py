import base64
import hashlib
import json
import os
import secrets
import threading
import time
import urllib.parse
import urllib.request
import webbrowser
from http.server import BaseHTTPRequestHandler, HTTPServer

# ============================================================
# YOUR SPOTIFY APP
# ============================================================

CLIENT_ID = "a45e539eecaf4f30a77dc0e064b51ef5"

REDIRECT_URI = "http://127.0.0.1:8888/callback"

# Permissions we need for the final TTGO project
SCOPES = (
    "user-read-currently-playing "
    "user-read-playback-state "
    "user-modify-playback-state"
)

AUTH_URL = "https://accounts.spotify.com/authorize"
TOKEN_URL = "https://accounts.spotify.com/api/token"
PLAYER_URL = "https://api.spotify.com/v1/me/player"

# ============================================================
# PKCE
# ============================================================

def generate_code_verifier():
    return secrets.token_urlsafe(64)


def generate_code_challenge(verifier):
    digest = hashlib.sha256(verifier.encode("ascii")).digest()
    return base64.urlsafe_b64encode(digest).decode("ascii").rstrip("=")


# ============================================================
# CALLBACK SERVER
# ============================================================

authorization_code = None
callback_error = None


class CallbackHandler(BaseHTTPRequestHandler):

    def do_GET(self):
        global authorization_code, callback_error

        parsed = urllib.parse.urlparse(self.path)

        if parsed.path != "/callback":
            self.send_response(404)
            self.end_headers()
            return

        params = urllib.parse.parse_qs(parsed.query)

        if "error" in params:
            callback_error = params["error"][0]
        elif "code" in params:
            authorization_code = params["code"][0]

        self.send_response(200)
        self.send_header("Content-Type", "text/html")
        self.end_headers()

        if authorization_code:
            message = """
            <html>
            <body>
                <h1>Spotify authentication successful!</h1>
                <p>You can close this browser tab.</p>
            </body>
            </html>
            """
        else:
            message = """
            <html>
            <body>
                <h1>Spotify authentication failed.</h1>
                <p>You can close this browser tab.</p>
            </body>
            </html>
            """

        self.wfile.write(message.encode("utf-8"))

    def log_message(self, format, *args):
        # Hide normal HTTP server messages
        pass


# ============================================================
# TOKEN EXCHANGE
# ============================================================

def exchange_code(code, verifier):

    data = urllib.parse.urlencode({
        "client_id": CLIENT_ID,
        "grant_type": "authorization_code",
        "code": code,
        "redirect_uri": REDIRECT_URI,
        "code_verifier": verifier
    }).encode()

    request = urllib.request.Request(
        TOKEN_URL,
        data=data,
        headers={
            "Content-Type": "application/x-www-form-urlencoded"
        },
        method="POST"
    )

    try:
        with urllib.request.urlopen(request) as response:
            return json.loads(response.read().decode())

    except urllib.error.HTTPError as e:
        error_body = e.read().decode()
        print("\nSpotify token error:")
        print(error_body)
        return None


# ============================================================
# GET CURRENTLY PLAYING
# ============================================================

def get_current_song(access_token):

    request = urllib.request.Request(
        PLAYER_URL,
        headers={
            "Authorization": f"Bearer {access_token}"
        }
    )

    try:
        with urllib.request.urlopen(request) as response:

            # Spotify can return 204 when nothing is playing
            if response.status == 204:
                return None

            return json.loads(response.read().decode())

    except urllib.error.HTTPError as e:

        if e.code == 204:
            return None

        print("\nSpotify API error:")
        print(e.read().decode())
        return None


# ============================================================
# MAIN
# ============================================================

def main():

    print("=" * 50)
    print("       TTGO SPOTIFY TEST")
    print("=" * 50)

    verifier = generate_code_verifier()
    challenge = generate_code_challenge(verifier)

    state = secrets.token_urlsafe(16)

    params = {
        "response_type": "code",
        "client_id": CLIENT_ID,
        "scope": SCOPES,
        "redirect_uri": REDIRECT_URI,
        "state": state,
        "code_challenge_method": "S256",
        "code_challenge": challenge
    }

    authorization_url = (
        AUTH_URL + "?" +
        urllib.parse.urlencode(params)
    )

    # Start callback server
    server = HTTPServer(
        ("127.0.0.1", 8888),
        CallbackHandler
    )

    print("\nStarting local authentication server...")
    print("Opening Spotify in your browser...\n")

    threading.Thread(
        target=server.serve_forever,
        daemon=True
    ).start()

    # Open Spotify login
    webbrowser.open(authorization_url)

    print("1. Log into Spotify if necessary.")
    print("2. Approve the application.")
    print("3. Wait for the browser to say authentication successful.\n")

    # Wait for callback
    timeout = 120
    start = time.time()

    while authorization_code is None and callback_error is None:

        if time.time() - start > timeout:
            print("Authentication timed out.")
            server.shutdown()
            return

        time.sleep(0.2)

    server.shutdown()

    if callback_error:
        print("Spotify returned an error:")
        print(callback_error)
        return

    print("Authorization code received!")

    # Exchange code for tokens
    token_data = exchange_code(
        authorization_code,
        verifier
    )

    if not token_data:
        return

    access_token = token_data.get("access_token")
    refresh_token = token_data.get("refresh_token")

    print("\nAuthentication successful!")
    print("Access token received:", bool(access_token))
    print("Refresh token received:", bool(refresh_token))

    # Save token locally
    with open("spotify_token.json", "w") as f:
        json.dump(token_data, f, indent=4)

    print("\nToken saved locally as:")
    print("spotify_token.json")

    # ========================================================
    # TEST CURRENT SONG
    # ========================================================

    print("\nChecking what Spotify is currently playing...\n")

    playback = get_current_song(access_token)

    if playback is None:
        print("Nothing is currently playing.")
        return

    item = playback.get("item")

    if not item:
        print("Spotify returned no track information.")
        return

    song_name = item.get("name", "Unknown")

    artists = item.get("artists", [])
    artist_name = ", ".join(
        artist.get("name", "Unknown")
        for artist in artists
    )

    album = item.get("album", {})
    album_name = album.get("name", "Unknown")

    progress_ms = playback.get("progress_ms", 0)
    duration_ms = item.get("duration_ms", 0)

    progress_seconds = progress_ms // 1000
    duration_seconds = duration_ms // 1000

    print("=" * 50)
    print("              NOW PLAYING")
    print("=" * 50)

    print("Song   :", song_name)
    print("Artist :", artist_name)
    print("Album  :", album_name)

    print(
        "Time   : "
        f"{progress_seconds // 60:02d}:"
        f"{progress_seconds % 60:02d}"
        " / "
        f"{duration_seconds // 60:02d}:"
        f"{duration_seconds % 60:02d}"
    )

    print(
        "Playing:",
        playback.get("is_playing", False)
    )

    print("=" * 50)


if __name__ == "__main__":
    main()