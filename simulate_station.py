import time
import random
import requests
import math
import sys
import argparse

# ── Configuration Loader ──────────────────────────────────────
def get_args():
    parser = argparse.ArgumentParser(description="Pumping Station Hardware Simulator")
    parser.add_argument("--station", required=True, help="Station ID (e.g. STATION_002)")
    parser.add_argument("--token", required=True, help="Custom Device Token from Firebase")
    parser.add_argument("--project", default="argus360-c0496", help="Firebase Project ID")
    parser.add_argument("--apikey", required=True, help="Firebase Web API Key")
    parser.add_argument("--dburl", default="https://argus360-c0496-default-rtdb.europe-west1.firebasedatabase.app", help="Firebase DB URL")
    args = parser.parse_args()
    args.station = args.station.strip().upper()
    return args

# ── Authentication Helper ────────────────────────────────────
class FirebaseAuthSimulator:
    def __init__(self, project_id, api_key, station_id, custom_token):
        self.project_id = project_id
        self.api_key = api_key
        self.station_id = station_id
        self.custom_token = custom_token
        self.id_token = None
        self.refresh_token = None
        self.expiry_time = 0

    def authenticate(self):
        print(f"[MODEM] AT+HTTPPARA=\"URL\",\"https://europe-west1-{self.project_id}.cloudfunctions.net/getDeviceCustomToken\"")
        print("[MODEM] AT+HTTPACTION=1 (POST)")
        exchange_url = f"https://europe-west1-{self.project_id}.cloudfunctions.net/getDeviceCustomToken"
        payload = {
            "stationId": self.station_id,
            "deviceToken": self.custom_token
        }
        try:
            res = requests.post(exchange_url, json=payload, timeout=10)
            if res.status_code != 200:
                print(f"[ERROR] Token exchange failed: {res.status_code} - {res.text}")
                return False
            
            firebase_custom_token = res.json().get("customToken")
            if not firebase_custom_token:
                print("[ERROR] Custom token missing from cloud function response.")
                return False
            
            print("[AUTH] Received Firebase Custom Token. Exchanging for ID Token...")
            sign_in_url = f"https://identitytoolkit.googleapis.com/v1/accounts:signInWithCustomToken?key={self.api_key}"
            sign_in_payload = {
                "token": firebase_custom_token,
                "returnSecureToken": True
            }
            auth_res = requests.post(sign_in_url, json=sign_in_payload, timeout=10)
            if auth_res.status_code != 200:
                print(f"[ERROR] Custom token sign-in failed: {auth_res.status_code} - {auth_res.text}")
                return False
            
            auth_data = auth_res.json()
            self.id_token = auth_data.get("idToken")
            self.refresh_token = auth_data.get("refreshToken")
            self.expiry_time = time.time() + int(auth_data.get("expiresIn", 3600))
            print("[AUTH] [+] Authentication successful. ID Token acquired.")
            return True
        except Exception as e:
            print(f"[ERROR] Auth Exception: {e}")
            return False

    def get_token(self):
        if not self.id_token or time.time() >= self.expiry_time - 60:
            if self.refresh_token:
                self.refresh_token_call()
            else:
                self.authenticate()
        return self.id_token

    def refresh_token_call(self):
        print("[AUTH] Refreshing ID token...")
        url = f"https://securetoken.googleapis.com/v1/token?key={self.api_key}"
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
                print("[AUTH] [+] Token refreshed successfully.")
            else:
                print(f"[ERROR] Token refresh failed: {res.status_code}. Re-authenticating...")
                self.authenticate()
        except Exception as e:
            print(f"[ERROR] Refresh Exception: {e}. Re-authenticating...")
            self.authenticate()

# ── Station Simulation ───────────────────────────────────────
def main():
    args = get_args()
    
    print("============================================================")
    print("      ESP32-S3 A7670E Pumping Station Hardware Simulator")
    print(f"      Firmware v2.0.0-sim | Station: {args.station}")
    print("============================================================")
    print("[SYSTEM] Booting...")
    time.sleep(1)
    print("[SYSTEM] Initializing Watchdog (timeout 60s)")
    time.sleep(0.5)
    print("[SYSTEM] Mounting NVS Storage...")
    time.sleep(0.5)
    
    print("[MODEM] Powering on A7670E LTE Modem...")
    time.sleep(1.5)
    print("[MODEM] AT OK")
    print("[MODEM] AT+CPIN? -> READY")
    time.sleep(1)
    print("[MODEM] Waiting for Network Registration...")
    print("[MODEM] AT+CGREG? -> 0,1 (Registered, Home Network)")
    time.sleep(0.5)
    print("[MODEM] AT+CGACT=1,1 -> OK (PDP Context Active)")
    
    auth = FirebaseAuthSimulator(args.project, args.apikey, args.station, args.token)
    if not auth.authenticate():
        print("[SYSTEM] [-] Initial authentication failed. Halting.")
        sys.exit(1)

    print("[PZEM] Initializing PZEM-004T AC Energy Monitor on UART1...")
    time.sleep(0.5)
    print("[MAX17048] Initializing LiPo Fuel Gauge on I2C...")
    time.sleep(0.5)

    uptime = 0
    energy = 0.0  # Start at 0 kWh like a real fresh PZEM sensor
    phase_offset = random.uniform(0, 2 * math.pi)

    print("\n[SYSTEM] Entering main telemetry loop (10s interval)...")
    
    while True:
        try:
            id_token = auth.get_token()
            if not id_token:
                print("[SYSTEM] [-] No valid ID Token. Retrying in 5s...")
                time.sleep(5)
                continue

            time_factor = (time.time() / 150.0 * 2 * math.pi) + phase_offset
            base_current = 0.4 + (0.1 * math.sin(time_factor)) 
            current = base_current + random.uniform(-0.05, 0.05)
            
            alert = False
            alert_type = None
            if random.random() > 0.9:
                if random.random() > 0.5:
                    current = random.uniform(0.85, 0.95)
                    alert = True
                    alert_type = "HIGH_CURRENT"
                else:
                    current = random.uniform(0.01, 0.1)
                    alert = True
                    alert_type = "LOW_CURRENT"

            current = max(0.0, min(current, 0.99))
            
            voltage = 230.0 + (3.0 * math.cos(time_factor)) + random.uniform(-0.5, 0.5)
            power = (voltage * current * 0.92) / 1000.0
            energy += power * (10.0 / 3600.0)
            frequency = 50.0 + random.uniform(-0.05, 0.05)
            power_factor = 0.88 + random.uniform(-0.02, 0.04)

            print(f"[PZEM] READ -> V:{voltage:.1f}V  I:{current:.3f}A  P:{power*1000:.1f}W  E:{energy:.3f}kWh  PF:{power_factor:.2f}")

            live_payload = {
                "current": round(current, 3),
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

            live_url = f"{args.dburl}/stations/{args.station}/live.json?auth={id_token}"
            status_url = f"{args.dburl}/stations/{args.station}/status.json?auth={id_token}"

            print("[FIREBASE] PUT /stations/live.json ...")
            live_res = requests.put(live_url, json=live_payload, timeout=5)
            print(f"[FIREBASE] PUT /stations/status.json ...")
            status_res = requests.put(status_url, json=status_payload, timeout=5)

            if live_res.status_code == 200 and status_res.status_code == 200:
                print(f"[SYSTEM] [+] Telemetry synced successfully. Alert: {alert} ({alert_type or 'None'})")
            else:
                print(f"[SYSTEM] [-] Telemetry report failed: Live {live_res.status_code} ({live_res.text}), Status {status_res.status_code} ({status_res.text})")

            uptime += 10
            time.sleep(10)

        except KeyboardInterrupt:
            print("\n[SYSTEM] [*] Halting simulator.")
            break
        except Exception as e:
            print(f"[SYSTEM] [-] Telemetry loop error: {e}")
            time.sleep(5)

if __name__ == "__main__":
    main()
