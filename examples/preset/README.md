# ESPHome Package: Matrix Lamp Preset Manager
## Project Description
This package extends LED matrix projects (e.g., [andrewjswan/matrix-lamp](https://github.com/andrewjswan/matrix-lamp)) with preset and playlist management. The package allows saving, loading, and automatically running custom settings for effects, brightness, intensity, speed, and scale for various lighting scenarios. It integrates with smart home systems (e.g., Home Assistant) and supports automation based on time, sun position, and day/night modes.

The project is developed for ESPHome users who want to expand the functionality of their LED matrix (e.g., based on ESP32 or similar devices) without having to write all the code from scratch. The package uses global variables to store settings, ensuring they are preserved during device reboots.

Project Features
Based on the provided code, the package includes the following capabilities:

### 1. Preset Management
5 individual presets (0-4): Each preset stores settings for effect (effect_name), brightness, intensity, speed, and scale.
Preset saving: The "Save Preset" button saves the current LED parameters to the selected preset.
Preset loading: Selecting a preset in the selector automatically applies the settings to the LED matrix.
### 2. Playlist
Combining presets: The playlist allows combining several presets (0-4) with individual durations (in MM:SS format).
Automatic playback: The play_preset_playlist script sequentially runs effects from the playlist with delays.
Including/excluding presets: "In playlist" and "out playlist" modes control which presets are included in the playlist.
### 3. Start Modes and Automation
Start modes for each preset:
"None": Manual start.
"start on time": Start at a specified time (HH:MM).
"sunrise": Start at sunrise with elevation angle correction.
"sunset": Start at sunset with elevation angle correction.
"day": Start all day (elevation > 0°).
"night": Start all night (elevation < 0°).
"in playlist": Add to playlist with duration.
"out playlist": Remove from playlist.
Sun corrections: Number element for setting elevation correction (in degrees) for sunrise/sunset.
Automatic start: Intervals (30 sec and 300 sec) check conditions and automatically start presets/playlists.
### 4. Interface and Sensors
Select for preset selection: Options: "0"-"4", "Playlist", "None".
Select for start mode: List of modes for the selected preset.
Datetime picker: For setting start time or duration (MM:SS for playlist).
Number for sunrise and sunset correction: "Sun Elevation Correction" (-90° to +90°).
Text sensors:
"Preset Info": Displays current preset settings (effect, parameters, playlist).
"Preset Start Settings": Shows mode, time, and corrections.
Current time (via SNTP and Home Assistant).
Sun sensors: Elevation and azimuth for automation.
Button for saving changes
### 5. Additional Features
Time synchronization: Via SNTP and Home Assistant.
Global variables: All settings are stored in globals with restore_value: yes for persistence.
Logging: ESP_LOG for debugging (e.g., preset saving).
Home Assistant integration: All elements (select, number, text_sensor) are available in the UI.
### 6. Technical Details
Dependencies: Requires basic ESPHome configuration with Project matrix-lamp: Neopixel LED (id: neopixel_led), numbers (matrix_intensity, matrix_speed, matrix_scale)

This package is ideal for creating dynamic lighting: for example, morning sunrise effects, evening sunset, nighttime playlists, or daytime scheduled modes.
