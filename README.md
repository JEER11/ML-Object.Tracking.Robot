# AI Object Tracking System  

This project was developed by NJIT students in the **IEEE Hardware and Computer (HAC) Club**.  
It is an **AI-powered robotic car** that uses an **ESP32-CAM** for vision, **OpenCV + YOLOv3** for object detection, and a motor driver to move towards detected objects. A buzzer provides audio feedback when a target object (like a bird) is found.  

---

## Hardware Setup  

- **ESP32-CAM** (AI Thinker)  
- **Robot chassis car kit** (4 DC motors)  
- **Motor Driver (L298N)**  
- **Passive Buzzer**  
- **External Antenna** (for ESP32-CAM WiFi stability)  
- **Battery / Power Bank**  
- **Jumper wires**  

### Pin Connections  
**ESP32-CAM → L298N Motor Driver:**  
- IN1 → GPIO 14  
- IN2 → GPIO 15  
- IN3 → GPIO 13  
- IN4 → GPIO 12  
- 5V → 5V  
- GND → GND  

**Passive Buzzer:**  
- GPIO 4 → Buzzer (+)  

---

## Software Stack  

### ESP32-CAM (C++ – Arduino IDE)  
- Libraries: `esp32cam`, `WiFi.h`, `WebServer.h`  
- Hosts a **web server** that provides:  
  - Camera image endpoint (`/cam-lo.jpg`, `/cam-mid.jpg`, `/cam-hi.jpg`)  
  - Motor control endpoints (`/move?action=forward|left|right|stop`)  
  - Buzzer endpoint (`/buzzer?action=on`)  

### Python (PC side)  
- Libraries: `opencv-python`, `numpy`, `requests`  
- Uses **YOLOv3** with COCO dataset classes  
- Fetches camera frames via HTTP  
- Runs object detection (`bird` in this demo)  
- Sends movement and buzzer commands to ESP32-CAM  

---

## How It Works  

1. ESP32-CAM connects to WiFi and starts a web server.  
2. Python script requests live frames (`.jpg`) from the ESP32-CAM.  
3. YOLOv3 model processes each frame to detect objects.  
4. If the target object is detected:  
   - Car moves forward briefly.  
   - Buzzer beeps as feedback.  
5. If no object is detected:  
   - Car rotates slightly, scanning the environment.  

This loop continues until the target object is found.  

---

## Usage  

1. Flash the ESP32-CAM code via **Arduino IDE**.  
2. Note the **ESP32-CAM IP address** printed in Serial Monitor.  
3. Update the Python script with that IP (`CAM_URL`, `BUZZER_URL`, etc.).  
4. Run the Python script:  


---

#Demo

![hippo]https://github.com/user-attachments/assets/16e4fcb4-5187-4aa3-b608-444fa1f9a86c
![34F7142D-93B7-4DB3-AF35-B10407E8313A](https://github.com/user-attachments/assets/607ff002-d39f-4a9d-a990-8ee9665bc01e)
![BF6DFAB5-95AD-4F32-A197-A6414CB4EFD2](https://github.com/user-attachments/assets/536d9363-6264-4d89-ad1d-20368b9c754c)

```bash
python object_tracking.py



