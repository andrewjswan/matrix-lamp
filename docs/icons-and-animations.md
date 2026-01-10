## Icons and Animations

Download and install all needed icons `(.jpg/.png)` and animations `(.gif)` under the `matrix_lamp:` key. All icons have to be 8x8 pixels in size. If necessary, you can scale them with the option `resize: 8x8`.

You can also specify a URL to directly download the image file. The URLs will only be downloaded once at compile time, so there is no additional traffic on the hosting website.

All other solutions provide ready-made icons, especially **Lametric** has a big database of [icons](https://developer.lametric.com/icons). If you find an icon, you can use it with its ID. e.g., `lameid: 1234`.

!!! note
    Please, check the copyright of the icons you use.

!!! note
    The maximum number of icons is limited to **100** in the code and also by the flash space and the RAM of your board.

!!! example annotate "Icons and Animations"

    ``` { .yaml .copy .annotate }
    matrix_lamp:
      display: ...
      icons:
        - id: boot
          file: icons/rocket.GIF
          duration: 75    
        - id: temp
          lameid: 1234
        - id: yoga
          file: icons/yoga-bridge.GIF
          pingpong: true
        - id: jackshome
          url: https://awtrix.blueforcer.de/animations/JackHomePage
          resize: 32x8
        - id: garage
          lameid: 1234
          duration: 100
        - id: homeassistant
          url: https://github.com/home-assistant/assets/raw/master/logo/logo-special.png      
    ```

## Parameters reference

The ID of the icons is used later to icon display. So, you should name them wisely.

!!! tip
    If you like to group icons, you should prefix them, e.g., with `weather_`.

There are some more advanced parameters:

- **frame_duration** (optional, `ms`): in the case of a GIF-file, the component tries to read the default interval for each frame. The default/fallback interval is 192 ms. In case you need to override the default value, set the duration per icon.

- **pingpong** (optional, `boolean`): in the case of a GIF-file, you can reverse the frames instead of starting from the first frame.

- **file** (Exclusive, `filename`): a local filename

- **url** (Exclusive, `url`): a URL to download the icon

- **lameid** (Exclusive, `number`): the ID from the **LaMetric** [icon](https://developer.lametric.com/icons) database

- **resize** (Optional, `8x8` or `16x16`): If the source file hasn't the right dimensions.

!!! example annotate "Icons resize"

    ``` { .yaml .copy .annotate }
        - id: jackshome
          url: https://awtrix.blueforcer.de/animations/JackHomePage
          resize: 16x16
    ```
