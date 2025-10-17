## A group of synchronized Matrix Lamps

To use `Matrix Lamps` in a group, you need to include the [matrix_lamp_espnow.yaml](https://github.com/andrewjswan/matrix-lamp/blob/main/packages/matrix_lamp_espnow.yaml) package in each lamp.
The `Master` and `Slave` switches will become available in the interface. 
Turn on the `Master` switch for one lamp and the `Slave` switch for all others, and it should work.

## Sound reactive on Matrix Lamp

If you need to use a Matrix Lamp with sound-reactive effects without a built-in microphone, you'll need a remote microphone. The [Music Leds](https://andrewjswan.github.io/esphome-components/music-leds/) component with the [neopixel_light_music_leds_espnow_master.yaml](https://github.com/andrewjswan/esphome-config/blob/main/packages/neopixel_light_music_leds_espnow_master.yaml) package enabled can serve as a remote microphone, while the [matrix_lamp_espnow_music_leds_slave.yaml](https://github.com/andrewjswan/matrix-lamp/blob/main/packages/matrix_lamp_espnow_music_leds_slave.yaml) package must be enabled in the Matrix Lamp.
On the [Music Leds](https://andrewjswan.github.io/esphome-components/music-leds/) side, turn on the `Master` switch, and on the Matrix Lamp side, turn on the `Slave` switch and select any music effect.

!!! note
    The Matrix Lamp use the [ESPNow](https://esphome.io/components/espnow/) protocol for synchronization.
