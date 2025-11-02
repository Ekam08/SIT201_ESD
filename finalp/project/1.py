from flask import Flask, render_template, jsonify, request
import paho.mqtt.client as mqtt
import threading
import time
import csv, os
from datetime import datetime
import RPi.GPIO as GPIO

# ==== CONFIG ====
BROKER = "localhost"
TOPIC_RFID = "esp32/cart/rfid"
TOPIC_ALERT = "esp32/cart/alert"
SERVO_PIN = 17
SERVO_FREQ = 50
PRODUCTS_CSV = "products.csv"
SCAN_COOLDOWN = 2

# ---- SERVO ANGLE CONTROL (CHANGED) ----
# Typical SG90 range at 50Hz is ~2.5% (0Â°) to ~12.5% (180Â°).
# If your unit binds or under/overshoots, tweak MIN_DUTY / MAX_DUTY slightly.
MIN_DUTY = 2.5   # duty % at 0Â°
MAX_DUTY = 12.5  # duty % at 180Â°
ANGLE_CLOSE = 0      # closed flap
ANGLE_OPEN  = 90     # open flap target (your requirement)
HOLD_SECONDS = 10    # how long to remain open before closing
# ======================================

app = Flask(__name__)
basket = []
products = {}
last_scan = {}
lock = threading.Lock()

# ==== SERVO SETUP ====
GPIO.setmode(GPIO.BCM)
GPIO.setup(SERVO_PIN, GPIO.OUT)
servo = GPIO.PWM(SERVO_PIN, SERVO_FREQ)
servo.start(0)

# servo state + timer
servo_state = "closed"   # "closed" or "open"
_servo_lock = threading.Lock()
_hold_timer = None

# ---- ANGLE HELPERS (CHANGED) ----
def _angle_to_duty(angle):
    """Map 0..180Â° to MIN_DUTY..MAX_DUTY linearly."""
    angle = max(0, min(180, float(angle)))
    return MIN_DUTY + (MAX_DUTY - MIN_DUTY) * (angle / 180.0)

def _go_to_angle(angle, hold_time=0.5):
    """Move to a specific angle, then set duty to 0 to avoid jitter."""
    duty = _angle_to_duty(angle)
    servo.ChangeDutyCycle(duty)
    time.sleep(hold_time)
    servo.ChangeDutyCycle(0)
# ----------------------------------

def _cancel_hold_timer():
    global _hold_timer
    if _hold_timer is not None:
        try:
            _hold_timer.cancel()
        except Exception:
            pass
        _hold_timer = None

def open_servo():
    """Open flap to 90Â° immediately and start/reset hold timer. (CHANGED)"""
    global servo_state, _hold_timer
    with _servo_lock:
        _go_to_angle(ANGLE_OPEN)   # <-- precise 90Â° open
        servo_state = "open"
        _cancel_hold_timer()
        _hold_timer = threading.Timer(HOLD_SECONDS, close_servo)
        _hold_timer.daemon = True
        _hold_timer.start()
        print("Flap opened to 90Â° (timer started/reset)")

def close_servo():
    """Close flap back to 0Â° (called by timer or API). (CHANGED)"""
    global servo_state, _hold_timer
    with _servo_lock:
        _cancel_hold_timer()
        _go_to_angle(ANGLE_CLOSE)  # <-- back to 0Â° closed
        servo_state = "closed"
        print("Flap closed to 0Â°")

# set initial closed position at startup
_go_to_angle(ANGLE_CLOSE)

# ==== PRODUCT DATABASE ====
def ensure_products():
    if not os.path.exists(PRODUCTS_CSV):
        with open(PRODUCTS_CSV, "w", newline="") as f:
            writer = csv.writer(f)
            writer.writerow(["uid", "name", "price"])

def load_products():
    ensure_products()
    products.clear()
    with open(PRODUCTS_CSV, newline="") as f:
        for row in csv.DictReader(f):
            uid = row["uid"].strip().upper()
            if uid:
                products[uid] = (row["name"], float(row["price"]))
    print(f"Loaded {len(products)} products")

load_products()

# ==== MQTT ====
def on_connect(client, userdata, flags, rc):
    print("MQTT connected:", rc)
    client.subscribe(TOPIC_RFID)
    client.subscribe(TOPIC_ALERT)

def on_message(client, userdata, msg):
    payload = msg.payload.decode("utf-8").strip()
    print(msg.topic, payload)
    if msg.topic == TOPIC_RFID:
        if payload.upper().startswith("RFID:"):
            uid = payload.split(":", 1)[1].replace(" ", "").upper()
        else:
            uid = payload.replace(" ", "").upper()
        handle_rfid(uid)
    elif msg.topic == TOPIC_ALERT:
        handle_alert(payload)

def handle_rfid(uid):
    now = time.time()
    if now - last_scan.get(uid, 0) < SCAN_COOLDOWN:
        return
    last_scan[uid] = now
    with lock:
        if uid in products:
            name, price = products[uid]
            basket.append({
                "uid": uid,
                "name": name,
                "price": price,
                "time": datetime.now().strftime("%H:%M:%S")
            })
            print(f"âœ… Product added: {name} - â‚¹{price}")
            open_servo()  # opens to 90Â°, resets timer if already open
        else:
            print(f"âŒ Unauthorized card: {uid}")

def handle_alert(msg):
    if "Obstacle" in msg:
        print("âš ï¸ Obstacle detected (ignored for servo)")
    else:
        print(f"Alert: {msg}")

mqtt_client = mqtt.Client()
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message
mqtt_thread = threading.Thread(
    target=lambda: mqtt_client.connect(BROKER, 1883, 60) or mqtt_client.loop_forever(),
    daemon=True
)
mqtt_thread.start()

# ==== FLASK ROUTES ====
@app.route("/")
def index():
    total = sum(i["price"] for i in basket)
    return render_template("index.html", basket=basket, total=total)

@app.route("/api/basket")
def api_basket():
    total = sum(i["price"] for i in basket)
    return jsonify({"basket": basket, "total": total})

@app.route("/api/checkout", methods=["POST"])
def api_checkout():
    if not basket:
        return jsonify({"status": "empty"})
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    os.makedirs("bills", exist_ok=True)
    fname = f"bills/bill_{ts}.csv"
    with open(fname, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["name", "price", "uid", "time"])
        for i in basket:
            w.writerow([i["name"], i["price"], i["uid"], i["time"]])
        w.writerow([])
        w.writerow(["Total", sum(i["price"] for i in basket)])
    basket.clear()
    close_servo()
    print(f"âœ… Bill saved: {fname}")
    return jsonify({"status": "ok", "file": fname})

@app.route("/api/servo/<action>")
def api_servo(action):
    if action == "open":
        open_servo()
        return jsonify({"status": "open"})
    else:
        close_servo()
        return jsonify({"status": "close"})

# ==== MAIN RUN ====
if __name__ == "__main__":
    try:
        app.run(host="0.0.0.0", port=5000, debug=False)
    finally:
        try:
            _cancel_hold_timer()
        except Exception:
            pass
        servo.stop()
        GPIO.cleanup()