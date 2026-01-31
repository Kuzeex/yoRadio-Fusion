This build adds **DLNA playlist browsing and playback** support to yoRadio,
tested primarily with **Synology NAS**.

## 1. Build Configuration (`myoptions.h`)

Enable DLNA support and configure your DLNA server:
#define USE_DLNA
#define dlnaHost "192.168.180.122"   // DLNA server IP address
#define dlnaIDX  21                 // Root MUSIC container ID (Synology default is usually 21)

2. DLNA Browser Overview

The DLNA browser works in two levels:

📁 Category List (Containers)

Displays DLNA folders / categories (e.g. Artist, Album, Genre)

Selecting a category loads its contents

Containers without playable tracks are handled gracefully

🎵 Item List (Tracks)

Displays playable audio items inside the selected container

Clicking an item starts playback immediately

Navigation is non-blocking and safe to use during playback.

3. Building a DLNA Playlist

Inside the DLNA browser:

Build DLNA Playlist

Builds a new playlist from the selected container

The playlist is stored internally and replaces the previous DLNA playlist

After build, the playlist index is reset to track 1

Append DLNA Playlist

Appends items from the selected container to the existing DLNA playlist

Current playback position is preserved

Build operations run asynchronously and do not freeze the UI.

4. Playlist Source Selection

Two explicit buttons control the active playlist source:

▶️ Use DLNA Playlist

Activates the DLNA playlist as the current source

Playback resumes automatically only if audio was playing before

DLNA playlist index is preserved between activations

▶️ Use WEB Playlist

Switches back to the regular WEB playlist

WEB playlist index and state are preserved independently from DLNA

👉 DLNA and WEB playlists are fully independent:
switching sources does not reset the other playlist.

5. Playback Behavior (Important Rules)

DLNA playlist build does not start playback automatically

User interaction (clicking a track or activating a playlist) always has priority

Track selection in DLNA works exactly like in WEB mode

No automatic index jumping or forced resets occur during normal use

6. Current Status

✅ Stable playback switching (WEB ↔ DLNA)

✅ Independent playlist indices

✅ Safe asynchronous build / append

⚠️ Considered experimental – feedback and testing welcome