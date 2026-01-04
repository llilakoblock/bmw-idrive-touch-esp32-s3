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

### Touchpad Response Format (0x0BF)

```
Byte 0: Counter (increments with each message)
Byte 1: X coordinate raw (0-255 per half)
Byte 2: Lower nibble = half indicator (0=left, 1=right)
        Upper nibble = unknown flags
Byte 3: Y coordinate raw (0-31)
Byte 4: Touch type
Byte 5-7: Reserved (always 0x00)
```

### Touch Types (byte[4])

| Value | Meaning |
|-------|---------|
| 0x11 | No finger (finger removed) |
| 0x10 | 1 finger touch |
| 0x00 | 2 finger touch |
| 0x1F | 3 finger touch |
| 0x0F | 4 finger touch |

### Coordinate System

**X Coordinate (byte[1] + byte[2]):**
- Left half (byte[2] & 0x0F == 0): raw 0-255
- Right half (byte[2] & 0x0F == 1): raw 0-255
- **Combined range: 0-511** (left half 0-255, right half 256-511)
- Total resolution: **512 steps**

**Y Coordinate (byte[3]):**
- Raw range: **0-31** (32 steps)
- 0 = bottom edge
- 31 = top edge

### Raw Coordinate Processing (Recommended)

For maximum precision, use raw coordinates without mapping:

```cpp
// Extract raw X with half combination
uint8_t raw_x = msg.data[1];
uint8_t x_lr = msg.data[2] & 0x0F;
int16_t combined_x = (x_lr == 1) ? (256 + raw_x) : raw_x;  // 0-511

// Extract raw Y
int16_t raw_y = msg.data[3];  // 0-31
```

**Why raw coordinates?**
- F-series mapping (map to -128...127) loses 50% X precision
- For relative mouse movement, only deltas matter
- Raw deltas preserve full sensor resolution

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
*Touchpad protocol fully decoded and working!*
