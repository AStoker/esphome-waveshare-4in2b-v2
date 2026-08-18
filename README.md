# Waveshare 4.2" (B) V2 e-Paper component for ESPHome

An ESPHome display component for the Waveshare 4.2inch e-Paper Module (B) V2 — a 400×300
black/white/red panel — supporting **both** hardware revisions of that panel.

Waveshare shipped two different controllers under the same product name. The built-in ESPHome
`waveshare_epaper` models (`4.20in-bv2`, `4.20in-bv2-bwr`) implement only the older one's command
set, which is the compatibility gap discussed in
[esphome/esphome#7995](https://github.com/esphome/esphome/pull/7995).

## Features

- Both panel revisions: `0x10`/`0x13` (old) and `0x24`/`0x26` (new) command sets
- Revision selected explicitly via `model:`, or auto-detected
- Full three-colour rendering — write `Color(0, 0, 0)` and `Color(255, 0, 0)` directly
- Panel is put to sleep between refreshes rather than left powered
- Standard ESPHome display features: text, images, graphics primitives

## Quick start

Pin numbers below are the fixed wiring of the Waveshare e-Paper ESP32 Driver Board.

```yaml
external_components:
  - source: github://AStoker/esphome-waveshare-4in2b-v2
    components: [waveshare_4in2b_v2]

spi:
  clk_pin: GPIO13   # SCLK
  mosi_pin: GPIO14  # DIN

display:
  - platform: waveshare_4in2b_v2
    cs_pin: GPIO15
    dc_pin: GPIO27
    reset_pin: GPIO26
    busy_pin: GPIO25
    update_interval: 300s
    lambda: |-
      it.fill(Color(255, 255, 255));
      it.print(10, 10, id(my_font), Color(0, 0, 0), "Hello World!");
```

Full options, colour handling, revision selection and driver-board notes are in
[components/waveshare_4in2b_v2/README.md](components/waveshare_4in2b_v2/README.md).

## License

MIT

## Credits

- Based on ESPHome's `waveshare_epaper` component and the official
  [Waveshare Arduino library](https://github.com/waveshareteam/e-Paper/tree/master/Arduino/epd4in2b_V2)
- Motivated by [esphome/esphome#7995](https://github.com/esphome/esphome/pull/7995)
- Maintained by [AStoker](https://github.com/AStoker)
