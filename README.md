# Galaga-Inspired Shooter Game (Embedded Systems)

**Author:** Andy Payan  
**Course:** CS/EE 120B – Introduction to Embedded Systems
**Platform:** AVR Microcontroller  

## Overview
This project is a Galaga / Space Invaders–inspired shooter game developed as a custom final project for an embedded systems course. The game runs entirely on an AVR-based system and integrates real-time input handling, graphics rendering, and state-driven gameplay using multiple hardware peripherals.

The objective is to survive endless waves of enemies while maximizing score. Difficulty increases progressively, and players must strategically use limited power-ups and lives to beat their high score.

---

## Key Features
- Real-time game loop implemented on AVR hardware
- Dual-display system:
  - **128×128 SPI TFT LCD** for gameplay rendering
  - **16×2 character LCD** for score, high score, and lives
- Custom pixel sprites for player, enemies, and bullets
- Increasing difficulty with faster enemies per wave
- Power-up system with visual LED indicator
- Physical joystick and button-based controls
- Persistent high-score tracking per game session

---

## Gameplay Summary
- Player starts with **3 lives** (no regeneration)
- +1 point per alien destroyed
- −1 point per alien that passes the player
- −1 life per collision with an alien
- Game ends when all lives are lost

### Power-Up Mode
- Earned after clearing **3 consecutive waves**
- Indicated by a **blue LED**
- Activated via button press
- Lasts **3 seconds**
- Doubles ship movement speed and firing rate

---

## Hardware Components
- AVR Microcontroller
- 128×128 HiLetgo SPI TFT LCD
- 16×2 Character LCD
- Analog joystick with switch
- Push buttons (Start / Reset)
- Blue LED (power-up indicator)
- Potentiometer
- Resistors and jumper wires

---

## Software & Libraries
- **C (AVR-GCC)**
- `stdlib.h` – integer-to-string conversion (`itoa`) for LCD output
- `spiAVR` – SPI communication and TFT LCD control
- `serialATmega` – serial debugging and runtime verification
- `helper.h` – bit manipulation macros (`setbit`, `getbit`)
- `periph.h` – joystick input handling
- `time.h` – random enemy wave generation
- `LCD.h` – 16×2 LCD control and custom character rendering
- **Custom `sprite.h`**
  - Hand-drawn pixel sprites
  - Optimized sprite rendering functions

---

## Technical Highlights
- Implemented a state-machine–driven game architecture
- Managed concurrent peripherals (SPI display, LCD, GPIO, ADC)
- Designed custom sprite rendering for constrained hardware
- Optimized input polling and timing for smooth gameplay
- Debugged using serial output for real-time verification

---

## Known Limitations
- Passive buzzer not implemented due to timer resource constraints
- Minor cosmetic flickering during horizontal movement

---

## Skills Demonstrated
- Embedded C programming
- AVR microcontroller development
- SPI communication
- Real-time systems
- Hardware/software integration
- Low-level debugging
- Game logic and state management

---

## Demo Video
https://www.youtube.com/watch?v=n11RDavdv7s


