#include <WiFi.h>
#include <esp32cam.h>
#include <WebServer.h>

#define BUZZER_PIN 4

#define IN1 14
#define IN2 15
#define IN3 13
#define IN4 12

const char* WIFI_SSID = "Hooly2G";
const char* WIFI_PASS = "18-sienna-6159";

WebServer server(80);

static auto loRes = esp32cam::Resolution::find(320, 240);
static auto hiRes = esp32cam::Resolution::find(800, 600);

unsigned long lastCommandTime = 0;
unsigned long patrolTimer = 0;
bool patrolLeft = true;

bool drive_active = false;
unsigned long drive_start = 0;
const int DRIVE_DURATION = 5000;

bool buzzer_active = false;
unsigned long buzzer_start = 0;

bool motion_detected = false;
unsigned long lastMotionCheck = 0;
const int MOTION_INTERVAL = 1500;
size_t lastFrameSize = 0;

void setupMotors() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  stopMotors();
}

void moveForward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  drive_start = millis();
  drive_active = true;
}

void moveBackward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  drive_start = millis();
  drive_active = true;
}

void turnLeft() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void turnRight() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void serveJpg() {
  auto frame = esp32cam::capture();
  if (frame == nullptr) {
    server.send(503, "", "");
    return;
  }
  server.setContentLength(frame->size());
  server.send(200, "image/jpeg");
  WiFiClient client = server.client();
  frame->writeTo(client);
  lastCommandTime = millis();
}

void handleRoot() {
  String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <title>ESP32 Robot Control</title>
    <style>
      body { background-color: #000; color: white; font-family: Arial; text-align: center; }
      h2 { font-size: 28px; margin-top: 20px; }
      img { border: 3px solid #990000; border-radius: 10px; margin-top: 10px; width: 320px; }
      .btn { width: 80px; height: 50px; margin: 6px; font-size: 16px; font-weight: bold; background-color: #660000; color: white; border: none; border-radius: 8px; }
      .btn:hover { background-color: #990000; }
      .row { display: flex; justify-content: center; }
      .row > .btn { margin: 5px; }
    </style>
  </head>
  <body>
    <h2>Camera Feed</h2>
    <img id="cam" src="/cam-lo.jpg"><br>
    <script>
      setInterval(() => {
        document.getElementById("cam").src = "/cam-lo.jpg?ts=" + new Date().getTime();
      }, 1000);
    </script>
    <h2>Robot Controller</h2>
    <div class="row"><button class="btn" onclick="sendCmd('forward')">UP</button></div>
    <div class="row">
      <button class="btn" onclick="sendCmd('left')">LEFT</button>
      <button class="btn" onclick="sendCmd('stop')">STOP</button>
      <button class="btn" onclick="sendCmd('right')">RIGHT</button>
    </div>
    <div class="row"><button class="btn" onclick="sendCmd('backward')">DOWN</button></div>
    <script>
      function sendCmd(cmd) {
        fetch('/move?action=' + cmd);
      }
    </script>
  </body>
  </html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void handleMove() {
  String action = server.arg("action");
  lastCommandTime = millis();
  if (action == "forward") moveForward();
  else if (action == "backward") moveBackward();
  else if (action == "left") turnLeft();
  else if (action == "right") turnRight();
  else stopMotors();
  server.send(200, "text/plain", "OK");
}

void handleBuzzer() {
  digitalWrite(BUZZER_PIN, HIGH);
  buzzer_start = millis();
  buzzer_active = true;
  lastCommandTime = millis();
  server.send(200, "text/plain", "Buzzer on");
}

void handleFlash() {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(2000);
  digitalWrite(BUZZER_PIN, LOW);
  lastCommandTime = millis();
  server.send(200, "text/plain", "Flash done");
}

void handleCam() {
  serveJpg();
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  setupMotors();

  using namespace esp32cam;
  Config cfg;
  cfg.setPins(pins::AiThinker);
  cfg.setResolution(hiRes);
  cfg.setBufferCount(2);
  cfg.setJpeg(80);

  bool ok = Camera.begin(cfg);
  Serial.println(ok ? "CAMERA OK" : "CAMERA FAIL");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/move", handleMove);
  server.on("/buzzer", handleBuzzer);
  server.on("/flash", handleFlash);
  server.on("/cam-lo.jpg", handleCam);

  server.begin();
  lastCommandTime = millis();
  patrolTimer = millis();
  lastMotionCheck = millis();
}

void loop() {
  server.handleClient();
  unsigned long now = millis();

  if (drive_active && now - drive_start > DRIVE_DURATION) {
    stopMotors();
    drive_active = false;
  }

  if (buzzer_active && now - buzzer_start > 2000) {
    digitalWrite(BUZZER_PIN, LOW);
    buzzer_active = false;
  }

  if (now - lastCommandTime > 10000) {
    if (now - patrolTimer > 5000) {
      if (patrolLeft) {
        turnLeft(); delay(300); stopMotors();
      } else {
        turnRight(); delay(300); stopMotors();
      }
      patrolLeft = !patrolLeft;
      patrolTimer = now;
    }

    if (now - lastMotionCheck > MOTION_INTERVAL) {
      auto frame = esp32cam::capture();
      if (frame != nullptr) {
        size_t currentSize = frame->size();
        if (lastFrameSize > 0) {
          size_t diff = abs((long)currentSize - (long)lastFrameSize);
          if (diff > 2500) {
            digitalWrite(BUZZER_PIN, HIGH);
            delay(500);
            digitalWrite(BUZZER_PIN, LOW);
            Serial.println("MOTION DETECTED");
          }
        }
        lastFrameSize = currentSize;
      }
      lastMotionCheck = now;
    }
  }
}

