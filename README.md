# The Globask

**An Arduino-based interactive device for visualizing collective opinion on nuanced questions.**

*Rafaelle Bueno & Natasha Litherland — ID4, CY Tech / CY Cergy Paris Université, March 2026 — Prof. Pierre Andry*

---

## What is the Globask?

The Globask is a physical prototype that invites people to express opinions on a spectrum rather than as a binary yes/no. It was born from a simple observation: social media has conditioned us to think in extremes. The Globask pushes back on that — it accepts any position between 0% and 100% agreement, aggregates responses from multiple users, and displays the evolving collective average through colored LEDs and a motorized globe structure.

The name is a contraction of *globe* and *ask*, reflecting both the physical structure and the idea of weighing opinion.

---

## Concept

Each user is presented with a question on an LCD screen. They turn a potentiometer to position themselves on a yes/no spectrum — not forced to choose a side. When they validate, their response is added to a running average. The LED strip updates to show the new collective balance (magenta for yes, cyan for no), and a servo motor shifts position to reflect both the average and the spread of opinions — a proxy for polarization.

The goal is to make visible something that is usually hidden: that most people's views are somewhere in the middle, even when online discourse makes it look like everyone has picked a team.

---

## Hardware

| Component | Role | Notes |
|---|---|---|
| Arduino UNO R3 (ELEGOO kit) | Main microcontroller | 5V logic |
| LCD1602 module | Displays questions and averages | 16×2 characters; connected via 6 digital pins |
| Servo motor (SG90) | Reflects average position and polarization | Limited to 180° rotation; ~75g max load |
| Stepper motor (28BYJ-48) | Rotates the globe rings during result animation | 2048 steps/rev; driven at 12 RPM |
| NeoPixel LED strip (56 LEDs, 5V) | Visualizes yes/no balance in color | Cut from a 5m reel; assembled on a 30cm metal hoop |
| Rotary potentiometer (×2) | One for yes/no input; one for LCD brightness | Both from ELEGOO kit |
| Joystick buttons (×3, button part only) | Start, validate, change question | Used as simple push buttons |
| Breadboard + jumper wires | Wiring | Many wires — a known limitation |
| Cardboard base | Physical structure | Replaced an intended custom enclosure |
| Salvaged terrestrial globe frame | Holds the metal hoops | Repurposed; provides the globe aesthetic |
| Two 30cm metal hoops | Visual display rings | Lightweight substitute for original plexiglass plan |

### What we originally planned vs. what we built

The original design called for overlapping transparent plexiglass discs (cyan, magenta, yellow) that would physically separate to show polarization. This had to be abandoned: the SG90 servo cannot lift more than ~75g (tested empirically), plexiglass was too heavy, and custom glass rods were unavailable. We also tried rice paper colored with alcohol markers — too fragile.

The final design uses the metal hoops from the salvaged globe frame, which fall within the servo's weight limit. The polarization effect that was originally spatial (discs moving apart) is instead encoded in the servo angle and LED color balance.

---

## Software

**Language:** Arduino C++  
**Libraries:**
- `LiquidCrystal.h` — LCD display
- `Servo.h` — servo motor control
- `Stepper.h` — stepper motor control
- `Adafruit_NeoPixel.h` — LED strip control

### State machine

The firmware runs a three-state loop:

| State | Description |
|---|---|
| `0` — Waiting | Shows "Press Start". Displays current average on LEDs. Awaits Start button. |
| `1` — Input | Shows the active question. User adjusts potentiometer. Displays live yes/no split. Awaits Validate. |
| `2` — Result | Shows updated average on LCD and LEDs. Servo settles to reflect average + dispersion. Awaits Start for next user. |

### Key logic

**Potentiometer input:** To prevent accidental jumps when a new user picks up the device, the potentiometer is only activated once it moves more than 15 analog units from its resting position (`potActive` flag). This avoids the previous user's position being immediately inherited.

**Average calculation:** Running totals are stored per question (`sommeYesQ[]`, `nbRepQ[]`). The average is recalculated on each validation. Minimum and maximum values are also tracked to compute the dispersion range.

**Servo positioning:** The servo angle is mapped from the average yes percentage (40°–140°). If dispersion (max − min across all responses) is low (≤20%), the servo holds steady at the average position. If dispersion is high (≥60°), the servo shifts by up to 40° from center — a physical signal that the group is divided. Values between these thresholds are interpolated linearly.

**Animation:** On validation, a 7-second animation runs — the servo oscillates and the stepper rotates the globe — before settling to the final average position. This creates a moment of suspense and draws attention to the result reveal.

**Reset:** Holding the Start button for 2 seconds resets all stored data across all questions.

### Questions

Three questions are hardcoded in the firmware:
```cpp
const char* questions[NB_QUESTIONS] = {
  "Jaune va gagner ?",
  "Guerre en 2026 ?",
  "Cours d'arduino ??"
};
```
To change them, edit this array in the source. Note the LCD1602 displays only 16 characters per line — questions longer than 16 characters will be truncated. This was discovered during testing and the questions were shortened accordingly.

---

## Wiring overview

```
Arduino UNO R3
├── Pin 7–12       → LCD1602 (RS, E, D4–D7)
├── Pin 2          → Start button (INPUT_PULLUP)
├── Pin 3          → Validate button (INPUT_PULLUP)
├── Pin 4          → Change question button (INPUT_PULLUP)
├── Pin 5, 6, A2, A4 → Stepper motor (28BYJ-48 via ULN2003)
├── A0             → Rotary potentiometer (yes/no input)
├── A1             → Servo motor (SG90)
├── A3             → NeoPixel LED strip (56 LEDs)
└── External 5V    → Powers LED strip and stepper (separate from USB)
```

> **Note:** The LED strip draws significant current. Powering it from the Arduino's onboard 5V pin is not recommended. The prototype uses a separate 5V supply. Insufficient power was a source of erratic LED behavior during development; reinforcing the physical connections (more glue) also helped.

---

## How to use

1. **Connect** both power cables (USB to computer for Arduino, external 5V for LEDs/stepper).
2. **Upload** `codenatasharafaelle.ino` via Arduino IDE.
3. The LCD displays *"Appuie Start pour lancer"*.
4. **Press Start** to begin a session for a new user.
5. The active question appears. **Turn the potentiometer** to set your position between No (0%) and Yes (100%). The LCD shows your split live.
6. **Press Validate** to submit. The globe animates for 7 seconds, then settles. The LCD and LEDs show the updated average.
7. **Press Start** again to pass to the next user (resets the potentiometer position without clearing the cumulative average).
8. **Press Change Question** at any time to switch between the three questions.
9. **Hold Start for 2 seconds** to reset all stored data.

---

## Experimental protocol & results

| Test | Method | Goal | Result | Change made |
|---|---|---|---|---|
| Question display | Power on with question loaded | Text displays correctly | Displayed but truncated | Shortened question text |
| User input | Potentiometer + joystick buttons | Easy input | Input works; buttons unclear | Added written labels on base |
| Average calculation | Multiple users submit responses | Average updates correctly | Correct, with occasional LED bugs | Reinforced LED wiring with glue |
| Load/stability | Progressively weighted paintbrush on servo | Find servo weight limit | ~75g; cannot lift plexiglass | Switched to metal hoops; changed to rotation rather than lifting |
| User comprehension | Observation + feedback from testers | Interaction is intuitive | Understood quickly with written labels | Added button labels before second test |

---

## Known limitations

| Area | Issue |
|---|---|
| Aesthetics | Many exposed wires; cardboard base; lacks the visual polish of the original vision |
| Ergonomics | Servo cannot support plexiglass; the overlapping colored disc effect was never achieved |
| LEDs | Prone to false contacts and high current draw; occasional glitches during operation |
| Persistence | Data is stored in RAM only — a full power cycle resets all responses |
| Scale | Only 3 hardcoded questions; no way to add questions without editing and re-uploading firmware |
| Screen | 16×2 LCD limits question length and display richness |

---

## What we learned

- Servo motors have hard physical limits — calculate load requirements before designing around them
- LED strips are electrically demanding and mechanically fragile; soldering (rather than breadboard connections) would have prevented most of the contact issues
- User testing early revealed interface problems (unclear buttons) that a label fix resolved — simple but effective
- Collaboration requires negotiating design decisions under real constraints, not ideal ones
- The gap between an idea and what a kit can physically support is larger than expected

---

## If we were to redo this

- Calculate servo torque requirements upfront rather than discovering limits through trial and error
- Solder the LED strip connections instead of relying on breadboard/glue
- Build a proper enclosure from scratch to hide wiring and give the object a cleaner appearance
- Reduce total wire count through better circuit planning
- Consider a microcontroller with more pins and memory to support more questions or persistent storage

---

## Repository structure

```
├── codenatasharafaelle.ino   # Main Arduino sketch
├── README.md                 # This file
```

---

## License

This project was made for an educational course (CY Tech, 2026). It is shared freely for learning and reuse. No formal license is applied.
