import time
import random
import requests
import math
import sys
import re

# ── Configuration Loader ──────────────────────────────────────
# Attempt to read config.h directly to keep simulation aligned with firmware config.
def load_config_from_header(header_path):
    config = {
        "FIREBASE_PROJECT_ID": "argus360-c0496",
        "FIREBASE_API_KEY": "AIzaSyBtGpOwNCicAIl5FUh07nXb39Dh-XPdabE",
        "FIREBASE_DB_URL": "https://argus360-c0496-default-rtdb.europe-west1.firebasedatabase.app",
        "DEFAULT_STATION_ID": "STATION_001",
        "DEFAULT_CUSTOM_TOKEN": ""
    }
    try:
        with open(header_path, 'r', encoding='utf-8') as f:
            content = f.read()
            # Clean up line continuations '\'
            content = content.replace('\\\n', '').replace('\\\r\n', '')
            
            project_id = re.search(r'#define\s+FIREBASE_PROJECT_ID\s+"([^"]+)"', content)
            api_key = re.search(r'#define\s+FIREBASE_API_KEY\s+"([^"]+)"', content)
            db_url = re.search(r'#define\s+FIREBASE_DB_URL\s+"([^"]+)"', content)
            station_id = re.search(r'#define\s+DEFAULT_STATION_ID\s+"([^"]+)"', content)
            custom_token = re.search(r'#define\s+DEFAULT_CUSTOM_TOKEN\s+"([^"]+)"', content)
            
            if project_id: config["FIREBASE_PROJECT_ID"] = project_id.group(1)
            if api_key: config["FIREBASE_API_KEY"] = api_key.group(1)
            if db_url: config["FIREBASE_DB_URL"] = db_url.group(1)
            if station_id: config["DEFAULT_STATION_ID"] = station_id.group(1)
            if custom_token: config["DEFAULT_CUSTOM_TOKEN"] = custom_token.group(1)
    except Exception as e:
        print(f"[*] Warning: Could not parse config.h ({e}). Using hardcoded fallbacks.")
    return config

# Load settings
CONFIG = load_config_from_header("firmware/components/config/include/config.h")

PROJECT_ID = CONFIG["FIREBASE_PROJECT_ID"]
API_KEY = CONFIG["FIREBASE_API_KEY"]
DB_URL = CONFIG["FIREBASE_DB_URL"]
STATION_ID = CONFIG["DEFAULT_STATION_ID"]
CUSTOM_TOKEN = CONFIG["DEFAULT_CUSTOM_TOKEN"]

# ── Authentication Helper ────────────────────────────────────
class FirebaseAuthSimulator:
    def __init__(self):
        self.id_token = None
        self.refresh_token = None
        self.expiry_time = 0

    def authenticate(self):
        print("[*] Exchanging device token for Firebase custom token...")
        exchange_url = f"https://europe-west1-{PROJECT_ID}.cloudfunctions.net/getDeviceCustomToken"
        payload = {
            "stationId": STATION_ID,
            "deviceToken": CUSTOM_TOKEN
        }
        try:
            res = requests.post(exchange_url, json=payload, timeout=10)
            if res.status_code != 200:
                print(f"[-] Token exchange failed: {res.status_code} - {res.text}")
                return False
            
            firebase_custom_token = res.json().get("customToken")
            if not firebase_custom_token:
                print("[-] Custom token missing from cloud function response.")
                return False
            
            print("[*] Signing in with Custom Token...")
            sign_in_url = f"https://identitytoolkit.googleapis.com/v1/accounts:signInWithCustomToken?key={API_KEY}"
            sign_in_payload = {
                "token": firebase_custom_token,
                "returnSecureToken": True
            }
            auth_res = requests.post(sign_in_url, json=sign_in_payload, timeout=10)
            if auth_res.status_code != 200:
                print(f"[-] Custom token sign-in failed: {auth_res.status_code} - {auth_res.text}")
                return False
            
            auth_data = auth_res.json()
            self.id_token = auth_data.get("idToken")
            self.refresh_token = auth_data.get("refreshToken")
            self.expiry_time = time.time() + int(auth_data.get("expiresIn", 3600))
            print("[+] Authentication successful!")
            return True
        except Exception as e:
            print(f"[-] Auth Exception: {e}")
            return False

    def get_token(self):
        if not self.id_token or time.time() >= self.expiry_time - 60:
            if self.refresh_token:
                self.refresh_token_call()
            else:
                self.authenticate()
        return self.id_token

    def refresh_token_call(self):
        print("[*] Refreshing ID token...")
        url = f"https://securetoken.googleapis.com/v1/token?key={API_KEY}"
        payload = {
            "grant_type": "refresh_token",
            "refresh_token": self.refresh_token
        }
        try:
            res = requests.post(url, data=payload, timeout=10)
            if res.status_code == 200:
                data = res.json()
                self.id_token = data.get("id_token")
                self.refresh_token = data.get("refresh_token")
                self.expiry_time = time.time() + int(data.get("expires_in", 3600))
                print("[+] Token refreshed successfully.")
            else:
                print(f"[-] Token refresh failed: {res.status_code}. Re-authenticating...")
                self.authenticate()
        except Exception as e:
            print(f"[-] Refresh Exception: {e}. Re-authenticating...")
            self.authenticate()

# ── Station Simulation ───────────────────────────────────────
def main():
    print("=" * 60)
    print(f"          Pumping Station Simulator - {STATION_ID}")
    print("=" * 60)
    print(f"[*] Project ID: {PROJECT_ID}")
    print(f"[*] Target DB:  {DB_URL}")
    
    if not CUSTOM_TOKEN:
        print("[-] ERROR: CUSTOM_TOKEN is empty. Please configure it in config.h first.")
        sys.exit(1)

    auth = FirebaseAuthSimulator()
    if not auth.authenticate():
        print("[-] Initial authentication failed. Exiting.")
        sys.exit(1)

    uptime = 0
    energy = random.uniform(100.0, 500.0)
    phase_offset = random.uniform(0, 2 * math.pi)

    print("\n[+] Starting telemetry loop (Press Ctrl+C to exit)...")
    
    while True:
        try:
            id_token = auth.get_token()
            if not id_token:
                print("[-] No valid ID Token. Retrying in 5s...")
                time.sleep(5)
                continue

            # Simulate sine wave base telemetry
            time_factor = (time.time() / 300.0 * 2 * math.pi) + phase_offset
            current = 10.0 + (5.0 * math.sin(time_factor)) + random.uniform(-0.3, 0.3)
            current = max(0.0, current)

            voltage = 230.0 + (3.0 * math.cos(time_factor)) + random.uniform(-0.5, 0.5)
            power = (voltage * current * 0.92) / 1000.0  # Apparent power with power factor
            energy += power * (10.0 / 3600.0)  # Energy over 10 second step
            frequency = 50.0 + random.uniform(-0.05, 0.05)
            power_factor = 0.88 + random.uniform(-0.02, 0.04)
            power_factor = min(1.0, max(0.0, power_factor))

            alert = False
            alert_type = None
            if current > 13.5:
                alert = True
                alert_type = "HIGH_CURRENT"
            elif current < 2.0:
                alert = True
                alert_type = "LOW_CURRENT"

            # Prepare telemetry payload
            live_payload = {
                "current": round(current, 2),
                "voltage": round(voltage, 1),
                "power": round(power, 3),
                "energy": round(energy, 4),
                "frequency": round(frequency, 2),
                "powerFactor": round(power_factor, 2),
                "alert": alert,
                "alertType": alert_type,
                "rssi": random.randint(-85, -60),
                "uptimeSeconds": uptime,
                "firmwareVersion": "v2.0.0-sim",
                "battVolts": round(4.1 - (0.3 * (uptime % 3600)/3600.0), 2),
                "battPercent": int(100 - (15 * (uptime % 3600)/3600.0)),
                "timestamp": {".sv": "timestamp"}
            }

            status_payload = {
                "online": True,
                "lastSeen": {".sv": "timestamp"}
            }

            # Send Telemetry & Status to Realtime Database
            live_url = f"{DB_URL}/stations/{STATION_ID}/live.json?auth={id_token}"
            status_url = f"{DB_URL}/stations/{STATION_ID}/status.json?auth={id_token}"

            live_res = requests.put(live_url, json=live_payload, timeout=5)
            status_res = requests.put(status_url, json=status_payload, timeout=5)

            if live_res.status_code == 200 and status_res.status_code == 200:
                print(f"[+] Reported: Current={round(current,2)}A, Volts={round(voltage,1)}V, Alert={alert} ({alert_type or 'None'})")
            else:
                print(f"[-] Telemetry report failed: Live {live_res.status_code}, Status {status_res.status_code}")

            uptime += 10
            time.sleep(10)

        except KeyboardInterrupt:
            print("\n[*] Simulator stopped by user.")
            break
        except Exception as e:
            print(f"[-] Telemetry loop error: {e}")
            time.sleep(5)

if __name__ == "__main__":
    main()
