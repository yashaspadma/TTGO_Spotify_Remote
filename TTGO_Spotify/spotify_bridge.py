import json
import io

import urllib.request
import urllib.error
import urllib.parse

from PIL import Image

from http.server import BaseHTTPRequestHandler, HTTPServer


# ============================================================
# CONFIGURATION
# ============================================================

TOKEN_FILE = "spotify_token.json"

CLIENT_ID = "a45e539eecaf4f30a77dc0e064b51ef5"

API_BASE = "https://api.spotify.com/v1"

TOKEN_URL = "https://accounts.spotify.com/api/token"


# ============================================================
# ALBUM ART
# ============================================================

def get_album_art_bytes():

    try:

        data = get_now_playing()

        album_art = data.get(
            "album_art",
            ""
        )

        if not album_art:

            print("No album art URL found")

            return None

        print(
            "Downloading album art from Spotify..."
        )

        request = urllib.request.Request(
            album_art,
            headers={
                "User-Agent": "Mozilla/5.0"
            }
        )

        with urllib.request.urlopen(
            request,
            timeout=10
        ) as response:

            original_data = response.read()

        print(
            "Original album art:",
            len(original_data),
            "bytes"
        )

        # ====================================================
        # RESIZE IMAGE FOR ESP32
        # ====================================================

        image = Image.open(
            io.BytesIO(original_data)
        )

        print(
            "Original resolution:",
            image.size
        )

        # Convert to RGB because some Spotify images
        # may contain modes that JPEG doesn't support.

        if image.mode != "RGB":

            image = image.convert(
                "RGB"
            )

        # ====================================================
        # RESIZE
        # ====================================================

        image = image.resize(
            (120, 120),
            Image.Resampling.LANCZOS
        )

        # ====================================================
        # COMPRESS AS JPEG
        # ====================================================

        output = io.BytesIO()

        image.save(
            output,
            format="JPEG",
            quality=85,
            optimize=True
        )

        resized_data = output.getvalue()

        print(
            "Resized album art:",
            len(resized_data),
            "bytes"
        )

        return resized_data

    except Exception as e:

        print(
            "Album art error:",
            e
        )

        return None


# ============================================================
# TOKEN MANAGEMENT
# ============================================================

def load_token_data():

    with open(
        TOKEN_FILE,
        "r"
    ) as f:

        return json.load(f)


def save_token_data(data):

    with open(
        TOKEN_FILE,
        "w"
    ) as f:

        json.dump(
            data,
            f,
            indent=4
        )


def refresh_access_token():

    print(
        "Refreshing Spotify access token..."
    )


    token_data = load_token_data()


    refresh_token = token_data.get(
        "refresh_token"
    )


    if not refresh_token:

        print(
            "ERROR: No refresh token found!"
        )

        return None


    post_data = urllib.parse.urlencode({

        "grant_type":
            "refresh_token",

        "refresh_token":
            refresh_token,

        "client_id":
            CLIENT_ID

    }).encode()


    request = urllib.request.Request(

        TOKEN_URL,

        data=post_data,

        headers={
            "Content-Type":
                "application/x-www-form-urlencoded"
        },

        method="POST"
    )


    try:

        with urllib.request.urlopen(
            request
        ) as response:

            new_data = json.loads(
                response.read().decode()
            )


            # Spotify may not return
            # a new refresh token.
            if (
                "refresh_token"
                not in new_data
            ):

                new_data[
                    "refresh_token"
                ] = refresh_token


            save_token_data(
                new_data
            )


            print(
                "Access token refreshed successfully!"
            )


            return new_data[
                "access_token"
            ]


    except urllib.error.HTTPError as e:

        print(
            "TOKEN REFRESH ERROR:",
            e.code
        )

        print(
            e.read().decode()
        )

        return None


def get_access_token():

    token_data = load_token_data()

    return token_data.get(
        "access_token"
    )


# ============================================================
# SPOTIFY REQUEST
# ============================================================

def spotify_request(
    endpoint,
    method="GET"
):

    access_token = get_access_token()


    if not access_token:

        return None, 401


    request = urllib.request.Request(

        API_BASE + endpoint,

        headers={
            "Authorization":
                f"Bearer {access_token}"
        },

        method=method
    )


    try:

        with urllib.request.urlopen(
            request
        ) as response:

            if response.status == 204:

                return None, 204


            try:

                data = json.loads(
                    response.read().decode()
                )


                return (
                    data,
                    response.status
                )


            except:

                return (
                    None,
                    response.status
                )


    except urllib.error.HTTPError as e:

        # ----------------------------------------------------
        # TOKEN EXPIRED
        # ----------------------------------------------------

        if e.code == 401:

            print(
                "Access token expired."
            )


            new_token = refresh_access_token()


            if not new_token:

                return None, 401


            request = urllib.request.Request(

                API_BASE + endpoint,

                headers={
                    "Authorization":
                        f"Bearer {new_token}"
                },

                method=method
            )


            try:

                with urllib.request.urlopen(
                    request
                ) as response:

                    if response.status == 204:

                        return None, 204


                    try:

                        data = json.loads(
                            response.read().decode()
                        )


                        return (
                            data,
                            response.status
                        )


                    except:

                        return (
                            None,
                            response.status
                        )


            except urllib.error.HTTPError as retry_error:

                print(
                    "Spotify retry error:",
                    retry_error.code
                )

                return (
                    None,
                    retry_error.code
                )


        print(
            "Spotify API error:",
            e.code
        )


        return (
            None,
            e.code
        )


# ============================================================
# CURRENTLY PLAYING
# ============================================================

def get_now_playing():

    data, status = spotify_request(
        "/me/player",
        "GET"
    )


    if not data:

        return {
            "playing": False
        }


    item = data.get(
        "item"
    )


    if not item:

        return {
            "playing": False
        }


    # --------------------------------------------------------
    # ARTIST
    # --------------------------------------------------------

    artists = item.get(
        "artists",
        []
    )


    artist_name = ", ".join(

        artist.get(
            "name",
            ""
        )

        for artist in artists
    )


    # --------------------------------------------------------
    # ALBUM
    # --------------------------------------------------------

    album = item.get(
        "album",
        {}
    )


    # --------------------------------------------------------
    # ALBUM ART
    # --------------------------------------------------------

    album_images = album.get(
        "images",
        []
    )


    album_art = ""


    if album_images:

        album_art = album_images[0].get(
            "url",
            ""
        )


    # --------------------------------------------------------
    # RETURN DATA
    # --------------------------------------------------------

    return {

        "playing":
            data.get(
                "is_playing",
                False
            ),

        "song":
            item.get(
                "name",
                "Unknown"
            ),

        "artist":
            artist_name,

        "album":
            album.get(
                "name",
                "Unknown"
            ),

        "album_art":
            album_art,

        "progress":
            data.get(
                "progress_ms",
                0
            ),

        "duration":
            item.get(
                "duration_ms",
                0
            )
    }


# ============================================================
# PREVIOUS
# ============================================================

def previous_track():

    print(
        "Previous button requested"
    )


    data, status = spotify_request(

        "/me/player/previous",

        "POST"
    )


    return status


# ============================================================
# NEXT
# ============================================================

def next_track():

    print(
        "Next button requested"
    )


    data, status = spotify_request(

        "/me/player/next",

        "POST"
    )


    return status


# ============================================================
# HTTP SERVER
# ============================================================

class SpotifyHandler(
    BaseHTTPRequestHandler
):


    # --------------------------------------------------------
    # JSON RESPONSE
    # --------------------------------------------------------

    def send_json(
        self,
        data
    ):

        response = json.dumps(
            data
        ).encode()


        self.send_response(
            200
        )


        self.send_header(
            "Content-Type",
            "application/json"
        )


        self.send_header(
            "Access-Control-Allow-Origin",
            "*"
        )


        self.send_header(
            "Content-Length",
            str(len(response))
        )


        self.end_headers()


        self.wfile.write(
            response
        )


    # --------------------------------------------------------
    # GET
    # --------------------------------------------------------

    def do_GET(self):


        # ====================================================
        # NOW PLAYING
        # ====================================================

        if self.path == "/now-playing":

            print(
                "Now playing requested"
            )


            data = get_now_playing()


            self.send_json(
                data
            )


        # ====================================================
        # PREVIOUS
        # ====================================================

        elif self.path == "/previous":

            status = previous_track()


            self.send_json({

                "success":
                    status in [200, 204],

                "status":
                    status
            })


        # ====================================================
        # NEXT
        # ====================================================

        elif self.path == "/next":

            status = next_track()


            self.send_json({

                "success":
                    status in [200, 204],

                "status":
                    status
            })


        # ====================================================
        # ALBUM ART
        # ====================================================

        elif self.path == "/album-art":

            print()
            print(
                "Album art requested"
            )


            image_data = (
                get_album_art_bytes()
            )


            if image_data:

                print(
                    "Sending album art:",
                    len(image_data),
                    "bytes"
                )


                self.send_response(
                    200
                )


                self.send_header(
                    "Content-Type",
                    "image/jpeg"
                )


                self.send_header(
                    "Content-Length",
                    str(len(image_data))
                )


                self.send_header(
                    "Cache-Control",
                    "no-cache"
                )


                self.send_header(
                    "Access-Control-Allow-Origin",
                    "*"
                )


                self.end_headers()


                self.wfile.write(
                    image_data
                )


            else:

                print(
                    "Could not retrieve album art"
                )


                self.send_response(
                    404
                )


                self.end_headers()


        # ====================================================
        # UNKNOWN URL
        # ====================================================

        else:

            print(
                "404:",
                self.path
            )


            self.send_response(
                404
            )


            self.end_headers()

        # ========================================================
    # POST
    # ========================================================

    def do_POST(self):

        # ====================================================
        # PREVIOUS
        # ====================================================

        if self.path == "/previous":

            print(
                "POST /previous"
            )

            status = previous_track()

            self.send_json({

                "success":
                    status in [200, 204],

                "status":
                    status

            })

            return

        # ====================================================
        # NEXT
        # ====================================================

        if self.path == "/next":

            print(
                "POST /next"
            )

            status = next_track()

            self.send_json({

                "success":
                    status in [200, 204],

                "status":
                    status

            })

            return

        # ====================================================
        # UNKNOWN POST
        # ====================================================

        print(
            "POST 404:",
            self.path
        )

        self.send_response(
            404
        )

        self.end_headers()

    # --------------------------------------------------------
    # DISABLE HTTP LOGGING
    # --------------------------------------------------------

    def log_message(
        self,
        format,
        *args
    ):

        pass


# ============================================================
# MAIN
# ============================================================

def main():

    HOST = "0.0.0.0"

    PORT = 8765


    server = HTTPServer(

        (
            HOST,
            PORT
        ),

        SpotifyHandler
    )


    print()

    print(
        "=" * 50
    )

    print(
        "       TTGO SPOTIFY BRIDGE"
    )

    print(
        "=" * 50
    )


    print()

    print(
        "Server running on:"
    )

    print(
        "http://localhost:8765"
    )


    print()

    print(
        "TTGO endpoint:"
    )

    print(
        "http://192.168.1.11:8765/now-playing"
    )


    print()

    print(
        "Controls:"
    )


    print(
        "Previous:"
    )

    print(
        "http://192.168.1.11:8765/previous"
    )


    print(
        "Next:"
    )

    print(
        "http://192.168.1.11:8765/next"
    )


    print()

    print(
        "Album art:"
    )

    print(
        "http://192.168.1.11:8765/album-art"
    )


    print()

    print(
        "Automatic token refresh: ENABLED"
    )


    print()

    print(
        "Press CTRL+C to stop."
    )


    print(
        "=" * 50
    )


    server.serve_forever()


# ============================================================
# START
# ============================================================

if __name__ == "__main__":

    main()