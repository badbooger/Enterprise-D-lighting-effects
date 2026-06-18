# USS Enterprise-D Prop Controller — Hardware Reference

## PCB Overview

The model splits into two physical sections, each with its own ESP32 and PCB stack.

| Section | Controller | PCB Stack | Ribbon Cables |
|---------|-----------|-----------|---------------|
| Saucer (Bridge) | ESP32-S3 | 3 custom PCBs | 8-pin + 15-pin |
| Stardrive (EngRoom) | Xiao ESP32-C3 | 4 custom PCBs | 8-pin x2 |

Power distribution uses JST Micro 1.25 connectors throughout.

---

## Bridge ESP32-S3 — GPIO Pinout

| Group | GPIO Pin | Description |
|-------|----------|-------------|
| G01 | 6 | Window group 1 |
| G02 | 7 | Window group 2 |
| G03 | 35 | Window group 3 |
| G04 | 5 | Window group 4 |
| G05 | 15 | Window group 5 |
| G06 | 21 | Window group 6 |
| G07 | 16 | Window group 7 |
| G08 | 14 | Window group 8 |
| G09 | 13 | Window group 9 |
| G10 | 12 | Window group 10 |
| G11 | 11 | Window group 11 |
| G12 | 10 | Window group 12 |
| G13 | 8 | Window group 13 |
| G14 | 3 | Window group 14 |
| G15 | 9 | Window group 15 |
| NAV | 38 | Navigation lights |
| IM LEFT | 36 | Impulse engine — port |
| IM RIGHT | 37 | Impulse engine — starboard |
| TX (Sound) | 17 | DY-SV17F UART TX |
| RX (Sound) | 18 | DY-SV17F UART RX |

### Startup / Shutdown Group Order

```
G15 → G14 → G02 → G01+G02 (simultaneous)
→ G09
→ G08+G10 → G07+G11 → G06+G12 → G06+G13
```

### Known Issue — Pin Label Assignments Are Mirrored and Flipped

The reference drawing used during Bridge PCB design was sourced from a website and had its pin labels mirrored and flipped. The label assignments were taken from that drawing, so the group labels on the board do not match the original kit documentation.

**The physical board is correct** — mounting holes, connector positions, and board layout all fit the model as intended. No physical rework needed.

**Current state — functional as-is:** The LED connector labels on the board are correct relative to each other, so plugging each LED connector to its matching labelled pin works without any modification.

**Board revision items identified during neck assembly — both confirmed 2026-05-19:**

1. **Mounting holes** — need to shift to the opposite side of the board. During neck assembly the board had to be moved to clear the power switch, and the current hole positions no longer line up. Shift holes to the other side in the next PCB revision.
2. **Pin label assignments** — pin order issue encountered during neck assembly as expected. Resolved by swapping LEDs/connectors into physically correct positions rather than following kit documentation pin order. Labels need correcting in a board revision if kit documentation compatibility is wanted. Not required for current functionality.

---

## DY-SV17F Sound Player — Mode Selection

The DY-SV17F supports multiple operating modes (UART, one-wire, IO trigger, etc.), selected by pulling certain pins high or low at boot. PCB 3 has a resistor array of three groups of pull-up and pull-down footprints to allow the mode to be changed by populating or depopulating resistors, rather than cutting traces or rewiring. This was designed in as a fallback in case UART control didn't work — any other mode could be configured by swapping resistors instead of spinning a new board.

**Current mode: UART** (TX=GPIO17, RX=GPIO18, 9600 baud). UART works reliably.

**PCB revision 2026-05-21:** Non-UART mode resistors removed from PCB 3. Only the resistors required to configure UART mode are retained. Simplifies the board with no functional change.

### No-Sound Build Variant

If the DY-SV17F is not wanted — for a simpler build or to keep costs down — the sound player and its resistor array can be omitted entirely. Without the DY player there is no 5V requirement, which means the 2S (7.4V) battery is no longer necessary. A no-sound build could run on either:

- **Original kit power** — 3 AAA batteries, exactly as the kit shipped. Simple, no modifications needed.
- **1S LiPo + USB charging circuit** — single cell (~3.7V nominal) with a small charging module (e.g. TP4056). Adds recharging capability with less risk than the current 2S setup, which has no BMS. A reasonable upgrade path for a no-sound build.

Note: the 2S battery was chosen specifically to give a simple linear regulator path to 5V for the DY player. Remove the player and the reason for 2S goes away.

### External Boost Option (out of scope)

An alternative to the full 2S setup: add solder pads on the PCB to power the DY-SV17F from a small external boost converter (3.7V→5V) running off a 1S LiPo or the original 3 AAA cells. This would allow a simpler battery with sound still present. Adds a separate module and extra design work — outside the scope of the current project, noted here for future consideration only.

---

## LED Resistor Values and Voltage History

All LED current-limiting resistors are 0603 package. Values were chosen empirically to match the original kit brightness at each supply voltage.

| Version | Supply | LED current-limiting resistor | Calculated LED current (Vf ≈ 3.2V) |
|---------|--------|-------------------------------|-------------------------------------|
| Original kit | 4.5V (3× AAA) | 820Ω | ~1.6mA |
| Current build (2S LiPo) | 7.4V nominal / 8.4V full | 1.2kΩ | ~3.5mA / ~4.3mA |
| 3S upgrade (board revision) | 11.1V nominal / 12.6V full | **2.2kΩ** | ~3.6mA / ~4.3mA |

The 2.2kΩ value for 3S was calculated to match the current 2S/1.2kΩ operating current almost exactly — same brightness, no PWM compensation needed.

**Note:** At these current levels (under 8mA in any configuration) the LEDs are lightly loaded relative to their ~20–30mA ratings. Long-term LED damage is not a concern at any of these supply voltages.

> ⚠️ **IMPORTANT — Two populations of 1.2kΩ resistors on the revised boards:**
> During the 2026-05-21 PCB revision, all transistor base resistors were changed from 10kΩ to **1.2kΩ** to reduce the BOM to a single resistor value per board. This means the boards now have two distinct groups of 1.2kΩ resistors:
> - **LED current-limiting resistors** — in series with each LED group. **These change when changing supply voltage.**
> - **Transistor base resistors** — between GPIO and transistor base. **These do NOT change with supply voltage — 1.2kΩ is correct for 3.3V logic at any supply.**
>
> When doing any voltage conversion, only replace the LED current-limiting resistors. The base resistors stay at 1.2kΩ regardless. Confusing these two populations and changing the base resistors would affect transistor drive and cause LEDs to be dim or not switch at all.

### 3S LiPo Upgrade Notes (for board revision)

Switching from 2S (7.4V) to 3S (11.1V nominal, 12.6V full) with existing 1.2kΩ resistors is safe for the LEDs — worst case is 7.8mA at full charge. PWM compensation works fine if doing this on current boards: drop baseline "full on" brightness from 255 to ~135 (~53% duty cycle) to match current brightness. No hardware change needed short-term.

For a board revision:
- Change **LED current-limiting resistors** to **2.2kΩ** — eliminates the brightness difference entirely (base resistors stay at 1.2kΩ)
- Replace linear voltage regulators with **buck converters** (e.g. MP2307, SOT-23-8 footprint) — linear regs run significantly hotter at 3S input (12.6V → 3.3V is a 9.3V drop vs 5.1V on 2S)
- Battery compartment gains ~50% more energy: 3× real 10440 cells (11.1V × 0.6Ah = 6.66Wh) vs current 2× cells + dummy (7.4V × 0.6Ah = 4.44Wh)
- 3S full charge = 12.6V — confirm voltage regulator input ratings before ordering

---

## EngRoom Xiao ESP32-C3 — LED Groups

9 LED pins, 6 groups (plus GPIO 2 for battery ADC):

| Group | Pins | Description |
|-------|------|-------------|
| Windows | 3 pins (individual) | 3 addressable window groups: neck all (GPIO 20 — back of neck + both battle bridges wired together), eng top (GPIO 7), eng bot (GPIO 21) |
| Nacelles | 2 pins (1 group) | Port (GPIO 8) + starboard (GPIO 9) warp nacelles |
| Deflector | 1 pin | Deflector dish (GPIO 6) |
| Photon torpedoes | 1 pin | Forward torpedo launcher — GPIO 4, own 1.2kΩ resistor on neck PCB (see revision note below) |
| Impulse engines | 1 pin | Stardrive impulse (GPIO 10) |
| Nav lights | 1 pin | Navigation lights — GPIO 5, white LED, 4 transistors total: EngRoom PCB, neck PCB, port nacelle, starboard nacelle. Own 1.2kΩ resistor on neck PCB (see revision note below) |

> **Neck PCB revision in progress (2026-05-21):** Addresses nav/photon shared resistor and nav transistor issues.
> - Nav and photon LEDs previously shared a single 1.2kΩ resistor — caused photon backfeed to lower board nav LED (fix #3) and nav dimming (fix #5). Fixed in revision: each LED now has its own 1.2kΩ resistor on the neck PCB.
> - Nav lights redesigned: neck board previously had no transistor — the neck nav LED was connected directly to the EngRoom PCB transistor collector. Neck PCB now has its own transistor for the neck nav LED, driven by the GPIO 5 control signal via the FFC. GPIO 5 now drives 4 transistor bases in parallel (EngRoom, neck, port nacelle, starboard nacelle) — ~8.8mA total, within ESP32-C3 limits.
> - All pull-down resistors removed from neck PCB (consistent with battle bridge PCB decision — ESP32 OUTPUT mode holds pins LOW, external pull-downs redundant).
> - All base resistors changed from 10kΩ to 1.2kΩ — same value as LED current-limiting resistors, reduces BOM to one resistor value on the board.
> - Connector footprint reversed so nav/photon wiring is correct by design — no re-pinning needed after this revision.
> - Board layout and mounting hole locations updated to match original model dimensions.
>
> **EngRoom PCB also revised (2026-05-21):** Multiple changes — current boards unchanged, firmware update deferred until new boards fitted.
> - FFC connector updated — nav pin now carries GPIO 5 control signal to neck PCB nav transistor base (was post-transistor collector output).
> - Voltage regulator moved away from centre standoff; input/output decoupling caps moved with it.
> - Battery voltage monitor added on GPIO 2 (ADC1). Voltage divider: R1=270kΩ, R2=100kΩ — maps 6.0V–8.4V battery range to 1.62V–2.27V on ADC. Use ADC_11db attenuation in firmware. **Hand-assembly bench substitute:** 470kΩ (R1) + 200kΩ (R2) — maps same range to 1.79V–2.51V, comfortably within 11dB range and ratio close to final design. Write firmware for final 270kΩ+100kΩ values (multiplier Vbat = Vadc × 3.70); bench parts read ~10% low, swap before calibrating. Divider placed on PCB close to GPIO 2, tapped directly from the local supply rail. Keep analog trace short.
> - Neck windows (10 LED-pair groups, 3 transistors) combined onto one GPIO signal — all 3 transistor bases tied to same net, no component changes needed.
> - Pull-down resistors removed. Base resistors changed from 10kΩ to 1.2kΩ — matches LED current-limiting resistor value, reduces BOM to one resistor value on the board.
> - Left nacelle connector footprint corrected to match physical wiring layout.
> - **New GPIO assignments for revised PCB:** GPIO 20 = all neck windows (back of neck + both battle bridges wired together), GPIO 8 = left nacelle, GPIO 3 = right nacelle, GPIO 2 = battery ADC (270kΩ+100kΩ, ADC_11db, multiplier 3.70). New board installed in model 2026-06-18.
>
> **Nacelle polarity fix (2026-06-17):** Left and right nacelle connector footprints had positive and negative pads reversed on the EngRoom PCB. Discovered during assembly — nacelle LEDs would not light with correct wiring polarity. PCB updated in EasyEDA and new Gerbers exported. No firmware change needed.
>
> **GPIO 9 strapping pin fix (2026-06-18):** Right nacelle was originally assigned to GPIO 9, which is the BOOT strapping pin on ESP32-C3. The 1.2kΩ base resistor into the NPN transistor base-emitter junction pulled GPIO 9 LOW at power-on, putting the chip into download mode (boot freeze). Left nacelle on GPIO 8 was unaffected — Xiao board has a stronger external pull-up on GPIO 8. Fix: right nacelle moved to GPIO 3 in firmware and PCB. Old EngRoom board was not affected — nacelles were on GPIO 2/3, neither is a strapping pin.

> **Wiring issue resolved — nav/photon connector (2026-05-20):** Nav and photon wires were soldered in reverse order on the neck board connector. Connector re-pinned 2026-05-20 — firmware was already correct, no sketch change needed. Permanent fix applied in PCB revision above.

> **EngRoom lower PCB voltage regulator standoff — fixed in PCB revision 2026-05-21.** Regulator moved to clear the standoff. Decoupling caps moved with it.

### LED Count and Power Budget

| Section | LED count | LED pairs | Current @ 7.4V | Current @ 8.4V |
|---------|-----------|-----------|----------------|----------------|
| EngRoom (all LEDs) | 50 | 25 | 87.5mA | 107.5mA |
| Bridge (all LEDs) | 170 | 85 | 297.5mA | 365.5mA |

ESP32 quiescent (normal use, ESP-NOW active, no WiFi):
- Xiao ESP32-C3 (EngRoom): ~45mA
- ESP32-S3 (Bridge): ~55mA

**Total current draw — all LEDs on:**

| Section | @ 7.4V nominal | @ 8.4V full charge |
|---------|---------------|-------------------|
| EngRoom | ~133mA | ~153mA |
| Bridge | ~353mA | ~421mA |

Real-world draw is lower — nav lights blink at ~50% duty, not all groups active simultaneously.

### Battery Configuration and Runtime

| Section | Config | 350mAh cells | 1000mAh cells |
|---------|--------|-------------|---------------|
| EngRoom | 1 set (2S, 2 cells) | 350mAh | 1000mAh |
| Bridge | 2 sets (2S2P, 4 cells) | 700mAh | 2000mAh |

Total cells: 6 × 10440 LiPo (AAA form factor).

**Estimated runtime — all LEDs on (worst case):**

| Section | 350mAh cells | 1000mAh cells |
|---------|-------------|---------------|
| EngRoom | ~2.6 hrs | ~7.5 hrs |
| Bridge | ~2.0 hrs | ~5.7 hrs |

> **Battery note:** Original cells were 350mAh. Replacement/additional cells ordered are 1000mAh rated 10440. Bridge is the limiting section — 350mAh cells give ~2 hours worst case; 1000mAh cells give ~5.7 hours.

**Observed runtime — 2026-05-22 (real-world session):**

~2.5 hours of active use including all-LEDs-on testing, multiple OTA reflashes, and general operation. Cell voltages at end of session (measured per cell):

| Section | Start | End | Notes |
|---------|-------|-----|-------|
| Bridge (saucer) | ~4.2V/cell | ~3.33V/cell | Getting low — near safe discharge floor. Total pack ~6.66V. |
| EngRoom (stardrive) | ~4.2V/cell | ~3.66V/cell | Still some headroom. Total pack ~7.32V. |

Bridge drained faster as expected — 170 LEDs at ~353mA vs EngRoom's 50 LEDs at ~133mA. **Note: Bridge was running on only one set of cells (350mAh) — the second set was not installed.** Full 2S2P config would be 700mAh and would roughly double Bridge runtime. The faster drain vs EngRoom is entirely explained by the higher current draw on half the intended capacity. Consistent with the ~2-hour estimate for 350mAh cells. Charge before further use when cells reach ~3.3V/cell.

**Battery compartment contact issue (discovered 2026-05-22):** 10440 LiPo cells (AAA form factor) have a protective insulating wrapper that extends over the top of the positive terminal to prevent shorts. The dummy filler cells have the same wrapper. The metal contact spring in the battery compartment lid can catch on this wrapper when cells are inserted, preventing the spring from making solid contact with the terminal. There is no workaround — the wrapper cannot be removed or repositioned. **If the model has no power or behaves unexpectedly after a battery swap, the first thing to check is the battery contacts.** Remove and re-seat all cells, pressing them firmly until the compartment lid closes flush and the spring contacts sit squarely on the terminal ends.

### EngRoom Current Measurements (bench)

| Section | State | Current | Notes |
|---------|-------|---------|-------|
| Neck | Power on (all neck LEDs lit) | 0.25 A (250 mA) | Measured on bench PSU, 2026-05-20. Includes ESP32-C3 current (WiFi likely active during test — explains higher reading vs calculated ~133mA). |
| Lower EngRoom | — | pending | Measure once accessible |
| Bridge | — | pending | Measure once accessible |
| Full model | — | pending | Both sections powered, all LEDs on |

### EngRoom Power Options

Unlike the Bridge, the EngRoom has no DY-SV17F and no 5V requirement. The 2S (7.4V) 10440 LiPo setup is used for consistency with the Bridge, but it is not technically required. The main drawback of the current setup is that to recharge the batteries they have to be physically removed from the model.

Simpler alternatives for the EngRoom:

- **Rechargeable AAA (NiMH)** — 3x NiMH cells (~3.6V total). Drop-in swap with no wiring changes. Only modification needed is changing the **LED current-limiting resistors** to values appropriate for the lower voltage (see LED Resistors table above). **Do not change the transistor base resistors** — both populations are 1.2kΩ on the revised boards but only the LED resistors are supply-voltage-dependent. Update the BOM to the correct LED resistor value only — worth confirming against the schematic before ordering.
- **1S LiPo + USB charging circuit** — single cell with onboard charging (e.g. TP4056). Adds recharging without battery removal. Same LED resistor consideration applies — base resistors stay at 1.2kΩ.

Either option removes the need to pull batteries for charging at the cost of a LED resistor BOM change. The rechargeable AAA route is the least effort — no new components, just different LED resistor values.

**Mains/external power via stand (future option):** The model sits on a display stand with a cable channel. A USB-C PD trigger board set to 9V could supply power through the stand and into the stardrive section — eliminating batteries in EngRoom entirely for display use. A 9V regulated supply or USB-C PD source would replace the 2S LiPo pack. Powering the saucer (Bridge) from the same source would require a mod to route power up through the neck connection, which is feasible but would make separating the two model sections harder. Alternative for saucer: leave Bridge on its own battery pack, power EngRoom from the stand only.

**Back to 4.5V / 1S LiPo with USB charging (future option):** The original kit LEDs were designed for 4.5V (3× AAA). Changing the **LED current-limiting resistors** to values appropriate for 4.5V (or a 1S LiPo at 3.7–4.2V) would allow a simpler single-cell setup with onboard USB charging — same approach as the DataPad (SM5308 charger, USB-C accessible from outside). Requires an LED resistor BOM change on all boards and a new 1S charging circuit, but removes the need to ever pull batteries. Compatible with USB-C PD supply via stand if resistors are matched to the supply voltage. **Note:** The revised boards have transistor base resistors that are also 1.2kΩ — do not change these. Only the LED current-limiting resistors are supply-voltage-dependent. See LED Resistors section for the full breakdown and the ⚠️ warning about the two populations.

**LIR2032 coin cell option (considered, not pursued):** The original kit includes 3× dual CR2032 holders (2 cells each = 6V per holder) used for section lighting. Standard CR2032s are 3V primary cells, not compatible with the 2S LiPo circuit. However, LIR2032 rechargeable lithium coin cells have the same 3.6–4.2V range per cell as 10440s — meaning 2× LIR2032 in series would match the 2S 10440 pack voltage. The 3 existing coin cell holders could in theory be wired in parallel with the 10440 pack to add capacity without modifying the compartments. Decided against: current runtime is adequate, wiring in the coin cell holders adds complexity and the holders are not well positioned for this purpose, and LIR2032 capacity (typically 70–100mAh each) adds only minor capacity gain per holder.

---

## Saucer Lighting — Section Map

Wiring color codes:

| Code | Colors |
|------|--------|
| RB | Red / Black |
| RBlu | Red / Blue |
| YG / Gy | Yellow / Green |
| YW / WY | Yellow / White |
| GW | Green / White |
| YB | Yellow / Black |

### Saucer Sections by PCB and Connector

| Section | Top Connectors / PCB | Bottom Connectors / PCB |
|---------|----------------------|-------------------------|
| 1 | 36H 44D 28F — PCB 3,2,1 | 99C 83D (2-pin board 2 right) 65E*YG 99C*YB — PCB 1,2,3 |
| 2 | 24F 24E 18Fx2 40I 40H — PCB 1 | 76D (2-pin board 3 inboard) 62C 62C 01U*YB 62C 62C 71F — PCB 2,3 |
| 3 | 32C 48E 52C — PCB 2 | 76D* (2-pin board 3 outboard) 81E 105I#3 105I#2*RB — PCB 3 |
| 4 (top only) | 54D 53H 54D — PCB 3 | — |
| 5 | 35F 33C*YB 34G — PCB 2,3 | 101I 101I 101H 99C*R&BLU — PCB 3 |
| 6 | 43K 41H 42C — PCB 2 | 88G 88H — PCB 3 |
| 7 | 26E 27F 25E — PCB 1 | 84I 84H 84H — PCB 3 |
| 8 | 22K 22K 04J — PCB 1 | 73Gx3 (3 separate 4-pin connections) — PCB 2 |
| 9 | 04J 04J 19Ex4 20E 20E*RB 21I*YB — PCB 1 | 65E*RB 65E 60G 60H 67E 67E*YG — PCB 1,2 |
| 10 | 21I*RBlu 20E*YW 37C 38C — PCB 1 | 67E*RB 78H 78G 78G*RB — PCB 2,3 |
| 11 | 29J 30D 39F — PCB 2 | 78G*YW 92I 92H*YB — PCB 3 |
| 12 | 45D 46F 47F — PCB 2 | 92H*RBlu 96G 96G*YB — PCB 3 |
| 13 | 51E 50E*YW 49F — PCB 2 (50E split w/ impulse) | 96G*RBlu 107H 107I 105I*GY#2 105I#1*RB — PCB 3 |
| 14 | 01U*RBlu 01T — PCB 2 | 105I#1*YG 99D 83D — PCB 2,3 (2-pin board 2 left) |
| 15 (Bridge) | 01V — PCB 2 | — |
| 16 & 17 (Impulse) | 33C*RBlu 50E*RB — PCB 2,3 | — |
| 18 (Nav lights) | 58H 58E 58E 58F 58G — PCB 1 | — |

> Layout rule: always work **outer ring → inner ring**.
> "See Note" entries = 4-pin/2-LED connections split across panels — confirm wire combos once final layout is set.

---

## Upper Saucer Panel Map

Outer ring → inner ring order.

**Ring 1 (U1):**
| Panel | Connectors |
|-------|-----------|
| U1-01 | 36H 36H |
| U1-02 | 44D 44D |
| U1-03 | 28F |
| U1-04 | 24F 24E 24E |
| U1-05 | 18F 18F / 18F 18F |
| U1-06 | 40I 40I 40H |
| U1-07 | 32C 32C |
| U1-08 | 48E 48E |
| U1-09 | 52C 52C |

**Ring 2 (U2):**
| Panel | Connectors | Notes |
|-------|-----------|-------|
| U2-01 | 35F 35F | |
| U2-02 | 43K 43K | |
| U2-03 | 27F 27F | |
| U2-04 | 22K 22K | |
| U2-05 | 19E | Split — also feeds U3-11 |
| U2-05 | 19E 21I | 21I is for panel U33-12 |
| U2-06 | 21I 37C | |
| U2-08 | 39F 39F | |
| U2-09 | 47F 47F | |
| U2-10 | 51E | |

**Ring 3 (U3):**
| Panel | Connectors | Notes |
|-------|-----------|-------|
| U3-01 | 54D 53D 54D | |
| U3-02 | 33C 33C | One 33C = impulse engines |
| U3-03 | 34G 34G | |
| U3-04 | 41H 41H | |
| U3-05 | 42C 42C | |
| U3-06 | 26E 26E | |
| U3-07 | 25E 25E | |
| U3-08 | 22K 22K | Split with U2-04 |
| U3-09 | 04J 04J | |
| U3-10 | 04J 04J 04J 04J | |
| U3-11 | 19E x6 | Split with U2-05 and U2-06 |
| U3-12 | 20E 20E 20E | |
| U3-13 | 20E 37C | |
| U3-14 | 38C 38C | |
| U3-15 | 29J 29J | |
| U3-16 | 30D 30D | |
| U3-17 | 45D 45D | |
| U3-18 | 46F 46F | |
| U3-19 | 49F 49F | |
| U3-20 | 50E 50E | Split with impulse engine |
| Bridge | 01V 01V | |
| Bridge decks + lower | 01T 01T 01U | 01U split with L1-06 |

---

## Lower Saucer Panel Map

Outer ring → inner ring order.

**Outer ring (L02):**
| Panel | Connectors | Notes |
|-------|-----------|-------|
| L02-01 | 101I 101I 101H (outer) / 101I 101I (inner) | |
| L02-02 | 88H 88H / 88G 88G | |
| L02-03 | 84H 84H 84H 84H / 84I 84I | |
| L02-04 | 73G x6 | |
| L02-05 | 65E (see notes) / 65E 65E | |
| L02-06 | 60G 60G | |
| L02-07 | 67E 67E / 67E (see notes) | |
| L02-08 | 78H 78H 78G 78G / (67E, 78G see notes) | |
| L02-09 | 92I 92I / (78G, 92H see notes) | |
| L02-10 | 96G 96G / (92H, 96G see notes) | |
| L02-11 | 107H 107I 107I / (96G, 105H see notes) | |

Saucer/neck transition: 105H and 99B add into L2 panels (see notes).

**Inner ring (L01):**
| Panel | Connectors | Notes |
|-------|-----------|-------|
| L01-01 | 99B 99B | |
| L01-02 | 83D | |
| L01-03 | 65E | See notes |
| L01-04 | 71F | |
| L01-05 | 62C 62C | |
| L01-06 | 01U | Split with bridge decks |
| L01-07 | 62C 62C | |
| L01-08 | 76D | Split with L01-09 |
| L01-09 | 76D | Split with L01-08 |
| L01-10 | 81D | |
| L01-11 | 105H 105H | Split with L02-11, neck/saucer transition |

Center under bridge: 105I (split bridge/L2-11), 99D, 83D (split bridge/L1-02).

---

## Split 4-Pin Connector Reference

All connectors marked `*` on the PCB label are split — the 4-pin connector is shared between two different lighting sections. Only the specified pins belong to the section you are wiring. Connecting the wrong pins will light the wrong panel group.

**Wire colour codes:** RB = Red/Black · RBlu = Red/Blue · YB = Yellow/Black · YG/GY = Yellow/Green · YW/WY = Yellow/White

Organised by connector for easy lookup during assembly:

| Connector | Pins 1–2 | Wire | Pins 3–4 | Wire | Notes |
|-----------|----------|------|----------|------|-------|
| 01U | Section 2 | YB | Section 14 | RBlu | |
| 20E | Section 9 | RB | Section 10 | YW | ⚠️ Verify on assembly — known candidate for backwards LED connection |
| 21I | Section 9 | YB | Section 10 | RBlu | |
| 33C | Section 5 | YB | Section 16 & 17 | RBlu | |
| 50E | Section 13 | YW | Section 16 & 17 | RB | Section 13 share is split with impulse engines |
| 65E | Section 9 | RB | Section 1 | YG | |
| 67E | Section 10 | RB | Section 9 | YG | |
| 78G | Section 11 | YW | Section 10 | RB | |
| 92H | Section 11 | YB | Section 12 | RBlu | |
| 96G | Section 12 | YB | Section 13 | RBlu | |
| 99C | Section 1 | YB | Section 5 | RBlu | |
| 105I #1 | Section 13 | RB | Section 14 | YG | Split |
| 105I #2 | Section 3 | RB | Section 13 | YG | Split |
| 105I #3 | Section 3 | — | Section 3 | — | Not split — 4-pin, all pins Group 3 |

> ⚠️ **105I assembly note:** Three physically similar 4-pin connectors labelled 105I. #3 is not split — all pins belong to Group 3. #1 and #2 are split connectors whose pins span Groups 3, 13, and 14 across both. Verify each connector matches its label before seating — wrong connector = wrong group lit.

---

## Engine Section (Stardrive) Board Notes

- All lower lights on same circuit — not easy to separate
- Top LEDs: left and right independently
- Top middle dorsal: individual control
- Deflector: 2 individual LEDs on a 4-pin connector
- Option to add 2nd 2-pin connector for aft photon launcher

### Engine Section Connector Positions

| Position | Description |
|----------|-------------|
| 79G | Top front right |
| 86H | Top middle right |
| 95J | Lower front right |
| 03G | Deflector (2 LEDs, 4-pin) |
| 84G | Top mid left |
| 77L | Top front left |
| 74G | Beacon lights |
| 93G | Lower back / lower front left |
| 101J | Lower back |
| 71E | Top rear left |
| 74H | Top rear right |
| 102F | Power IN |
| 81E | Top middle dorsal (2-pin, 1 LED) |

---

## Battle Bridge PCB Notes

- Saucer PCB 1 starts on stage 31
- Left Battle Bridge (LBB): connectors 11d, 10f, 9e, 11e (back of neck)
- Right Battle Bridge (RBB): 22L
- Aft battle bridge: 11d 10f 9e 11e
- Flashing lights: pins 3 & 4
- Photon launcher: pins 1 & 2 — **note: change to red LED**
- Impulse engine: 14d (2-pin)

### Pull-down resistors — do not fit, do not include in future revisions

The Left Battle Bridge board had a fault where fitting the base pull-down resistor caused all LEDs on the board to stay permanently on. Root cause traced to the GND pad of the pull-down footprint being connected to the wrong net via the copper pour (EasyEDA clearance issue) — confirmed by multimeter showing a direct short where the schematic showed no connection. Replacing all SMD components made no difference; the fault is in the PCB copper.

**Resolution:** pull-down resistors removed from all battle bridge boards. Board operates correctly without them.

**Design decision:** do not include base pull-down resistors on any future board revisions. The ESP32 drives all LED pins as `OUTPUT` and actively holds them LOW when not commanded — this is lower impedance than any external pull-down resistor and makes the resistors redundant. The only window where pins can float is the brief boot period before `setup()` runs, which is not a concern for this application.

---

## Bridge PCB Revision 2026-05-21

All three Bridge PCBs revised. Current boards unchanged — firmware update deferred until new boards fitted.

- **PCB 2:** B0530W Schottky diode (SOD-123, 0.5A 30V, Vf≈0.34V) added on each 3.3V regulator output — prevents USB back-feed through battery regulator when battery is off. USB-C D+ and D- pins corrected (were swapped on original layout, required physical workaround to connect). **Hand-assembly substitute:** SS14 (SMA, 1A 40V) — electrically better, SMA body overhangs SOD-123 pads but both ends make solid contact. EasyEDA BOM retains B0530W for JLCPCB assembly.
- **PCB 2 EN pin circuit — fixed 2026-05-28:** Original schematic had the RC circuit reversed (3.3V → cap → EN → resistor → GND), holding EN low during boot. 100nF cap across reset switch also removed (unnecessary). EasyEDA updated to correct circuit: **3.3V → 10kΩ → EN pin → pushbutton → GND.** No capacitors. Confirmed working with new boards.
- **PCB 3:** Non-UART mode resistors removed from DY-SV17F mode selection array — only UART configuration resistors retained. Simplifies board with no functional change.
- **All boards:** Group 9 LED pin now correctly split between groups 9 and 1 — was only connected to group 9 on original layout. Battery voltage monitor added on GPIO 2 (ADC1) — voltage divider R1=270kΩ, R2=100kΩ on PCB, tapped from local supply rail close to GPIO 2. Maps 6.0V–8.4V → 1.62V–2.27V on ADC. Use ADC_11db attenuation in firmware.

---

## Nacelle PCB Notes

One board design used ×2 — port and starboard nacelles are identical boards. All components desoldered from original nacelle boards and transplanted to new PCBs, except the original power connector (footprint present on new board as a VCC injection point but not normally populated).

### SMD LEDs (YLED1206B, 1206)

6× blue 1206 SMD LEDs — main nacelle bussard collector glow ("warp flash" effect). Standard SMT assembly. Wired as 3 parallel pairs, each pair sharing a single current-limiting resistor, all 3 pairs switched together by one MMBT2222A transistor. LED specs: 20mA rated, Vf 2.6–3.1V.

**Current-limiting resistors (confirmed 2026-06-09):** Each parallel-pair resistor = **240Ω** (~8.75mA per LED at 7.4V nominal). Because each resistor serves a parallel pair, current won't split perfectly evenly (Vf mismatch), but the nacelle diffusers mask any visible imbalance — the only constraint is keeping the hotter LED under 20mA worst-case, which 240Ω satisfies comfortably.

### Through-hole footprints — not all are LEDs

The 4× through-hole LED footprints (204-10SUGC/S400-A5-L) serve different purposes and are all hand-assembled:

| Footprint use | Description | Resistor (0603) |
|---------------|-------------|-----------------|
| 2×3×4 square LED | Actual square LED fitted directly to board | **470Ω** |
| VCC injection point | Replicates original power connector — not normally used, available if board needs external power injected for bench testing | — |
| Forward nacelle LED | Red 3mm LED on ~3" wire leads — runs to mounting location inside nacelle | **470Ω** |
| Nav lights | Two 3mm LEDs pigtailed in parallel, ~3" wire leads each — driven from nav lights transistor | **1.2kΩ** |

470Ω chosen to match ~8.75mA per LED of the parallel SMD pairs at 7.4V nominal. Nav lights 1.2kΩ unchanged from standard board value.

Original board photos retained for reference wiring layout.

---

## PCB Gerber Files

Located in `enterprise documentation/PCB Design/`:

| Gerber zip | BOM | Pick & Place | Description |
|------------|-----|--------------|-------------|
| BridgePCB1.zip | BOM_BridgePCB1.xlsx | PickAndPlace_BridgePCB1.xlsx | Saucer PCB 1 |
| BridgePCB2.zip | BOM_BridgePCB2.xlsx | PickAndPlace_BridgePCB2.xlsx | Saucer PCB 2 (ESP32-S3, regulators, USB-C) |
| BridgePCB3.zip | BOM_BridgePCB3.xlsx | PickAndPlace_BridgePCB3.xlsx | Saucer PCB 3 (DY-SV17F sound player) |
| EngRoomPCB.zip | BOM_EngRoomPCB.xlsx | PickAndPlace_EngRoomPCB.xlsx | Engine room PCB (Xiao ESP32-C3) |
| NeckPCB.zip | BOM_NeckPCB.xlsx | PickAndPlace_NeckPCB.xlsx | Neck PCB |
| NacellePCB.zip | BOM_NacellePCB.xlsx | PickAndPlace_NacellePCB.xlsx | Nacelle LED controller (used ×2) |

EasyEDA project files in `PCB Design/projects/` — main project: `enterprise pcbs.eprj`.

---

## 3D Print Files

All 3D print files are in `enterprise documentation/`.

### DataPad Case — `Data Pad stl/`

| File | Description |
|------|-------------|
| `shell data pad top.stl` | Top half of DataPad PADD case |
| `shell data pad bottom.stl` | Bottom half of DataPad PADD case |
| `Body23.stl` | Internal body/mount component |

### PCB Test-Fit Prints — `pcbs test prints/`

Test prints used to verify PCB dimensions before ordering. Print these before placing a JLCPCB order to confirm boards will fit in the model.

| File | Description |
|------|-------------|
| `PCB1.stl` | Test fit for saucer PCB 1 |
| `pcb 2&3.stl` | Test fit for saucer PCB 2 & 3 stack |
| `Necell test.stl` | Test fit for nacelle PCB |

### WarpCore — `Warp_Core/`

Full archive of the original Thingiverse design used for this build. The original listing (`thingiverse.com/thing:1656741`) is no longer available, so the complete download is preserved here.

**Design:** ST:TNG Warp Core by **ElmoC**
**License:** Creative Commons Attribution — Non-Commercial — No Derivatives (CC BY-NC-ND 3.0)
**Attribution file:** `attribution_card.html`, `LICENSE.txt`, `README.txt`

The original design uses a custom PCB (OSH Park order) and an Arduino Nano with a non-ESP32 sketch (`WarpCore/WarpCore.ino` — AVR only, uses SoftPWM, kept for reference). This project replaces that with an Arduino Nano ESP32 and a custom sketch.

| File/Folder | Description |
|-------------|-------------|
| `files/*.stl` | All printable parts (chamber, injectors, light rings, uprights, base, etc.) |
| `files/*.brd` / `*.zip` | Original Eagle PCB files — unmodified |
| `WarpCore/WarpCore.ino` | Original Arduino sketch — unmodified, AVR only |
| `files/Instructions.pdf` | Original build instructions |
| `images/` | Original Thingiverse photos |

**Note on alternatives:** Other warp core designs exist on Thingiverse in different sizes and configurations, including versions designed around ARGB LED strips rather than discrete NPN-transistor-driven LEDs. These may be worth considering for anyone building from scratch — ARGB gives colour control (the moving green/blue glow effect) vs. the single-colour PWM approach used here.

---

## DataPad (Waveshare 7" ESP32-S3) — Physical Build

3D-printed PADD prop case housing the Waveshare ESP32-S3-Touch-LCD-7 (800×480, 7" display). Self-contained unit with internal 1S LiPo, two independent charging paths, and a local sound module.

### Battery

3000mAh 1S LiPo, internal to the printed case.

**Observed runtime — 2026-05-22:** ~7 hours of active use including UI interaction, OTA reflashing, ESP-NOW communication, and screen on throughout. Battery was not fully depleted at the end of the session — exact remaining charge not measured. Power draw not yet characterised (screen backlight + ESP32-S3 + DY1703A audio player — screen likely dominates). The 3000mAh capacity is generous for this use case; runtime is not a concern for normal display sessions.

### Power Switch

SPDT 2-position switch wired as a transfer switch on the battery positive rail. The common terminal connects to battery+. The two throws connect to the load rail and the SM5308 charger BAT pin respectively — so only one path is connected to the battery at a time.

- **ON:** battery connects to Waveshare board and all electronics. External SM5308 charger is disconnected from battery. The Waveshare board has a built-in LiPo charger that could charge the battery in this state, but the Waveshare USB-C ports are not accessible with the case closed and fully assembled — so in practice charging does not occur in the ON position during normal use.
- **OFF:** Waveshare board and all electronics are disconnected from battery. External SM5308 charger is connected to battery and charges it. This is the only charging path available without disassembling the case.

The SPDT transfer arrangement naturally prevents the SM5308 and the load from being on the battery simultaneously, which avoids the charge termination issue that occurs when a load draws current through an SM5308 (no power-path management on that IC).

### Charging

Only one charging method is usable without opening the case:

**External SM5308 charger — USB-C port on case exterior:**

1S LiPo charger module based on the **SM5308** IC. BAT pin connects to the SPDT switch (OFF throw). Charge by flipping the switch to OFF and connecting USB-C to the external port. The board is unpowered while charging.

The Waveshare board has a built-in LiPo charger accessible via its own USB-C ports, but those ports are not reachable when the case is closed. That path is only available during bench work with the case open.

### Sound — DY1703A

- DY Electronics DY1703A module, RS485 connector, UART1: TX=GPIO16, RX=GPIO15, 9600 baud
- Same DY Electronics 0xAA frame protocol as the Bridge DY-SV17F — identical command structure
- Requires 5V; supplied by a small boost converter from the 3.3V rail (Waveshare only exposes 3.3V)
- SD card: files `00001.mp3`–`00044.mp3` in root (no folders). Full file map in `SOUND_MAP.md`.
  - Files 1–25: shared with Bridge DY-SV17F — same file number = same sound on both players
  - Files 26–44: DataPad-only sounds (touch feedback, voiced alerts, ambient loops, easter eggs)

### Mounting

Case attaches to Waveshare screen with M3×4 screws. Waveshare USB-C port is not accessible without disassembly — use the external SM5308 USB-C port for normal charging.

---

## WarpCore Hardware

**Board:** Arduino Nano ESP32 (drop-in replacement for original Arduino Nano)
**Sketch:** `WarpCore_ESP32/WarpCore_ESP32.ino`
**mDNS:** `WarpCore.local`
**MAC:** `20:6e:f1:31:da:54`

Original Arduino Nano sketch (`WarpCore/WarpCore.ino`) kept as reference only — uses SoftPWM library, AVR-only, not portable to ESP32.

**Original assembly note:** First build used solid-core ethernet wire throughout. Solid core has no flex tolerance — connections broke during handling and required resoldering. Use stranded wire for any rework.

### Pin assignments

| Pin | Function |
|-----|----------|
| A4, A3, A2, A1, A0 | Chase LEDs 1–5 (NPN transistors) |
| D2, D3, D4, D5, D6 | Chase LEDs 6–10 (NPN transistors) |
| A5 | Ambient LED (NPN transistor) |
| A7 | Speed pot (wiper; ends to 3.3V and GND) |
| D7 | LD2410C OUT (presence detection) |

**Note:** Use `D2`–`D6` Arduino constants in firmware, not raw integers `2`–`6`. On the Nano ESP32, raw `2` = GPIO 2 = A1 on the connector — wrong pin.

### LD2410C mmWave Presence Sensor

- **Supply:** 5V from a small boost converter stepping up the Nano ESP32's 3.3V rail
- **Logic:** 3.3V OUT pin, push-pull — no pull-up resistor needed on D7
- **Configuration:** UART pins not yet wired (only OUT is connected). Plan is a spare Arduino Nano as USB-to-serial passthrough to HLKRadarTool on a PC for detection-range/gate-sensitivity tuning — see `DESIGN_STORY.md` Presence Detection section. OUT = HIGH on presence, LOW on clear.
- **Firmware behaviour:** rising edge on D7 → sends `LED_RADAR_TRIG` to DataPad only (not directly to Bridge/EngRoom — DataPad holds the authoritative on/off state and calls `PowerUpAll()` if the model is off, avoiding a startup/trigger loop). 3-second debounce lockout. Ignored if model already on.
- **Enable/disable toggle:** implemented — SENSORS ON/OFF button on DataPad Screen 2, persisted to NVS (`radar_on`) on both WarpCore and DataPad via `LED_RADAR_EN`.
- **Sensor housing:** new WarpCore base with an integrated sensor bay (single 0.4mm PLA wall as the radar window) is designed and printed — not yet fitted. See `DESIGN_STORY.md` "Remaining to Complete the Project".
