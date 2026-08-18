# Waveshare 4.2inch e-Paper (B) V2 — ESPHome component

ESPHome external component for the **Waveshare 4.2inch e-Paper Module (B) V2**, a 400×300
three-colour (black / white / red) panel.

## Why this component?

Waveshare shipped two different controllers under the same "4.2inch e-Paper (B) V2" name. The
built-in ESPHome `waveshare_epaper` models `4.20in-bv2` and `4.20in-bv2-bwr` both implement only
the **old** controller's command set. This component supports both revisions.

| Revision | Plane data      | Refresh     | BUSY while busy |
|----------|-----------------|-------------|-----------------|
| old      | `0x10` / `0x13` | `0x12`      | low             |
| new      | `0x24` / `0x26` | `0x22`+`0x20` | high          |

## Installation

```yaml
external_components:
  - source: github://AStoker/esphome-waveshare-4in2b-v2
    components: [waveshare_4in2b_v2]
```

## Configuration

Pins below match the **Waveshare e-Paper ESP32 Driver Board** as wired on the board itself
(all revisions, including the Type-C Rev 3 / 20241230 board).

```yaml
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
      it.fill(Color(255, 255, 255));                                 // start from a blank page
      it.print(10, 10, id(my_font), Color(0, 0, 0), "Hello World!");  // black
      it.print(10, 50, id(my_font), Color(255, 0, 0), "Red Text!");   // red
      it.filled_rectangle(10, 100, 50, 50, Color(0, 0, 0));           // black
      it.filled_rectangle(70, 100, 50, 50, Color(255, 0, 0));         // red
```

### Options

- **cs_pin** (*Required*): SPI chip select.
- **dc_pin** (*Required*): Data/command select.
- **reset_pin** (*Required*): Hardware reset.
- **busy_pin** (*Required*): Busy status input.
- **model** (*Optional*, default `auto`): `auto`, `old`, or `new`. See below.
- **update_interval** (*Optional*, default `5min`): How often to refresh.
- All the standard [display component](https://esphome.io/components/display/) options.

## Colours

Colours are classified by RGB, so you write the colour you want:

- **Black** — `Color(0, 0, 0)`, or any colour with R, G and B all below 128.
- **Red** — `Color(255, 0, 0)`, or any colour with R above 127 and G, B below 128.
- **White** — `Color(255, 255, 255)`, or anything else.

### Clearing the screen

`auto_clear_enabled` defaults to **false** here, unlike most ESPHome displays.

ESPHome's clear colour is `COLOR_OFF`, which is `Color(0, 0, 0)` — indistinguishable from the
black you draw with, and therefore black *ink* on this panel. Leaving auto-clear on would repaint
the whole screen black before every frame. Start your lambda with an explicit
`it.fill(Color(255, 255, 255));` instead.

### Drawing with default colours

ESPHome's colourless drawing helpers use `COLOR_ON`, which is white — and white is invisible on a
white background. On a three-colour panel, always pass the colour you want:

```yaml
      it.print(10, 10, id(my_font), Color(0, 0, 0), "text");     # not it.print(10, 10, id(my_font), "text")
      it.line(0, 0, 400, 0, Color(0, 0, 0));                     # not it.line(0, 0, 400, 0)
      it.image(0, 0, id(my_image), Color(0, 0, 0), Color(255, 255, 255));  # ink, then background
```

## Choosing the revision

The official Waveshare library identifies the revision by writing `0x2F` and reading a byte back
over the DIN line. The ESP32 driver board wires no read-back path, so `model: auto` falls back to
sampling BUSY after a reset: the old controller idles high, the new one idles low.

That heuristic works on the boards it has been tried on, but it is a heuristic. If the log line
`Auto-detected the … panel revision` names the wrong one — or the display stays blank and you see
`Panel stayed busy for 30000ms` — pin it down explicitly:

```yaml
    model: new   # or: old
```

## Power and timing behaviour

Each refresh runs a complete reset → init → data → refresh → deep-sleep cycle, so the panel is
never left in its high-voltage driving state between updates. This matches Waveshare's own example
code and is what the panel is rated for.

A three-colour refresh takes 15–20 seconds of panel time. That wait happens in `loop()`, not
inside `update()`, so the device stays responsive to Home Assistant, OTA and automations while the
panel settles. `update()` itself blocks for roughly half a second (the reset sequence is mostly
fixed delays required by the controller).

This matters more than it sounds: ESPHome warns when a component blocks the main loop, and its
warning threshold saturates at 2550 ms with no way to raise it. A driver that waited inline for
the refresh would log `display took a long time for an operation (~16000 ms)` on *every* update,
forever.

## Hardware notes — e-Paper ESP32 Driver Board

- The ESP32↔panel wiring is fixed on the board: DIN→GPIO14, SCLK→GPIO13, CS→GPIO15, DC→GPIO27,
  RST→GPIO26, BUSY→GPIO25.
- **Set DIP switch 1 to position "A" (3R)** — that is the setting Waveshare lists for the 4.2inch
  e-Paper (B). The wrong position produces a poor or missing image that looks exactly like a
  driver fault.
- Set DIP switch 2 to ON to power the USB-UART bridge; flashing fails with it off.
- Rev 3 (the 20241230 revision) only swapped micro-USB for Type-C and changed the USB-serial chip
  to CH343. The panel wiring is unchanged, so no configuration differences.

## Tested hardware

- Waveshare 4.2inch e-Paper (B) V2 (400×300, BWR)
- Waveshare e-Paper ESP32 Driver Board

## License

MIT
