# ESPHome Package: Matrix Lamp Preset Manager
Project Description
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
###########################################       RU          ###############################################################################
## Описание проекта

Этот проект представляет собой пакет (package) для ESPHome, предназначенный для управления пресетами и плейлистами в LED-лампе на базе Neopixel (матрица светодиодов). Пакет позволяет сохранять, загружать и автоматически запускать пользовательские настройки эффектов, яркости, интенсивности, скорости и масштаба для различных сценариев освещения. Он интегрируется в умный дом (например, Home Assistant) и поддерживает автоматизацию по времени, положению солнца и режимам "день/ночь".

Проект разработан для пользователей ESPHome, желающих расширить функциональность своей LED-матрицы (например, на базе ESP32 или аналогичных устройств) без необходимости писать весь код с нуля. Пакет использует глобальные переменные для хранения настроек, что обеспечивает их сохранение при перезагрузке устройства.

## Функции проекта

Этот паккадж расширяет возможности проекта LED matrix projects (e.g., [andrewjswan/matrix-lamp](https://github.com/andrewjswan/matrix-lamp)) 

### 1. **Управление пресетами**
   - **5 индивидуальных пресетов (0-4)**: Каждый пресет хранит настройки эффекта (effect_name), яркости (brightness), интенсивности (intensity), скорости (speed) и масштаба (scale).
   - **Сохранение пресета**: Кнопка "Save Preset" сохраняет текущие параметры светодиода в выбранный пресет.
   - **Загрузка пресета**: Выбор пресета в селекторе автоматически применяет настройки к LED-матрице.

### 2. **Плейлист (Playlist)**
   - **Сочетание пресетов**: Плейлист позволяет объединять несколько пресетов (0-4) с индивидуальными длительностями (в формате MM:SS).
   - **Автоматическое воспроизведение**: Скрипт `play_preset_playlist` последовательно запускает эффекты из плейлиста с задержками.
   - **Включение/исключение пресетов**: Режимы "in playlist" и "out playlist" управляют, какие пресеты входят в плейлист.

### 3. **Режимы старта и автоматизация**
   - **Режимы старта для каждого пресета**:
     - "None": Ручной запуск.
     - "start on time": Запуск по заданному времени (HH:MM).
     - "sunrise": Запуск по восходу солнца с коррекцией угла elevation.
     - "sunset": Запуск по закату солнца с коррекцией угла elevation.
     - "day": Запуск весь день (elevation > 0°).
     - "night": Запуск всю ночь (elevation < 0°).
     - "in playlist": Добавление в плейлист с длительностью.
     - "out playlist": Удаление из плейлиста.
   - **Коррекции для солнца**: Number-элемент для настройки коррекции elevation (в градусах) для sunrise/sunset.
   - **Автоматический запуск**: Интервалы (30 сек и 300 сек) проверяют условия и автоматически запускают пресеты/плейлист.

### 4. **Интерфейс и сенсоры**
   - **Select для выбора пресета**: Опции: "0"-"4", "Playlist", "None".
   - **Select для режима старта**: Список режимов для выбранного пресета.
   - **Datetime picker**: Для установки времени старта или длительности (MM:SS для плейлиста).
   - **Number для коррекции запуска на восходе и закате**: "Sun Elevation Correction" (-90° до +90°).
   - **Text sensors**:
     - "Preset Info": Отображает настройки текущего пресета (эффект, параметры, плейлист).
     - "Preset Start Settings": Показывает режим, время и коррекции.
     - Текущее время (SNTP и Home Assistant).
   - **Сенсоры солнца**: Elevation и azimuth для автоматизации.
   - **Button для сохранения изменений**

### 5. **Дополнительные возможности**
   - **Синхронизация времени**: Через SNTP и Home Assistant.
   - **Глобальные переменные**: Все настройки сохраняются в globals с `restore_value: yes` для persistence.
   - **Логирование**: ESP_LOG для отладки (например, сохранение пресетов).
   - **Интеграция с Home Assistant**: Все элементы (select, number, text_sensor) доступны в UI.

### 6. **Технические детали**
   - **Зависимости**: Требует базовой конфигурации ESPHome с проектом matrix-lamp:  Neopixel LED (`id: neopixel_led`), числами (`matrix_intensity`, `matrix_speed`, `matrix_scale`) 

Этот пакет идеален для создания динамического освещения: например, утренний sunrise-эффект, вечерний sunset, ночной плейлист или дневной режим по расписанию.
