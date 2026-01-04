# BMW iDrive G-Series ZBE4 CAN Protocol

Research findings for BMW G-series iDrive controller (ZBE4) touchpad functionality.

**Hardware tested:** ZBE-06-TC, Part number 65826829079

---

## CAN Bus Configuration

| Parameter | Value |
|-----------|-------|
| Bus Type | K-CAN4 |
| Speed | 500 kbps |
| Used In | G-series (G20, G30, etc.) |

---

## CAN Message IDs

### Messages We Use

| ID (Hex) | Direction | Description |
|----------|-----------|-------------|
| 0x273 | TX | Rotary encoder init request |
| 0x277 | RX | Rotary encoder init reply |
| 0x264 | RX | Rotary encoder data |
| 0x267 | RX | Main input data (buttons, joystick) |
| 0x317 | TX | **Touchpad poll command (G-series specific!)** |
| 0x0BF | RX | Touchpad response data |
| 0x202 | TX | Backlight control |
| 0x501 | TX | Keepalive poll |
| 0x5E7 | RX | Status messages |

### Messages NOT Used (reference only)

| ID (Hex) | Description | Notes |
|----------|-------------|-------|
| 0x510 | K-CAN4 wake-up | Not needed for touchpad |
| 0x03C | Ignition status | Not needed for touchpad |
| 0x560 | Bus alive (K-CAN2) | F-series only |

---

## Initialization Sequence

1. **Rotary Init**: Send `0x273` with `{0x1D, 0xE1, 0x00, 0xF0, 0xFF, 0x7F, 0xDE, 0x04}`
2. **Wait for response**: Receive `0x277`
3. **Light Control**: Send `0x202` with `{0xFD, 0x00}` (ON) or `{0xFE, 0x00}` (OFF)
4. **Start touchpad polling**: Send `0x317` every 5-20ms

---

## Touchpad Protocol (G-Series ZBE4)

### KEY DISCOVERY: Poll Message Format

**CRITICAL**: byte[0] bit4 (0x10) must be SET for coordinates to update!

```cpp
// Correct poll message - simple and works!
uint8_t data[8] = {0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
can_.Send(0x317, data, 8);
```

| byte[0] value | Result |
|---------------|--------|
| 0x10, 0x11, 0x31, etc. (bit4=1) | ✅ Coordinates UPDATE |
| 0x00, 0x01, 0x21, etc. (bit4=0) | ❌ Coordinates FROZEN |

**Note:** Cycling byte[0] is NOT required. Fixed `0x10` works perfectly.

### Operation Mode

G-series ZBE4 touchpad is **POLL-BASED**:
- Send poll message to `0x317`
- Each poll returns current touch state on `0x0BF`
- Recommended poll interval: **5-20ms** (50-200Hz) for smooth mouse movement

### Touchpad Response Format (0x0BF) - Multi-Touch Capable

```
┌────────────────────────────────────────────────────────────────────────┐
│                       8-Byte Touch Message                             │
├────────┬────────┬────────┬────────┬────────┬────────┬────────┬────────┤
│ Byte 0 │ Byte 1 │ Byte 2 │ Byte 3 │ Byte 4 │ Byte 5 │ Byte 6 │ Byte 7 │
├────────┼────────┼────────┼────────┼────────┼────────┼────────┼────────┤
│ Counter│ F1 X   │ F1 Y-L │ F1 Y-H │ State  │ F2 X   │ F2 Y-L │ F2 Y-H │
└────────┴────────┴────────┴────────┴────────┴────────┴────────┴────────┘

Byte 0: Sequence counter (low nibble cycles 0-F)
Byte 1: Finger 1 X coordinate (8-bit)
Byte 2: Finger 1 Y low byte
Byte 3: Finger 1 Y high byte
Byte 4: Touch state
Byte 5: Finger 2 X coordinate (8-bit, 0x00 if single finger)
Byte 6: Finger 2 Y low byte (0x00 if single finger)
Byte 7: Finger 2 Y high byte (0x00 if single finger)
```

### Touch States (byte[4])

| Value | Meaning | Bytes 5-7 |
|-------|---------|-----------|
| 0x11 | No finger (released) | 00 00 00 |
| 0x10 | 1 finger touch | 00 00 00 |
| 0x00 | 2 fingers touch | Finger 2 data |
| 0x1F | 3 fingers touch | Finger 2 data |
| 0x0F | 4 fingers touch | Finger 2 data |

### Multi-Touch Example

```
Two fingers moving in circles:
[3E 7A 61 1A 00 D4 71 15]
 │  │  └──┬──┘ │  │  └──┬──┘
 │  │     │    │  │     └── F2 Y = 0x1571 (5489)
 │  │     │    │  └──────── F2 X = 0xD4 (212)
 │  │     │    └─────────── State = 0x00 (two fingers)
 │  │     └──────────────── F1 Y = 0x1A61 (6753)
 │  └────────────────────── F1 X = 0x7A (122)
 └───────────────────────── Seq = 0x3E

Single finger:
[34 CB A0 19 10 00 00 00]
                │  └─────── No Finger 2 data
                └────────── State = 0x10 (one finger)
```

### Coordinate System

**Finger X Coordinate (byte[1] for F1, byte[5] for F2):**
- Range: **0x00-0xFF** (0-255)
- 0x00 = left edge
- 0xFF = right edge

**Finger Y Coordinate (bytes[2-3] for F1, bytes[6-7] for F2):**
- 12-bit value, little-endian
- Range: approximately **0x000-0x1FFF** (0-8191)
- Lower values = bottom edge
- Higher values = top edge

### Raw Coordinate Processing (Updated for Multi-Touch)

```cpp
// Finger 1 coordinates
uint8_t f1_x = msg.data[1];
uint16_t f1_y = msg.data[2] | (static_cast<uint16_t>(msg.data[3]) << 8);

// Touch state
uint8_t state = msg.data[4];
bool two_fingers = (state == 0x00);

// Finger 2 coordinates (only valid when state == 0x00)
uint8_t f2_x = msg.data[5];
uint16_t f2_y = msg.data[6] | (static_cast<uint16_t>(msg.data[7]) << 8);

// Multi-touch gesture detection
if (two_fingers) {
    // Calculate pinch/zoom from distance between fingers
    int16_t dx = f2_x - f1_x;
    int16_t dy = f2_y - f1_y;
    float distance = sqrt(dx*dx + dy*dy);

    // Calculate rotation from angle between fingers
    float angle = atan2(dy, dx);
}
```

**Multi-Touch Use Cases:**
- Pinch to zoom (distance change between fingers)
- Two-finger scroll (both fingers moving same direction)
- Rotation gesture (angle change between fingers)

---

## Button & Joystick Protocol

### Input Message Format (0x267)

```
Byte 0: Counter
Byte 1-2: Reserved
Byte 3: State (lower nibble) | Direction (upper nibble for joystick)
Byte 4: Input type
Byte 5: Button/input ID
Byte 6-7: Reserved
```

### Input Types (byte[4])

| Value | Type |
|-------|------|
| 0xC0 | Button press |
| 0xDD | Joystick move |
| 0xDE | Joystick center press |

### Button IDs (byte[5])

| Value | Button |
|-------|--------|
| 0x01 | Menu |
| 0x02 | Back |
| 0x04 | Option |
| 0x08 | Radio |
| 0x10 | CD/Media |
| 0x20 | Nav |
| 0x40 | Tel |

### Joystick Directions (byte[3] upper nibble)

| Value | Direction |
|-------|-----------|
| 0x01 | Up |
| 0x02 | Right |
| 0x04 | Down |
| 0x08 | Left |
| 0x00 | Center |

### Input States (byte[3] lower nibble)

| Value | State |
|-------|-------|
| 0x00 | Released |
| 0x01 | Pressed |
| 0x02 | Held (long press) |

---

## Rotary Encoder Protocol

### Rotary Data Format (0x264)

```
Byte 0-2: Reserved
Byte 3: Position low byte
Byte 4: Position high byte (with direction hints)
```

Position is 16-bit value that wraps around. Calculate delta from previous position.

---

## Configuration Summary

### Current Working Settings

```cpp
// Timing
constexpr uint32_t kPollIntervalMs = 5;    // 200Hz for smooth mouse

// Multipliers (for raw coordinates)
constexpr int kXMultiplier = 5;            // X: 512 steps
constexpr int kYMultiplier = 30;           // Y: 32 steps (needs more amplification)
constexpr int kMinMouseTravel = 1;         // 1 step threshold
```

### Mouse Movement Calculation

```cpp
// Delta from raw coordinates
int16_t delta_x = current_x - prev_x;
int16_t delta_y = current_y - prev_y;

// Apply multipliers (Y inverted for screen coordinates)
int8_t mouse_x = delta_x * kXMultiplier / 10;
int8_t mouse_y = -delta_y * kYMultiplier / 10;  // Inverted!
```

---

## Key Differences: G-Series vs F-Series

| Feature | F-Series | G-Series ZBE4 |
|---------|----------|---------------|
| Touchpad TX ID | 0x0BF | **0x317** |
| Touchpad RX ID | 0x0BF | 0x0BF |
| Poll message | {0x21, ...} | **{0x10, ...}** |
| Key bit | Unknown | **bit4 (0x10) enables coords** |
| Y range | 0-30 (documented) | **0-31 (tested)** |

---

## Sources

### GitHub Repositories

- [thatdamnranga/iDrive](https://github.com/thatdamnranga/iDrive) - Arduino library (F-series)
- [IAmOrion/BMW-iDrive-HID](https://github.com/IAmOrion/BMW-iDrive-HID) - BLE HID implementation

### Forum Threads

- [K-CAN4 messages for iDrive controller ZBE4](https://www.bimmerfest.com/threads/k-can4-messages-for-idrive-controller-zbe4.1475910/)
- [ZBE4 K-CAN4 Wake Up Message](https://g30.bimmerpost.com/forums/showthread.php?t=2024962)

---

*Last updated: January 2025*
*Touchpad protocol fully decoded with multi-touch support!*
*Supports up to 2 simultaneous fingers for gestures.*
