# Waveshare 4.2" B V2 E-Paper Display Component for ESPHome

This repository provides an ESPHome display component for the Waveshare 4.2" B V2 e-paper module. It is designed to resolve compatibility and rendering issues addressed in [esphome/esphome#7995](https://github.com/esphome/esphome/pull/7995), ensuring correct operation with the V2 hardware revision.

## Features

- Full support for the Waveshare 4.2" B V2 e-paper display
- Corrects display initialization and update sequences for V2 hardware
- Integrates seamlessly with ESPHome YAML configuration
- Supports standard ESPHome display features (text, images, graphics primitives)

## Background

The official ESPHome Waveshare e-paper component did not fully support the V2 revision of the 4.2" B display, resulting in incorrect rendering or failed updates. This component implements the necessary changes to initialization, LUTs, and update logic, as discussed and resolved in [PR #7995](https://github.com/esphome/esphome/pull/7995).

## Installation

1. Copy the component files from this repository into your ESPHome project's `components/` directory.
2. Reference the custom component in your ESPHome YAML:

```yaml
external_components:
  - source: github://AStoker/esphome-waveshare-4in2b-v2

display:
  - platform: waveshare_4in2b_v2
    cs_pin: D8
    dc_pin: D2
    busy_pin: D6
    reset_pin: D4
    # Add other configuration as needed
```

## Usage

After installation, you can use all standard ESPHome display features. Refer to the ESPHome [display documentation](https://esphome.io/components/display/index.html) for usage examples.

## License

MIT License

## Credits

- Based on ESPHome's original Waveshare e-paper component
- Fixes and improvements inspired by [esphome/esphome#7995](https://github.com/esphome/esphome/pull/7995)
- Maintained by [AStoker](https://github.com/AStoker)