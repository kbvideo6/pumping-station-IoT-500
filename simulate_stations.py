import time
import random
import requests
import json
import os
import getpass
import threading

# ── Configuration ─────────────────────────────────────────────
# Load from environment variables or .env file.
# Install python-dotenv for .env support: pip install python-dotenv
try:
    from dotenv import load_dotenv
    load_dotenv()
except ImportError:
    pass  # python-dotenv not installed, rely on system env vars

# Firebase config (defaults are for local emulator)
API_KEY = os.environ.get("FIREBASE_API_KEY", "")
DB_URL = os.environ.get("FIREBASE_DB_URL", "http://127.0.0.1:9000")
PROJECT_ID = os.environ.get("FIREBASE_PROJECT_ID", "pumping-station-iot")
NUM_STATIONS = int(os.environ.get("NUM_STATIONS", "50"))
USE_AUTH = os.environ.get("USE_AUTH", "false").lower() == "true"

def get_id_token(email, password):
    print("Authenticating...")
    auth_url = f"https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key={API_KEY}"
    payload = {
        "email": email,
        "password": password,
        "returnSecureToken": True
    }
    response = requests.post(auth_url, json=payload)
    if response.status_code == 200:
        print("Successfully authenticated!")
        return response.json().get("idToken")
    else:
        print("Authentication failed:", response.text)
        return None

def update_station(station_id, id_token):
    print(f"Starting simulation for {station_id}...")
    
    # Send initial config
    config_url = f"{DB_URL}/stations/{station_id}/config.json?ns={PROJECT_ID}-default-rtdb"
    if id_token:
        config_url += f"&auth={id_token}"
        
    config_data = {
        "stationName": f"Simulated Station {station_id.split('-')[-1]}",
        "highThreshold": 20.0,
        "lowThreshold": 5.0,
        "reportIntervalSec": 5,
        "configPollIntervalSec": 60,
        "pumpPowerKW": 5.5,
        "calibration": 1.0,
        "lat": 48.8566 + random.uniform(-0.1, 0.1),
        "lng": 2.3522 + random.uniform(-0.1, 0.1)
    }
    requests.put(config_url, json=config_data)
    
    import math
    uptime = 0
    energy = random.uniform(100, 500)
    
    # Randomize the phase so each station starts at a different point in the sine wave
    phase_offset = random.uniform(0, 2 * math.pi)
    
    # State for simulating occasional spikes
    spike_timer = random.randint(30, 300)
    
    while True:
        # Base realistic sine wave for current (e.g. daily usage curve)
        # Period = 600 seconds for simulation speed
        time_factor = (time.time() / 600.0 * 2 * math.pi) + phase_offset
        
        # Current fluctuates between 8A and 16A normally
        base_current = 12.0 + (4.0 * math.sin(time_factor))
        # Add a little high-frequency noise
        current = base_current + random.uniform(-0.5, 0.5)
        
        # Simulate an occasional alert spike (high current or low current)
        spike_timer -= 5
        if spike_timer <= 0:
            if random.random() > 0.5:
                current = 22.0 + random.uniform(0, 2) # High spike
            else:
                current = 2.0 + random.uniform(0, 1) # Low drop
            spike_timer = random.randint(60, 600) # Reset timer
        
        voltage = 230.0 + (2.0 * math.cos(time_factor)) + random.uniform(-1.0, 1.0)
        power = voltage * current / 1000.0
        energy += power * (5 / 3600.0) # 5 seconds of energy
        frequency = 50.0 + random.uniform(-0.1, 0.1)
        
        # Power factor drops when current is very low
        if current < 5.0:
            power_factor = random.uniform(0.70, 0.80)
        else:
            power_factor = random.uniform(0.88, 0.98)
        
        alert = False
        alert_type = None
        if current > 19.5:
            alert = True
            alert_type = "HIGH_CURRENT"
        elif current < 4.5:
            alert = True
            alert_type = "LOW_CURRENT"
            
        live_data = {
            "current": current,
            "voltage": voltage,
            "power": power,
            "energy": energy,
            "frequency": frequency,
            "powerFactor": power_factor,
            "alert": alert,
            "alertType": alert_type,
            "rssi": -70 + int(10 * math.sin(time_factor * 2)) + random.randint(-5, 5),
            "uptimeSeconds": uptime,
            "firmwareVersion": "v1.0.0-sim",
            "battVolts": 4.0 + (0.1 * math.sin(time_factor/2)),
            "battPercent": min(100.0, max(0.0, 80.0 + (20.0 * math.sin(time_factor/2)))),
            "timestamp": {".sv": "timestamp"}
        }
        
        status_data = {
            "online": True,
            "lastSeen": {".sv": "timestamp"}
        }
        
        # URLs
        live_url = f"{DB_URL}/stations/{station_id}/live.json?ns={PROJECT_ID}-default-rtdb"
        status_url = f"{DB_URL}/stations/{station_id}/status.json?ns={PROJECT_ID}-default-rtdb"
        
        if id_token:
            live_url += f"&auth={id_token}"
            status_url += f"&auth={id_token}"
            
        try:
            requests.put(live_url, json=live_data)
            requests.put(status_url, json=status_data)
        except Exception as e:
            pass # Ignore connection errors in simulation output to keep it clean
            
        uptime += 5
        time.sleep(5)

def main():
    print("=== Pumping Station Simulator ===")
    
    id_token = None
    if USE_AUTH:
        if not API_KEY:
            print("ERROR: FIREBASE_API_KEY is required when USE_AUTH=true. Set it in your .env file.")
            return
        email = input("Firebase Email: ")
        password = getpass.getpass("Firebase Password: ")
        id_token = get_id_token(email, password)
        if not id_token:
            print("Exiting due to auth failure.")
            return

    print(f"Starting simulation for {NUM_STATIONS} stations...")
    print(f"Target DB: {DB_URL}")
    
    threads = []
    for i in range(1, NUM_STATIONS + 1):
        station_id = f"STATION-{i:03d}"
        t = threading.Thread(target=update_station, args=(station_id, id_token), daemon=True)
        t.start()
        threads.append(t)
        time.sleep(0.1) # Stagger start times
        
    print("All stations started! Press Ctrl+C to stop.")
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("Simulation stopped.")

if __name__ == "__main__":
    main()
