# Wiring Guide — Seeed XIAO ESP32-C3 + PCA9685 (this fork)

This fork replaces the microcontroller-direct PWM of the upstream builds
(Lolin S2 Mini / Distro Boards) with two parts Vlad already had:

- **Seeed XIAO ESP32-C3** — runs the firmware, Wi-Fi and the OLED face.
- **PCA9685 breakout** (16-channel, 12-bit I2C PWM driver) — generates
  every servo pulse in hardware. The C3 only sends "channel N, angle A"
  over I2C, so timing stays clean and the C3's few pins are enough.

Everything else about the build (printed parts, MG90S servos, poses) is
unchanged from the [main build guide](../../build-guide/README.md).

## 1. XIAO ESP32-C3 → PCA9685

Four wires. This is the *logic* side of the PCA9685 only — it carries
signals and a trickle of power for the chip, never servo current.

| XIAO ESP32-C3 pin | PCA9685 pin | Notes |
|---|---|---|
| 3.3V | VCC | Logic power for the PCA9685 chip. Use 3.3 V so the I2C levels match the C3. |
| GND | GND | Must also be common with the servo supply ground (see §3). |
| D4 (GPIO6) | SDA | I2C data |
| D5 (GPIO7) | SCL | I2C clock |
| — (leave off) | OE | Most breakouts pull OE down internally, so the outputs are enabled by default. Optionally wire OE to D3 (GPIO10) if you ever want a firmware kill-switch for all servos at once. |
| — (leave off) | A0–A5 | Address jumpers: leave all unsoldered/open. The firmware expects the default address **0x40**. |

Keep the I2C wires short (10–15 cm is fine) and twist SDA/SCL loosely
with a ground wire if you get flaky behaviour.

**This build uses the PCA9685 board as the wiring hub** (decided
2026-11-14): every ground in the system terminates on the PCA9685 board
itself, so no two wires ever need to be bridged or spliced together.
The C3 connects to the header at one end of the board, the OLED plugs
into the matching header at the *other* end (most breakouts duplicate
GND/VCC/SCL/SDA there for chaining), and the servo supply ground lands
on the V+ screw terminal. Each wire keeps its own pin.

## 2. The OLED plugs into the far end of the PCA9685

The SSD1306 OLED face display shares the I2C bus, but it does **not**
wire back to the C3. Plug it into the duplicated header at the opposite
end of the PCA9685 board from the C3:

| OLED pin | PCA9685 far-end pin |
|---|---|
| SDA | SDA |
| SCL | SCL |
| VCC | VCC |
| GND | GND |

> [!IMPORTANT]
> Check the silkscreen labels at the far end before plugging in — the
> pin order can be mirrored compared to the C3 end. Match labels, not
> positions (VCC and GND swapped would kill the display).
>
> If your breakout has no far-end header, the fallbacks, in order:
> the GND pin of any spare servo slot (8–15) for the OLED ground plus
> solderless joins for SDA/SCL/VCC, or a mini breadboard as the hub.

No address clash: the OLED is at I2C address **0x3C**, the PCA9685 at
**0x40**.

## 3. Servo power — the important part

> [!CAUTION]
> **Never power the servos from the XIAO.** Eight MG90S servos can pull
> several amps in bursts; the C3's regulator would brown out or cook.
> Servo power goes into the PCA9685's blue **V+ screw terminal** from a
> separate supply, exactly like the rest of this build:
>
> **Battery (2S, 7.4 V) → buck converter / BEC set to 5.0–5.3 V
> (≥3 A continuous, 5 A preferred) → PCA9685 V+ terminal.**

- The V+ terminal feeds the red wire of *every* servo plug on the board.
- Fit a **470–1000 µF electrolytic capacitor** across V+ and GND at the
  terminal (many breakouts include one) to soak up servo start spikes.
- **Grounds must all meet**: BEC output ground, PCA9685 GND and C3 GND
  are one common ground. In this build they meet *on the PCA9685 board*
  (see §1–2), which is the single star point — the BEC's ground wire
  terminates on the V+ terminal's GND screw, so nothing needs joining
  anywhere else. Without a common ground, the PWM signals have no
  reference and the legs will twitch or ignore you.
- Powering the C3 itself: on the bench, its USB-C cable is fine. For
  battery running, feed the C3's **5V** pin from the same BEC output
  (after the buck converter, never raw battery).

## 4. Servo plugs — channel map

Plug each servo's 3-pin connector into the PCA9685 channel below, with
the **brown/black (ground) wire on the GND row** — on most breakouts
that's the row nearest the board edge, and the row is labelled on the
silkscreen. A reversed plug is the fastest way to kill a servo.

| PCA9685 channel | Joint | | PCA9685 channel | Joint |
|---|---|---|---|---|
| 0 | **L1** | | 4 | **R1** |
| 1 | **L3** | | 5 | **R3** |
| 2 | **L2** | | 6 | **R2** |
| 3 | **L4** | | 7 | **R4** |

Per side the order is 1, 3, 2, 4 (this is deliberate — it matches how
the leads fall inside the frame). Channels 8–15 are spare.

The firmware's movement code still thinks in the upstream logical motor
order (R1, R2, L1, L2, R4, R3, L3, L4); a single table in
`sesame-firmware-main.ino` (`servoChannels`) translates logical motor →
PCA9685 channel, and the web UI's Manual Motor Control sliders are
labelled with both the joint and its PCA channel.

## 5. Bench order (first power-up)

1. Wire §1 and §2 only (C3 ↔ PCA9685 ↔ OLED). Leave all servo plugs out
   and V+ disconnected.
2. Flash `firmware/debugging-firmware/sesame-motor-tester.ino`, open the
   Serial Monitor at 115200 — it should print the channel map.
3. With a multimeter on the BEC output (servos still unplugged), set it
   to **5.0–5.3 V, correct polarity**, then wire it to the V+ terminal.
4. Plug in **one** servo, on channel 0 (L1), centre it with `0,90` (or
   `L1,90`). Repeat channel by channel — this is the build guide's
   Phase 5 calibration, just addressed by PCA channel/joint instead of
   board header.
5. Centre all eight (90°) *before* fitting horns or joints, exactly as
   the build guide says, then flash the main firmware.

## 6. Troubleshooting

- **Nothing moves, web UI works** — check the PCA9685 has 3.3 V on VCC
  and that SDA/SCL aren't swapped. An I2C scanner sketch should find
  0x40 (PCA9685) and 0x3C (OLED).
- **Robot resets when it walks** — servo supply sagging. Shorter/thicker
  V+ leads, the capacitor fitted, `motorCurrentDelay` up in the web
  settings, or a stronger BEC — in that order.
- **A leg moves but it's the wrong leg** — the servo is in the wrong
  channel. Check against the table in §4 before touching the code; per
  the build guide, this is the cause 99% of the time.
