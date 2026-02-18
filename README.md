**<center>Terrarium Monitor</center>**

>An integration of ESP32-CAM with a custom HTTP API backend and Flutter to monitor tree plant growing

<hr>

![Flutter](https://img.shields.io/badge/Flutter-%2302569B.svg?style=for-the-badge&logo=Flutter&logoColor=white)![Arduino](https://img.shields.io/badge/-Arduino-00979D?style=for-the-badge&logo=Arduino&logoColor=white)![C++](https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white)![HTTP](https://img.shields.io/badge/HTTP-API-blue?style=for-the-badge)

**<center>Table of Contents</center>**

- [List of Materials](#list-of-materials)
  - [Terrarium](#terrarium)
  - [Hardware](#hardware)
- [Objective](#objective)
- [Workflow](#workflow)
- [ESP32](#esp32)
  - [Configuration](#configuration)
  - [Pseudo-code](#pseudo-code)
- [API Backend](#api-backend)
  - [API Endpoint](#api-endpoint)
    - [Authentication](#authentication)
    - [Request Format](#request-format)
    - [Example Request](#example-request)
    - [Response](#response)
    - [Backend Implementation Notes](#backend-implementation-notes)
- [App](#app)
- [PCB](#pcb)
- [3D Files](#3d-files)

# List of Materials

## Terrarium

- [X] Hinged Acrylic Box with holes for cross-ventilation
- [X] Expanded Foam
- [X] Glue for foam
- [X] Tree Bark
- [X] Black paper
- [X] Transparent Tape
- [X] Branches
- [X] Sand
- [X] Moss

## Hardware

- [X] ESP32-CAM
- [X] Light Sensor
- [X] White LEDs
- [X] AHT10 temperature and humidity sensor
- [X] Resistors
- [X] Diodes
- [X] Transistors
- [X] Cable and charger for powering
- [X] PCB
- [X] Exhausting Fan
- [X] 3D-printed case for the circuit
- [X] 3D-printed structure for the White LEDs "strip"
- [X] Wires
- [X] JWT Connectors

# Objective

Build an insect terrarium with image and sensor-based data monitoring and environment automation. The image and sensor-based data is sent to a custom HTTP API backend and retrieved by a mobile app. The sensor-based data is used to turn on exhausting fans and LEDs.

![Terrarium](README_Images/Terrarium.jpg)

# Workflow

The workflow works as depicted in the picture below.

![Workflow](README_Images/TerrariumMonitor.png)

The ESP32 captures temperature, air humidity, and light every 2 minutes. If the temperature or humidity exceeds the set threshold, the exhaust fans are activated. If the light is below the set threshold, the LEDs are turned on. After this, the data is sent to a custom HTTP API backend. Every day, at 9 AM and 4 PM, the image is captured, and all the data is uploaded to the backend.

The backend API (running at `********`) receives multipart form data containing:
- Timestamp
- Light intensity (lux)
- Temperature (°C)
- Humidity (%)
- JPEG image file

The API is secured with an API key (`*********` header). The backend stores the images and sensor data, making them available for the mobile app.

A Flutter-based app retrieves the images and sensor-based data from the API and displays it in the screen in form of image and graphs.

# ESP32

The final code for the ESP32-CAM is in this [folder](ESP32/terrariumMonitor/). I split the actuators and sensors to make it more organized. All the sensitive information is on `secrets.h` (which I obviously did not push to Git). 

## Configuration

In `secrets.h`, define:
```cpp
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"
```

The API endpoint and key are configured directly in the main sketch:
```cpp
const char* API_ENDPOINT = "***********";
const char* API_KEY_HEADER = "*********";
const char* API_KEY_VALUE = "****************";
```

## Pseudo-code

``` bash

INCLUDE required libraries (Arduino, WiFi, HTTPClient, Camera, LittleFS, Sensors)

DEFINE camera pin assignments

CONFIGURE API endpoint and authentication
    - API URL: ***********
    - API Key header: *********
    - API Key value: ****************

SETUP timing variables for periodic data sending (2 minutes)

DECLARE variables for sensor readings (lux, temperature, humidity)

DECLARE function prototypes

FUNCTION setup():
    Start serial communication
    Print startup message
    Test actuators (LEDs and motors)
    Initialize WiFi and wait for connection
    Configure time via NTP
    Initialize camera with optimal settings
    Initialize LittleFS filesystem
    Initialize sensors (light, temperature, humidity)
    Print setup complete

FUNCTION loop():
    IF 2 minutes have passed:
        
        Read sensors (lux, temp, humidity)
        Print sensor values
        Control actuators based on sensor thresholds
        Get current timestamp (via NTP or uptime fallbacks
        Send data to API via HTTP POST multipart/form-data:
            - timestamp
            - lux value
            - temperature value
            - humidity value
        IF upload successful:
            Print success message
        ELSE:
            Print error message
    IF 9 AM or 4 PM:
        Delete previous image file if it exists
        Capture photo and save to LittleFS with timestamp filename
        Send data to API via HTTP POST multipart/form-data:
            - timestamp
            - lux value
            - temperature value
            - humidity value
            - image file (JPEG)
        IF upload successful:
            Delete local image file to free space
            Print success message
        ELSE:
            Print error message
FUNCTION capturePhotoSaveLittleFS(photoPath):
    Flush camera buffer (3 warmup captures)
    Capture photo
    IF capture failed: print error and return
    Print photo file name
    Ensure parent directory exists (create if needed)
    Open file for writing
    IF file open failed: print error and return
    Write photo data to file, flush, and close
    Print file saved message with size
    Verify file exists and size matches written bytes
    IF verification passed: print success
    ELSE: print warning

FUNCTION initLittleFS():
    Mount LittleFS with format option, restart if failed

FUNCTION initWiFi():
    Connect to WiFi with credentials from secrets.h
    Wait for connection, print status and IP address

FUNCTION initCamera():
    Configure camera settings (pins, resolution, quality)
    IF PSRAM detected: use UXGA resolution
    ELSE: use SVGA resolution
    Initialize camera, restart if failed
    Configure sensor parameters (brightness, contrast, exposure, etc.)
    Print camera initialized message

FUNCTION sendDataToAPI(imagePath, lux, temp, humi, timestamp):
    Open image file from LittleFS
    IF file open failed: return false
    Initialize HTTP client with API endpoint
    Add API key header
    Generate random boundary for multipart form data
    Build multipart body with:
        - Form field: timestamp
        - Form field: lux
        - Form field: temperature
        - Form field: humidity
        - Form field: image (binary JPEG data)
    Send HTTP POST request manually:
        - Send headers (Host, *********, Content-Type, Content-Length)
        - Send form fields
        - Stream image file in 512-byte chunks with progress logging
        - Send closing boundary
    Wait for server response (10s timeout)
    Parse HTTP response code
    IF code is 200 or 201: return true (success)
    ELSE: print error and return false

```

# API Backend

The system uses a custom HTTP API backend running at `********`.

## API Endpoint

**POST** `/api/upload`

### Authentication
Requests must include the `*********` header with value: `************************************`

### Request Format
Content-Type: `multipart/form-data`

**Form fields:**
- `timestamp` - Unix timestamp of the data collection
- `lux` - Light intensity in lux (float)
- `temperature` - Temperature in Celsius (float)
- `humidity` - Humidity percentage (float)
- `image` - JPEG image file

### Example Request
```http
POST /api/upload HTTP/1.1
Host: 10.0.0.50:8080
*********: ************************************
Content-Type: multipart/form-data; boundary=----WebKitFormBoundaryXYZ

------WebKitFormBoundaryXYZ
Content-Disposition: form-data; name="timestamp"

1700000000
------WebKitFormBoundaryXYZ
Content-Disposition: form-data; name="lux"

450.50
------WebKitFormBoundaryXYZ
Content-Disposition: form-data; name="temperature"

24.50
------WebKitFormBoundaryXYZ
Content-Disposition: form-data; name="humidity"

65.30
------WebKitFormBoundaryXYZ
Content-Disposition: form-data; name="image"; filename="1700000000.jpg"
Content-Type: image/jpeg

[binary image data]
------WebKitFormBoundaryXYZ--
```

### Response
- **200 OK** or **201 Created** - Data successfully received and stored
- **401 Unauthorized** - Invalid or missing API key
- **400 Bad Request** - Invalid form data

### Backend Implementation Notes
The backend should:
- Validate the API key
- Parse multipart form data
- Store images with timestamp-based filenames
- Store sensor readings in a database (linked by timestamp)
- Optionally implement data retention policies (e.g., delete data when the image folder exceeds 10GB)

# App

The App was made using Flutter and deployed locally. It retrieves all the readings and images from the custom API backend. The last image and data is shown on the screen. All the data is used to plot line graphs. The 8 most recent images are shown on the screen. If the user wants to see more, they click "See More" and 8 more will appear.

The app communicates with the backend API to:
- Retrieve images with their associated timestamps and sensor data
- Display historical data (24h) in graph format

![Main Page](README_Images/Screenshot%20Main.jpg)

When clicking on past images, the image will pop-up and be shown bigger on the screen, with the Date and Time, and data measured at the same time.

![Previous Data](README_Images/Screenshot%20Previous.jpg)

# PCB

Under the [PCB](PCB) folder, you can find the schematics, gerber file and PCB layout for the PCB. I used a simple circuit with transistor, diode and resistor to activate and deactivate the LED and exhausting fans.

![Schematics](README_Images/Schematics.png)

![PCB](README_Images/PCB.png)

# 3D Files

Under the [3D Files](3DFiles) folder, you can find all the necessary files to print the case for the PCB and the LEDs/AHT10 sensor encasing.

**PCB Case:**

![PCB Case](README_Images/Case.png)

**Sensors Case:**

![Sensors Case](README_Images/Sensor.png)
