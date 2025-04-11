# Jerry 3.0 – Maze-Solving Robot

We're excited to share our team's (Jerry Team) latest maze-solving robot, built for the **"Mobile Robots in the Maze"** competition at **Óbuda University, Hungary**. This is our **third-generation robot**, and it features major upgrades based on the lessons we've learned from previous years.

In earlier competitions, we used Arduino-based controllers, but this year we've upgraded to an **ESP32**, which has significantly improved our robot’s performance and development workflow.

---

## About the Robot

**Jerry 3.0** is a compact, 16×16 cm robot designed to solve mazes autonomously. It’s powered by an **ESP32 WROOM-32** microcontroller on a **Wemos D1 R32** board, which handles all sensor processing and motor control thanks to its powerful **240 MHz dual-core processor** and extensive I/O support.

### Key Features

- **Web Interface via WiFi**  
  We leverage the ESP32’s built-in WiFi by putting it in SoftAP mode, which allows us to connect directly to the robot from our phones.  
  Through a custom web interface, we can:
  - View live sensor data
  - Tune PID parameters
  - Switch between profiles (e.g., "sprint mode" or "precision mode")  
  This real-time tuning has dramatically streamlined our testing process.

- **Infrared Sensors & Kalman Filtering**  
  The robot uses **IR distance sensors** to detect and follow maze walls. We apply a **Kalman filter** to reduce sensor noise and improve accuracy.

- **RFID Navigation**  
  An **MFRC522 RFID reader** (connected via SPI) detects directional tags placed throughout the maze, helping the robot make smarter navigation decisions.

- **Motion Control**  
  Two **DC motors** with an **L298N driver** provide tank-style movement. An **MPU-6050 accelerometer/gyroscope** measures rotation during turns, allowing for accurate and consistent movement.

---

## Technical Overview

### System Components

- **Sensor Processing**  
  Reads from 3 IR sensors and applies Kalman filters for stable distance measurements.

- **PID Control**  
  Used for wall following: keeps the robot centered or maintains a constant distance from a single wall.

- **RFID Navigation**  
  Reads maze navigation tags with the MFRC522 module via SPI.

- **Web Interface**  
  Hosted by the ESP32’s built-in server. Enables real-time monitoring and live tuning without reprogramming.

- **Motion Control**  
  Gyroscope-assisted turns and dynamic speed adjustments based on obstacle distance.

---

## Wall-Following Algorithm

This was one of the biggest challenges. Our adaptive wall-following logic:
- Centers the robot when walls are detected on both sides
- Maintains a set distance if only one wall is detected
- Relies on gyroscope data to maintain heading when no walls are present

---

## What We’ve Learned

- Switching from Arduino to **ESP32** was a huge leap forward.  
- The extra processing power enabled more complex algorithms.
- Real-time tuning via WiFi saved countless hours during testing.
- The **dual-core architecture** lets us run sensor control and web communication in parallel, eliminating performance bottlenecks.

---

Thanks for checking out Jerry 3.0!  
We hope this inspires others building robots for maze-solving challenges.