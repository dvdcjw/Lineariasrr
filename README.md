# Lineariasrr - DirectInput 8 Force Feedback (FFB) LUT Wrapper

A lightweight, per-game DirectInput 8 (`dinput8.dll`) proxy wrapper designed to intercept steering wheel Force Feedback commands, apply a custom Look-Up Table (LUT), and send the linearized output forces to the physical wheel base.

This wrapper is primarily designed to improve force feedback linearity and eliminate motor deadzones on entry-level gear- or belt-driven steering wheels (such as Logitech G25/G27/G29/G920/G923 or Thrustmaster T150/T300) in games that do not natively support LUT files.

---

## :warning: Anti-Cheat Warning

> [!WARNING]
> This wrapper DLL (as with all mods in general) is intended for **single-player games**. Many online multiplayer games with anticheat systems might flag this mod as a cheat vector and result in a **ban**. Use it at your own risk.

---

## Installation & Setup

Follow these steps to generate a linearization profile for your specific wheel and install it in your game:

### Step 1: Install the Wrapper
1. Download the precompiled **`dinput8.dll`** from releases and copy it into your game's main executable folder (where the game's main `.exe` file is located).

### Step 2: Generate Your Custom LUT Profile
By measuring your wheel's specific hardware characteristics and correct them, you can generate a calibration file:
1. Follow [this guide](https://www.overtake.gg/downloads/lut-generator-for-ac.9740) to generate a LUT table specifically for your wheel (Skip if you already have an LUT table). 
8. Rename the generated file to **`ffb_lut.txt`**, and copy it into the game's main executable folder next to **`dinput8.dll`**.

> [!NOTE]
> The generated file will consist of input-to-output ratios (values between `0.0` and `1.0`). If you prefer, you can also write/edit this file manually.
> Example `ffb_lut[ffb_test.cpp](ffb_test.cpp).txt` (to compensate for a 5% motor deadzone):
> ```
> 0.0, 0.0
> 0.02, 0.08   # Maps 2% game force to 8% motor output to clear deadzone
> 0.05, 0.13   # Maps 5% game force to 13% motor output
> 0.10, 0.19
> 0.40, 0.49
> 1.00, 1.00
> ```

### Step 3: Configure In-Game Settings
1. Launch the game.
2. Navigate to the game's in-game controller/FFB settings.
3. **Set "Min Force" or "Minimum Force" to `0%`!** Because your custom LUT now fully handles deadzone correction and linear force scaling at the driver level, keeping this setting active in-game will cause force feedback to feel overly harsh or clip.

### Step 4: Verify It is Running
Check the **`dinput8_lut.log`** file created in your game folder after launch. You should see entries confirming the device wrapper initialized and successfully intercepted FFB updates:
```
[INIT] Logger started. Log file: ...\dinput8_lut.log
[SYSTEM] Loading real dinput8.dll from: C:\WINDOWS\system32\dinput8.dll
[MAIN] DirectInput8Create called.
[LUT] Successfully loaded LUT with 50 entries.
[DEVICE] Proxy device created. Waiting for FFB calls...
[FFB] SUCCESS: Intercepted first FFB call!
```

---

## Features

- **Dynamic FFB Interception**: Intercepts `IDirectInputDevice8W` effect parameters (specifically `GUID_ConstantForce`) at the API level.
- **Custom LUT Mappings**: Applies linear interpolation based on the local `ffb_lut.txt` file.
- **Lightweight & Thread-Safe**: High-performance COM proxy interface with thread-safe execution.
- **Diagnostics Tool Included**: Includes `ffb_test.exe` to scan for FFB devices, configure signed axis limits, and apply test forces to verify the LUT behaves correctly without launching a game.

---

## How It Works

Windows applications load libraries like `dinput8.dll` by searching the local application directory before looking in system folders (DLL redirection). 

By placing this wrapper in the game's executable folder:
1. The game loads our custom `dinput8.dll`.
2. Our wrapper loads the real system DLL from `C:\Windows\System32\dinput8.dll` and forwards all standard calls.
3. When the game initializes a force feedback device and creates a Constant Force effect, our wrapper intercepts the COM interfaces (`IDirectInputDevice8` and `IDirectInputEffect`).
4. Whenever the game updates the FFB force magnitude (via `SetParameters` or `CreateEffect`), the wrapper intercepts the raw torque value, queries the LUT file, applies the linearization math, and forwards the modified force to the wheel driver.

---

## Calibration and Testing Tool (`ffb_test.exe`)

The included console application allows you to test the FFB loop safely without running a full game.

1. Ensure your steering wheel is plugged in, powered, and active.
2. Run `ffb_test.exe` from a command prompt.
3. The utility will:
   - Load the local wrapper DLL and scan for attached FFB game controllers.
   - Acquire exclusive access and turn off default auto-centering.
   - Apply a centering force.

---

## Compiling from Source

If you wish to compile the binaries yourself, you will need a C++ compiler (such as GCC/MinGW-w64).

Run the included **`build.bat`** script, or compile via command prompt:

```bash
# Compile the dinput8.dll wrapper
g++ -O2 -shared -std=c++17 -static -o dinput8.dll main.cpp logger.cpp lut.cpp proxy_device.cpp proxy_dinput.cpp dinput8.def -ldxguid -lkernel32 -luser32 -lole32

# Compile the testing utility
g++ -O2 -std=c++17 -static -o ffb_test.exe ffb_test.cpp -ldinput8 -ldxguid -lkernel32 -luser32
```

---

## License

This project is open-source and free to modify or distribute. Use at your own discretion.
