<script type="module" src="https://unpkg.com/esp-web-tools@10/dist/web/install-button.js?module"></script>
<script src="https://ajax.googleapis.com/ajax/libs/jquery/3.7.1/jquery.min.js"></script>
<script src="../js/installer.js"></script>

<style>
  .md-typeset h1 {
    display: none;
  }
</style>

[![Made for ESPHome](img/made-for-esphome.svg){ align=right loading=lazy style="width:30%" }](https://esphome.io/guides/made_for_esphome.html)

!!! info "ESP Web Tools"
    User friendly tools to manage `ESP8266` and `ESP32` devices in the browser:

    * Install &amp; update firmware.
    * Connect device to the `Wi-Fi` network.
    * Visit the device's hosted `web interface`.
    * Access logs and send terminal commands.
    * Add devices to [Home Assistant](https://www.home-assistant.io).

## Install MatrixLamp <b><span id="version"></span></b>

<div class="esp-installer-page">
<div class="radios">
  <label>
    <input
      type="radio"
      name="matrix-lamp"
      class="device"
      id=""
      value=""
      checked
    />
    <img src="../img/matrix-lamp.png" alt="Matrix Lamp" />
    <span></span>
  </label>
</div>

<div class="button-row">
  <esp-web-install-button manifest="../manifest.json"></esp-web-install-button>
</div>

</div>

---

[MatrixLamp](https://github.com/andrewjswan/matrix-lamp) — Installer powered by [ESP Web Tools](https://esphome.github.io/esp-web-tools/).
