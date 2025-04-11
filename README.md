I'm excited to share our team's (Jerry Team) latest maze-solving robot that we've built for the "Mobile Robots in the Maze" competition at Óbuda University, Hungary. This is our third-generation robot, and we've made significant improvements based on our experiences from previous years.

In previous competitions, we used Arduino-based controllers, but this year we've upgraded to an ESP32, which has been a game-changer for our robot's capabilities and development process.

About the Robot:

Jerry 3.0 is a compact (16×16 cm) maze-solving robot that navigates using an ESP32 as its brain. The ESP32 WROOM 32 microcontroller on our Wemos D1 R32 board handles all the sensor processing and motor control with its impressive 240MHz dual-core processor and abundant I/O capabilities.

One of the most valuable features we've implemented is utilizing the ESP32's WiFi capabilities to create a web interface for real-time monitoring and tuning. During testing, we set up the ESP32 in SoftAP mode, allowing us to connect directly to the robot with our phones. Through this interface, we can view live sensor data, adjust PID parameters, and even load different profiles (like "sprint mode" for maximum speed or more conservative settings for precise navigation). This has been incredibly helpful for fine-tuning the robot's behavior without having to reprogram it constantly.

The robot uses infrared distance sensors to detect walls and maintain its position in the maze corridors. We've implemented a Kalman filter for the sensor readings to reduce noise and improve accuracy. For navigation, we use an RFID reader (connected via SPI, not I2C as we initially planned) to read tags placed throughout the maze that contain directional information.

The robot's movement is controlled by two DC motors with an L298N motor driver, allowing for tank-style steering. We've also added an MPU-6050 accelerometer to precisely measure rotation angles during turns, which has significantly improved our navigation accuracy compared to previous versions.

Technical Details:

The code is structured around several key components:

Sensor Processing: The ESP32 reads data from three IR distance sensors and processes it through Kalman filters to get stable distance measurements.
PID Control: We use a PID controller for wall following, which keeps the robot centered in corridors or at a consistent distance from a single wall.
RFID Navigation: The MFRC522 RFID reader detects tags in the maze that contain navigation instructions.
Web Interface: The ESP32 hosts a web server that displays real-time sensor data and allows parameter adjustments. This has been invaluable during development and testing.
Motion Control: The robot can perform precise turns using gyroscope feedback and adjusts its speed based on the distance to obstacles.
The most challenging part was getting the wall-following algorithm to work reliably. Our solution adapts to different scenarios: when there are walls on both sides, it centers itself; when there's only one wall, it maintains a fixed distance; and when there are no walls, it uses gyroscope data to maintain its heading.

What We've Learned:

Moving from Arduino to ESP32 has been a significant upgrade. The additional processing power allows us to implement more complex algorithms, and the WiFi capability has transformed our development process. Being able to tune parameters in real-time without connecting to a computer has saved us countless hours during testing.

The ESP32's dual-core architecture also lets us handle multiple tasks simultaneously without performance issues. One core handles the sensor readings and motor control, while the other manages the web interface and communication.

