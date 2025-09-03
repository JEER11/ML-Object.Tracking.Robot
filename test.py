import cv2
import numpy as np
import urllib.request
import requests
import time

# Set your ESP32-CAM IP
IP = '192.168.1.129'  # Update if your IP changes

CAM_URL = f'http://{IP}/cam-lo.jpg'
BUZZER_URL = f'http://{IP}/buzzer?action=on'
FLASH_URL = f'http://{IP}/flash'
MOVE_FORWARD_URL = f'http://{IP}/move?action=forward'
TURN_LEFT_URL = f'http://{IP}/move?action=left'
TURN_RIGHT_URL = f'http://{IP}/move?action=right'
STOP_URL = f'http://{IP}/move?action=stop'

# Timers
BUZZER_COOLDOWN = 3
WATCH_TIME = 5  # seconds to wait between nudge actions

last_buzzer_time = 0
watching = False
watch_start_time = 0

# YOLOv3 Setup
whT = 320
confThreshold = 0.5
nmsThreshold = 0.3

# Load class names
classesfile = 'coco.names'
classNames = []
with open(classesfile, 'rt') as f:
    classNames = f.read().rstrip('\n').split('\n')

# Load YOLO model
modelConfig = 'yolov3.cfg'
modelWeights = 'yolov3.weights'
net = cv2.dnn.readNetFromDarknet(modelConfig, modelWeights)
net.setPreferableBackend(cv2.dnn.DNN_BACKEND_OPENCV)
net.setPreferableTarget(cv2.dnn.DNN_TARGET_CPU)

# --- Helper ---
def send(url, label):
    try:
        requests.get(url, timeout=0.5)
        print(f"[✓] {label}")
    except Exception as e:
        print(f"[X] Failed: {label} — {e}")

# --- Detection + Movement ---
def findObject(outputs, im):
    global watching, watch_start_time, last_buzzer_time

    hT, wT, _ = im.shape
    bbox, classIds, confs = [], [], []
    found_bird = False

    for output in outputs:
        for det in output:
            scores = det[5:]
            classId = np.argmax(scores)
            confidence = scores[classId]
            if confidence > confThreshold:
                w, h = int(det[2]*wT), int(det[3]*hT)
                x, y = int((det[0]*wT)-w/2), int((det[1]*hT)-h/2)
                bbox.append([x, y, w, h])
                classIds.append(classId)
                confs.append(float(confidence))

    indices = cv2.dnn.NMSBoxes(bbox, confs, confThreshold, nmsThreshold)

    for i in indices:
        box = bbox[i]
        x, y, w, h = box
        label = classNames[classIds[i]]
        if label == 'bird':
            found_bird = True
            if time.time() - last_buzzer_time > BUZZER_COOLDOWN:
                send(BUZZER_URL, "Buzzer")
                send(FLASH_URL, "Flash")
                send(MOVE_FORWARD_URL, "Move Forward")
                time.sleep(5)
                send(STOP_URL, "Stop")
                last_buzzer_time = time.time()
            watching = False

        cv2.rectangle(im, (x, y), (x+w, y+h), (255, 0, 255), 2)
        cv2.putText(im, f'{label.upper()} {int(confs[i]*100)}%',
                    (x, y-10), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 0, 255), 2)

    # === SCANNING LEFT/RIGHT ===
    if not hasattr(findObject, "scanState"):
        findObject.scanState = "left"
        findObject.lastTurnTime = time.time()

    if not found_bird:
        now = time.time()
        if not watching:
            print("No bird — scanning position...")
            if findObject.scanState == "left":
                send(TURN_LEFT_URL, "Nudge Left")
                time.sleep(0.3)
                send(STOP_URL, "Stop")
                findObject.scanState = "right"
            else:
                send(TURN_RIGHT_URL, "Nudge Right")
                time.sleep(0.3)
                send(STOP_URL, "Stop")
                findObject.scanState = "left"

            findObject.lastTurnTime = now
            watching = True
        elif now - findObject.lastTurnTime > WATCH_TIME:
            watching = False

# --- Main Loop ---
while True:
    try:
        img_resp = urllib.request.urlopen(CAM_URL)
        imgnp = np.array(bytearray(img_resp.read()), dtype=np.uint8)
        im = cv2.imdecode(imgnp, -1)

        blob = cv2.dnn.blobFromImage(im, 1/255, (whT, whT), [0, 0, 0], 1, crop=False)
        net.setInput(blob)
        outputNames = [net.getLayerNames()[i - 1] for i in net.getUnconnectedOutLayers()]
        outputs = net.forward(outputNames)

        findObject(outputs, im)

        cv2.imshow('AI Bird Tracker', im)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
    except Exception as e:
        print("[!] Error:", e)

cv2.destroyAllWindows()

