# safeNsound

SafeNSound is a wearable safety system designed to enhance situational awareness using thermal sensing, haptic feedback, and audio alerts. Built on the Adafruit ESP32 Feather, it detects nearby individuals with an MLX90640 thermal camera and alerts the userer using a vibration motor (DRV2605) and a piezoelectric siren. The system is ideal for environments where personal safety and awareness are critical, such as walking alone or working in low-visibility conditions.

**Features**:
🔥 **Thermal Detection:** MLX90640 thermal camera monitors surrounding temperature changes.
🛑 **Audio Alerts:** Piezoelectric siren triggered via physical button.
⚡ **Haptic Feedback:** DRV2605 vibration motor activates when heat threshold is exceeded.
📡 **IoT Integration:** Sends center temperature readings to Adafruit IO for remote monitoring.
🧠 **Embedded Logic:** Runs on an ESP32 using C++ and Arduino libraries.

**Components:**
-Adafruit ESP32 Feather
-MLX90640 Thermal Camera
-DRV2605L Haptic Motor Controller
-Piezo Siren
-Push Button
-Rocker Switch
-Adafruit IO (for data visualization)
