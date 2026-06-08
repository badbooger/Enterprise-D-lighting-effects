# Assembly Notes — FFC Routing & Component Mounting

A handful of build photos showing how cabling and key components were fitted inside the model. Not a step-by-step manual — more a visual reference for the trickiest parts if you're doing a similar retrofit: routing flat flex cables (FFC) through the kit's structural frame, and fitting the speaker and DataPad electronics into limited space.

See `HARDWARE.md` for connector pinouts and the full hardware reference.

---

## FFC routing through the frame

Flat flex cables were chosen specifically because they slip through gaps that already exist in the kit's structural frame — no cutting, drilling, or filing required. Regular wire bundles are too thick to route the same way. The connection points between sections cap the cable width at 8-pin, which is what shaped the two-cable split (control signals / power) through the stardrive.

![Cable routing through the stardrive frame](assembly_photos/cable%20routing.jpg)

Stardrive (EngRoom) section, showing both the nacelle and neck FFC runs coming through the structural framing to the EngRoom board — the cables follow existing channels in the frame rather than running over or through it.

![Neck section FFC routing](assembly_photos/Neck%20routing%202.jpg)

Neck-to-EngRoom connection — the FFC routed between the two sections, wrapped in electrical tape to help prevent shorting against the frame.

The neck bottom plate is normally held on by 5 screws. The last 2 (closest to the cable run) were left out here to avoid putting pressure on the FFC at that point — they can be fitted if you're careful about clearance, but aren't required for this to go together properly.

![Close-up of the taped FFC at the neck connection](assembly_photos/Neck%20routing%20and%20protection%201.jpg)

Closer look at the same run and tape wrap.

![Nacelle FFC routing 1](assembly_photos/Nacelle%20Routing%201.jpg)

FFC entering the nacelle assembly from the pylon — routed along the inside of the structure to the nacelle's LED groups.

![Nacelle FFC routing 2](assembly_photos/Nacelle%20Routing%202.jpg)

Close-up of the same run, showing the cable bridging from the pylon frame into the nacelle body.

---

## Speaker mounting

There are two speakers for the DY-SV17F sound player, secured to the lower framing with zip ties — one on the left, one on the right. With careful placement they clear everything else in the saucer (wiring, PCB stack, LED ring) and fit nicely.

![Speaker mounting 1](assembly_photos/Speaker%20mounting%201.jpg)

Speaker zip-tied to the lower framing, positioned clear of the surrounding window-group wiring and PCB.

![Speaker mounting 2](assembly_photos/Speaker%20mounting%202.jpg)

Closer view of the zip-tie mount and connector.

![Speaker mounting 3](assembly_photos/Speaker%20mounting%203.jpg)

Wider view showing the speaker's position relative to the LED ring PCB — also visible here is the FFC routing: the connections sit underneath the boards, and the cable slips easily between the hull panels and the framing as needed.

---

## DataPad internal wiring

![DataPad internal wiring](assembly_photos/Data%20Pad%20Wiring.jpg)

Inside the 3D-printed DataPad case: speaker, charger board, ESP32, and LiPo battery. See the DataPad section of the root `README.md` and `HARDWARE.md` for the full hardware rundown — battery, charging, and power switch wiring.
