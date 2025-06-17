
# 🐄 Vermoo Animatronic Cow System

This project controls the Vermoo animatronic cow rig using Bottango SOLAR, IMPULSE, and NOVA boards, along with linear actuators, audio playback, and an automated show schedule. It supports both automated looped shows and manual control triggers.

## 🚀 Project Goals
- Fully animatronic cow rig mounted on a 1937 Chevy farm truck.
- Runs standalone without a PC using SOLAR/NOVA/IMPULSE boards.
- Plays audio from SOLAR SD card through Fosi amp to Polk speakers.
- Animations advance automatically on a schedule.
- Starts/stops automatically during shop operating hours.
- Manual triggers via onboard buttons; expandable to more controls.

## ⚙ Hardware Overview
| Component | Purpose |
|------------|---------|
| SOLAR board | Controls up to 10 servos |
| IMPULSE boards | Adds servo channels (10 per board) |
| NOVA board | Show controller, triggers, sync |
| Fosi V3 amp | Audio amplification |
| Polk RC85i | Speakers |
| Linear actuators | Rig/door movement |
| Reed switches | Detect rig/door position |
| Qwiic GPIO expander | Extra button inputs (optional) |
| Power supply 5-6V | Board + servo power |
| Fans/vents | Active cooling |

## 🎬 Software Logic
- Fetches animation list from Bottango API.
- Plays animations in sequence.
- Resets playhead to start for each animation.
- Checks current time against operating hours.
- Loops animations; pauses outside operating hours.
- Clean exit with Ctrl+C.

## 🕒 Operating Hours
| Day | Start | End |
|------|-------|-----|
| Mon-Thu | 16:00 | 20:30 |
| Fri | 13:00 | 21:00 |
| Sat-Sun | 13:00 | 20:30 |

## 📂 File Structure
```
Vermoo-Cow-Project/
├── cow_controller.py
├── README.md
├── diagrams/
│   ├── cow_flowchart.png
│   └── cow_state_diagram.png
├── audio/
└── .gitignore
```

## ⚡ Power & Cooling
- Each board uses 5-6V power.
- Distribute servos to reduce heat load.
- Use fans/vents for ventilation.
- Protect against moisture and debris.

## 📝 Custom Code Needs
- Actuator logic to ensure rig/door positions.
- Expanded button support (Qwiic or custom patterns).
- Switch between standalone and PC control.

## 💡 Future Enhancements
- Remote triggers (phone app, wireless remote).
- Rain sensors to block shows in bad weather.
- Live puppeteering support (PC required).

## 🛠 Example Usage
- Run: `python cow_controller.py`
- Stop: `Ctrl + C`

## 🙏 Acknowledgments
Thanks to Evan (Bottango) for hardware and support.


