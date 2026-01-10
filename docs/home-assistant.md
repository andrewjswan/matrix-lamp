## Weather

### Weather effect

!!! example annotate "Weather effect"

    ``` { .yaml .copy .annotate }
    id: matrix_lamp_weather
    alias: Matrix Lamp - Weather
    trigger:
      - platform: state
        entity_id: weather.current_weather
    action:
      - service: light.turn_on
        target:
          entity_id: light.matrix_lamp_neopixel
          data:
            effect: >-
              {% set state = states('weather.current_weather') %}
              {% set weather = {
                "clear-night": "Fire 2021",
                "cloudy": "Clouds",
                "exceptional": "Spectrum",
                "fog": "Smoke",
                "hail": "Popcorn",
                "lightning": "Thunderstorm in a Jar",
                "lightning-rainy": "Thunderstorm in a Jar",
                "partlycloudy": "Clouds",
                "pouring": "Precipitation",
                "rainy": "Drops on Glass",
                "snowy": "Snowfall",
                "snowy-rainy": "Starfall / Snowstorm",
                "sunny": "DNA",
                "windy": "Flock",
                "windy-variant": "Flock N Predator"} %}
              {{ weather[state] }}
    ```

### Weather icon

!!! example annotate "Weather icon"

    ``` { .yaml .copy .annotate }
    id: matrix_lamp_weather_icon
    alias: Matrix Lamp - Weather Icon
    trigger:
      - platform: state
        entity_id: weather.current_weather
    action:
      - action: esphome.matrix_lamp_show_icon (1)
        data:
          icon_name: "weather_{{ states('weather.current_weather') | replace('-','_') }}"
    ```
1. To use home assistant services and icons, they must be enabled and added to the Matrix Lamp configuration.

!!! note
    To use Home Assistant [services](matrix-lamp-component.md#custom-services) and icons, they must be enabled and added to the Matrix Lamp configuration.
