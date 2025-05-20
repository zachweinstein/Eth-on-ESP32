## Project Requirements

1. The system shall use the host ESP32’s Wi-Fi by default to connect to the network.

2. The system shall provide an ethernet interface using W550.

3. The system shall switch to ethernet automatically when an RJ45 ethernet cable is plugged in.

4. The system shall switch back to Wi-Fi when the RJ45 ethernet cable is unplugged.

5. The system shall be written C/C++ using FreeRTOS.

6. The system shall use SPI protocol to interface W5500 ethernet module to the ESP32

7. The system shall switch between ethernet and Wi-Fi and vice versa within five (5) seconds.

8. The system must provide feedback to the user to indicate that a switch has been made.

9. The system shall host a static web server on the ESP32.

10. The system shall allow the client to connect to the server hosted by ESP32.
