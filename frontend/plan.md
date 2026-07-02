# Frontend Implementation Plan — Web Dashboard

## Overview

A **single-page web application** hosted on Firebase Hosting. Uses **Firebase Auth (Google Sign-In)** for access control. Reads live data from **Firebase Realtime DB** and historical data from **Firestore**. Built with **vanilla HTML/CSS/JS** (no framework) to keep it simple, fast, and easy to hand over.

---

## Authentication & Access Control

### Google Sign-In Flow
```
1. User visits dashboard URL
2. If not authenticated → show Google Sign-In button
3. User clicks → Google OAuth popup
4. On success → check /users/{uid} in Firestore
   → If exists: load dashboard with user's role & assigned stations
   → If not exists: show "Access Denied — Contact Admin" message
5. Admin can add users via Station Management UI
```

### Role-Based UI

| Element | Admin | Viewer |
|---|---|---|
| View live data | ✅ | ✅ |
| View charts | ✅ | ✅ |
| View alert history | ✅ | ✅ |
| Edit thresholds | ✅ | ❌ (read-only) |
| Add/remove stations | ✅ | ❌ (hidden) |
| Manage users | ✅ | ❌ (hidden) |
| Provision device | ✅ | ❌ (hidden) |

---

## Pages & Components

### Page 1: Login Page (`/`)

**Layout:**
- Full-screen centered card
- Company logo placeholder (customer can replace)
- "Pumping Station Monitor" title
- Google Sign-In button (Firebase UI or custom styled)
- Footer with version number

**Behavior:**
- If already authenticated, redirect to `/dashboard`
- On auth failure, show error toast

---

### Page 2: Dashboard Overview (`/dashboard`)

**Layout:**
```
┌──────────────────────────────────────────────────────────────┐
│  HEADER: Logo | "Pumping Station Monitor" | User Menu ▼     │
├──────────────────────────────────────────────────────────────┤
│  SIDEBAR (collapsible):                                      │
│  ├── 📊 Overview          (active)                           │
│  ├── 🗺️ Station Map                                         │
│  ├── 🔔 Alert History                                        │
│  ├── ⚙️ Station Management  (admin only)                     │
│  └── 👥 User Management     (admin only)                     │
├──────────────────────────────────────────────────────────────┤
│  MAIN CONTENT:                                               │
│                                                              │
│  ┌─────────────────┐  ┌─────────────────┐  ┌──────────────┐ │
│  │ Total Stations  │  │ Online Now      │  │ Active Alerts │ │
│  │     12          │  │     11          │  │     2         │ │
│  └─────────────────┘  └─────────────────┘  └──────────────┘ │
│                                                              │
│  STATION CARDS (grid):                                       │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐       │
│  │ Station 001  │  │ Station 002  │  │ Station 003  │       │
│  │ ● Online     │  │ ● Online     │  │ ○ Offline    │       │
│  │ 12.4A        │  │ 8.7A         │  │ --           │       │
│  │ ✅ Normal    │  │ ⚠️ High     │  │ 🔴 Offline  │       │
│  │ [View →]     │  │ [View →]     │  │ [View →]     │       │
│  └──────────────┘  └──────────────┘  └──────────────┘       │
└──────────────────────────────────────────────────────────────┘
```

**Data Source:** Firebase RTDB `/stations/` — real-time listener on all stations

**Features:**
- Summary stats cards (total, online, alerts) — auto-update in real-time
- Station cards in a responsive grid
- Each card shows: name, online/offline indicator, current reading, alert status
- Click card → navigate to station detail
- Color-coded borders: green (normal), amber (warning), red (alert), gray (offline)
- Search/filter bar for stations

---

### Page 3: Station Detail (`/station/{stationId}`)

**Layout:**
```
┌──────────────────────────────────────────────────────────────┐
│  ← Back | Station: "Pumpstation Graz-Ost" | Status: ● Online│
├──────────────────────────────────────────────────────────────┤
│                                                              │
│  LIVE METRICS ROW:                                           │
│  ┌────────────┐ ┌────────────┐ ┌────────────┐ ┌───────────┐ │
│  │ Current    │ │ Signal     │ │ Uptime     │ │ Firmware  │ │
│  │ 12.4 A     │ │ -67 dBm   │ │ 24h 12m    │ │ v1.0.0    │ │
│  │ ✅ Normal  │ │ ████░ Good│ │            │ │           │ │
│  └────────────┘ └────────────┘ └────────────┘ └───────────┘ │
│                                                              │
│  LIVE CHART (last 30 minutes, auto-scrolling):               │
│  ┌──────────────────────────────────────────────────────────┐ │
│  │  Current (A)                                 ▬ High Thr │ │
│  │  18 ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─  │ │
│  │                                                          │ │
│  │     ╱╲    ╱╲    ╱╲                                       │ │
│  │  ──╱──╲──╱──╲──╱──╲─────────────────────                │ │
│  │                                                          │ │
│  │  2  ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─  │ │
│  │                                              ▬ Low Thr  │ │
│  └──────────────────────────────────────────────────────────┘ │
│                                                              │
│  TABS: [ Live ] [ History ] [ Config ] [ Alerts ]            │
│                                                              │
│  ═══════════════════════════════════════════════════════════  │
│                                                              │
│  TAB: Config (admin only editable)                           │
│  ┌──────────────────────────────────────────────────────────┐ │
│  │ High Current Threshold:  [  18.0  ] A                    │ │
│  │ Low Current Threshold:   [   2.0  ] A                    │ │
│  │ Report Interval:         [   30   ] sec                  │ │
│  │ Station Name:            [ Pumpstation Graz-Ost    ]     │ │
│  │ Pump Power:              [   1.5  ] kW                   │ │
│  │                                                          │ │
│  │                              [ Save Changes ]            │ │
│  └──────────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────────┘
```

**Live Chart:**
- Library: **Chart.js** (lightweight, no build step needed)
- X-axis: time (last 30 min, sliding window)
- Y-axis: current in Amps
- Horizontal reference lines for high/low thresholds
- New data points added in real-time via RTDB listener
- Buffer last 60 points in memory

**History Tab:**
- Date range picker (default: last 24h)
- Max range: 30 days
- Data from Firestore `/history/{stationId}/readings`
- Aggregated view for longer ranges (hourly averages for >3 days)
- Download CSV button

**Config Tab:**
- Editable form for thresholds and settings
- Writes directly to RTDB `/stations/{stationId}/config`
- Admin only — viewers see read-only values
- Save button with confirmation toast
- Input validation (min/max ranges, required fields)

**Alerts Tab:**
- Table of recent alerts from Firestore `/alerts/`
- Columns: timestamp, type, value, threshold, acknowledged
- Click to acknowledge (admin only)
- Filter by alert type

---

### Page 4: Station Map (`/map`)

**Layout:**
- Full-width map (Leaflet.js with OpenStreetMap tiles — free, no API key)
- Markers for each station, color-coded by status:
  - 🟢 Green: normal
  - 🟡 Amber: warning (near threshold)
  - 🔴 Red: active alert
  - ⚫ Gray: offline
- Click marker → popup with station summary + "View Details" link
- Station location stored in RTDB config (`lat`, `lng`)

> [!NOTE]
> For the pilot (1 station), the map is simple. As stations scale, clustering (Leaflet.markercluster) prevents visual clutter.

---

### Page 5: Alert History (`/alerts`)

**Layout:**
- Filterable table of all alerts across all stations
- Filters: station, alert type, date range, acknowledged/unacknowledged
- Columns: timestamp, station, type, current value, threshold, status
- Bulk acknowledge button (admin only)
- Pagination (25 per page)

---

### Page 6: Station Management (`/stations/manage`) — Admin Only

**Features:**
- Table of all stations with: ID, name, status, last seen, firmware version
- "Add Station" button → modal:
  - Enter: stationId, stationName, pumpPowerKW, lat/lng
  - Sets default thresholds
  - Triggers device provisioning (calls Cloud Function to generate custom token)
  - Displays provisioning token for flashing
- "Edit" button per station → inline edit name, location
- "Remove" button → confirmation dialog → soft delete (marks inactive)

---

### Page 7: User Management (`/users/manage`) — Admin Only

**Features:**
- Table of users: email, role, assigned stations, last login
- "Invite User" button → modal:
  - Enter email
  - Select role (admin/viewer)
  - Assign stations (multi-select)
  - Creates entry in Firestore `/users/` (user sees access on next login)
- Edit role / station assignments inline
- Remove user button

---

## Design System

### Color Palette

```css
:root {
    /* Primary - Deep industrial blue */
    --primary-900: #0f172a;
    --primary-800: #1e293b;
    --primary-700: #334155;
    --primary-600: #475569;
    --primary-500: #64748b;
    
    /* Accent - Electric cyan */
    --accent-500: #06b6d4;
    --accent-400: #22d3ee;
    --accent-300: #67e8f9;
    
    /* Status colors */
    --success: #22c55e;
    --warning: #f59e0b;
    --danger: #ef4444;
    --offline: #6b7280;
    
    /* Background - Dark mode default */
    --bg-primary: #0f172a;
    --bg-card: #1e293b;
    --bg-elevated: #334155;
    
    /* Text */
    --text-primary: #f1f5f9;
    --text-secondary: #94a3b8;
    --text-muted: #64748b;
    
    /* Borders */
    --border: #334155;
    --border-hover: #475569;
}
```

### Typography
- **Font**: `Inter` from Google Fonts (clean, modern, excellent readability)
- **Headings**: 600 weight
- **Body**: 400 weight
- **Monospace** (for readings): `JetBrains Mono` or `Fira Code`

### Component Styles
- **Cards**: Dark glassmorphism (`backdrop-filter: blur(10px)`, subtle border, shadow)
- **Buttons**: Rounded corners (8px), subtle gradient, hover glow effect
- **Inputs**: Dark background, accent-colored focus ring
- **Charts**: Dark background, accent-colored data line, semi-transparent fill
- **Transitions**: All interactive elements have 200ms ease transition
- **Animations**: Fade-in on page load, pulse on live data update, slide-in for sidebars

---

## Technical Implementation

### Firebase SDK Usage

```html
<!-- Firebase SDKs (CDN, no build step) -->
<script src="https://www.gstatic.com/firebasejs/10.x/firebase-app-compat.js"></script>
<script src="https://www.gstatic.com/firebasejs/10.x/firebase-auth-compat.js"></script>
<script src="https://www.gstatic.com/firebasejs/10.x/firebase-database-compat.js"></script>
<script src="https://www.gstatic.com/firebasejs/10.x/firebase-firestore-compat.js"></script>
```

### Real-Time Data Binding

```javascript
// Listen to all stations live data
const stationsRef = firebase.database().ref('stations');
stationsRef.on('value', (snapshot) => {
    const stations = snapshot.val();
    updateDashboard(stations);
});

// Listen to single station
const stationRef = firebase.database().ref(`stations/${stationId}/live`);
stationRef.on('value', (snapshot) => {
    const live = snapshot.val();
    updateLiveChart(live);
    updateMetricCards(live);
});
```

### Firestore History Query

```javascript
// Query last 24h of readings
const yesterday = new Date(Date.now() - 86400000);
const readings = await firebase.firestore()
    .collection('history')
    .doc(stationId)
    .collection('readings')
    .where('timestamp', '>=', yesterday)
    .orderBy('timestamp', 'asc')
    .get();
```

### Routing (Hash-based SPA)

```javascript
// Simple hash router — no build tools needed
window.addEventListener('hashchange', () => {
    const route = window.location.hash.slice(1);
    switch (true) {
        case route === '' || route === '/':
            showLogin(); break;
        case route === '/dashboard':
            showDashboard(); break;
        case route.startsWith('/station/'):
            showStationDetail(route.split('/')[2]); break;
        case route === '/map':
            showMap(); break;
        case route === '/alerts':
            showAlerts(); break;
        case route === '/stations/manage':
            showStationManagement(); break;
        case route === '/users/manage':
            showUserManagement(); break;
    }
});
```

---

## File Structure

```
frontend/
├── plan.md                  ← This file
├── index.html               ← Single HTML entry point
├── css/
│   ├── variables.css        ← Design tokens (colors, spacing, fonts)
│   ├── base.css             ← Reset, typography, global styles
│   ├── layout.css           ← Header, sidebar, grid system
│   ├── components.css       ← Cards, buttons, inputs, tables, modals
│   └── pages.css            ← Page-specific overrides
├── js/
│   ├── app.js               ← Firebase init, auth state, router
│   ├── auth.js              ← Google Sign-In, role checking
│   ├── dashboard.js         ← Overview page logic
│   ├── station-detail.js    ← Single station view, live chart
│   ├── history.js           ← Firestore history queries, CSV export
│   ├── map.js               ← Leaflet map integration
│   ├── alerts.js            ← Alert history table
│   ├── station-mgmt.js      ← Add/edit/remove stations (admin)
│   ├── user-mgmt.js         ← User management (admin)
│   └── utils.js             ← Formatters, helpers, toast notifications
├── assets/
│   └── logo.png             ← Customer logo (placeholder)
└── firebase.json            ← Firebase Hosting config
```

---

## External Libraries (CDN, no npm)

| Library | Purpose | Size |
|---|---|---|
| Firebase JS SDK | Auth, RTDB, Firestore | ~50KB gzipped |
| Chart.js 4.x | Live and historical charts | ~65KB gzipped |
| Leaflet.js | Station map | ~40KB gzipped |
| Google Fonts (Inter) | Typography | ~20KB |

**Total page weight: ~175KB gzipped** — loads in <2s on 4G

---

## Responsive Breakpoints

| Breakpoint | Layout |
|---|---|
| ≥1200px | Sidebar visible, 3-column card grid |
| 768–1199px | Sidebar collapsed (hamburger), 2-column grid |
| <768px | No sidebar (bottom nav), 1-column stack |

---

## Testing Plan

| Test | Method |
|---|---|
| Google Sign-In works | Test with 2 Google accounts |
| Unauthorized user blocked | Sign in with non-whitelisted account |
| Admin vs viewer permissions | Verify UI elements show/hide by role |
| Live data updates in <2s | Watch dashboard while ESP32 sends data |
| Historical chart loads correctly | Query Firestore, verify data points |
| Threshold edit saves to RTDB | Change value, check Firebase console |
| Map markers render correctly | Add stations with lat/lng, verify placement |
| Responsive layout works | Test on mobile, tablet, desktop viewports |
| Offline station detection | Stop ESP32, verify gray/offline status |
| CSV export works | Download and open in Excel |
