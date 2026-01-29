# Waveshare 4.2inch e-Paper (B) V2 Custom Component

This ESPHome external component provides proper support for the **Waveshare 4.2inch e-Paper Module (B) V2** three-color display (black/white/red).

## Why This Component?

The built-in ESPHome `waveshare_epaper` component's `4.20in-bV2-bwr` model only works with the **OLD** hardware revision. Waveshare has released a **NEW** hardware revision that uses a different command set and busy pin logic, which the built-in component doesn't support.

This component **automatically detects** which hardware revision you have and uses the correct commands.

## Hardware Versions

| Version | Commands | Busy Pin Logic |
|---------|----------|----------------|
| OLD | 0x10/0x13 for black/red data | LOW when busy |
| NEW | 0x24/0x26 for black/red data | HIGH when busy |

## Installation

Add this to your ESPHome YAML:

```yaml
external_components:
  - source:
      type: local
      path: components
    components: [waveshare_4in2b_v2]
```

Or from a GitHub repository:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/astoker/esphome-waveshare-4in2b-v2
    components: [waveshare_4in2b_v2]
```

## Configuration

```yaml
spi:
  clk_pin: GPIO13
  mosi_pin: GPIO14

display:
  - platform: waveshare_4in2b_v2
    cs_pin: GPIO15
    dc_pin: GPIO27
    reset_pin: GPIO26
    busy_pin: GPIO25
    update_interval: 300s
    lambda: |-
      // Black text
      it.print(10, 10, id(my_font), Color(0,0,0), "Hello World!");
      
      // Red text
      it.print(10, 50, id(my_font), Color(255,0,0), "Red Text!");
      
      // Filled rectangles
      it.filled_rectangle(10, 100, 50, 50, Color(0,0,0));    // Black
      it.filled_rectangle(70, 100, 50, 50, Color(255,0,0));  // Red
```

## Configuration Variables

- **cs_pin** (Required): SPI chip select pin
- **dc_pin** (Required): Data/Command control pin
- **reset_pin** (Required): Hardware reset pin
- **busy_pin** (Required): Busy status pin
- **update_interval** (Optional, default: 300s): How often to refresh the display
- **full_update_every** (Optional, default: 1): Number of partial updates before a full refresh

## Colors

For the 3-color display:
- **Black**: `Color(0, 0, 0)` or any dark color (R,G,B all < 127)
- **White**: `Color(255, 255, 255)` or any bright color
- **Red**: `Color(255, 0, 0)` or any color with high red and low green/blue

## Notes

- Display refresh takes approximately 15-20 seconds (normal for 3-color e-paper)
- The component auto-detects hardware version on startup (check logs for "Detected OLD/NEW hardware version")
- Tested with Waveshare ESP32 Driver Board

## Tested Hardware

- Waveshare 4.2inch e-Paper (B) V2 (400×300, BWR)
- Waveshare ESP32 Driver Board

## License

MIT License
