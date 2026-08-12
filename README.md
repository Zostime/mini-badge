# Mini Badge
This is a tiny digital badge created by Zostime

## Main Peripherals
- **MCU:** STM32F103CBT6
- **Display:** 1.14" 240x135 IPS LCD
- **Connectivity:** 16-Pin Type-C connector
- **Audio:** 4KHz Buzzer
- **Power:** 3.7V Li-ion battery with charging circuit
- **Interaction:** 3 Buttons
- **Storage:** TF Card slot
- **Timekeeping:** RTC real-time clock
- **Debugging:** ST-Link programming interface
- **Communication:** USART interface

## Quick Start
### Requirements
- Mini Badge
- TF Card (FAT32)
- Keil MDK 5.42 or later
- ST-Link V2
- 4P 0.5mm FPC cable
- 4P 0.5mm FPC connector

### Steps
1. Git clone [Mini-Badge](https://github.com/Zostime/Mini-Badge.git)
2. Open `bootloader/MDK-ARM/Project.uvprojx` with Keil, build and flash.
3. Copy `sdcard/` content to a TF card.
4. Insert TF card, power on.
