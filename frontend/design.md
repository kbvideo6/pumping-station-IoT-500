# Frontend Design Specification — Pumping Station Monitor

## Design Philosophy

**Industrial Precision meets Modern SaaS.**
The dashboard is used by maintenance engineers, not developers. Every element must communicate status at a glance — clarity over cleverness. The visual language draws from industrial HMI panels (high-contrast, status-coded) but executed with the polish of a modern SaaS product (glassmorphism, smooth transitions, dark mode default).

Key principles:
- **Status in 0.5 seconds** — Color alone must communicate normal/warning/alert/offline
- **Dark mode by default** — Engineers often check dashboards in dim server rooms or outdoors on bright screens; dark reduces eye strain and saves screen burn-in
- **No clutter** — Every element on screen earns its place
- **Alive, not static** — Live data should feel live (pulsing, smooth updates)

---

## Color System

### Base Palette (CSS Custom Properties)

```css
:root {
    /* ── Backgrounds ─────────────────────────────── */
    --bg-base:        #080e1a;   /* Deepest layer — page background        */
    --bg-surface:     #0f1829;   /* Cards, sidebars                         */
    --bg-elevated:    #182236;   /* Dropdowns, modals, hover states         */
    --bg-input:       #1e2d45;   /* Form inputs, code blocks                */
    --bg-overlay:     rgba(8, 14, 26, 0.85); /* Modal backdrops           */

    /* ── Borders ─────────────────────────────────── */
    --border-subtle:  rgba(255, 255, 255, 0.06); /* Default card borders  */
    --border-default: rgba(255, 255, 255, 0.12); /* Inputs, dividers      */
    --border-hover:   rgba(255, 255, 255, 0.22); /* Hover states          */
    --border-focus:   #06b6d4;                   /* Focused inputs        */

    /* ── Primary Accent — Electric Cyan ──────────── */
    --accent-900: #083344;
    --accent-700: #0e7490;
    --accent-500: #06b6d4;   /* Primary CTA, links, active nav          */
    --accent-400: #22d3ee;   /* Hover on accent elements                */
    --accent-300: #67e8f9;   /* Subtle highlights, chart fill top        */
    --accent-glow: rgba(6, 182, 212, 0.25); /* Shadow/glow on accent    */

    /* ── Status: Normal / Online ─────────────────── */
    --status-ok:         #22c55e;
    --status-ok-dim:     rgba(34, 197, 94, 0.15);
    --status-ok-border:  rgba(34, 197, 94, 0.35);

    /* ── Status: Warning (approaching threshold) ─── */
    --status-warn:        #f59e0b;
    --status-warn-dim:    rgba(245, 158, 11, 0.15);
    --status-warn-border: rgba(245, 158, 11, 0.35);

    /* ── Status: Alert / High-Low Current ────────── */
    --status-alert:        #ef4444;
    --status-alert-dim:    rgba(239, 68, 68, 0.15);
    --status-alert-border: rgba(239, 68, 68, 0.35);

    /* ── Status: Offline ─────────────────────────── */
    --status-offline:        #6b7280;
    --status-offline-dim:    rgba(107, 114, 128, 0.15);
    --status-offline-border: rgba(107, 114, 128, 0.30);

    /* ── Text ────────────────────────────────────── */
    --text-primary:   #f0f6ff;   /* Headings, key values                  */
    --text-secondary: #94a3b8;   /* Labels, descriptions                  */
    --text-muted:     #4b5e77;   /* Disabled, placeholders                */
    --text-inverse:   #080e1a;   /* Text on light/accent backgrounds      */

    /* ── Gradients ───────────────────────────────── */
    --gradient-header:  linear-gradient(135deg, #0f1829 0%, #0d1f38 100%);
    --gradient-card:    linear-gradient(145deg, #0f1829 0%, #0e1624 100%);
    --gradient-accent:  linear-gradient(135deg, #0891b2 0%, #06b6d4 100%);
    --gradient-alert:   linear-gradient(135deg, #dc2626 0%, #ef4444 100%);
    --gradient-hero:    linear-gradient(180deg, rgba(6,182,212,0.08) 0%, transparent 60%);
}
```

### Status Color Usage Map

| Context | Normal | Warning | Alert | Offline |
|---|---|---|---|---|
| Card border | `--status-ok-border` | `--status-warn-border` | `--status-alert-border` | `--status-offline-border` |
| Card background tint | `--status-ok-dim` | `--status-warn-dim` | `--status-alert-dim` | `--status-offline-dim` |
| Badge text | `--status-ok` | `--status-warn` | `--status-alert` | `--status-offline` |
| LED dot | `--status-ok` (pulse) | `--status-warn` | `--status-alert` (pulse fast) | `--status-offline` (no pulse) |
| Chart line | `--status-ok` → `--accent-500` | `--status-warn` | `--status-alert` | — |
| Map marker | 🟢 | 🟡 | 🔴 | ⚫ |

---

## Typography

```css
/* Google Fonts import */
@import url('https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&family=JetBrains+Mono:wght@400;500&display=swap');

:root {
    --font-sans:  'Inter', system-ui, -apple-system, sans-serif;
    --font-mono:  'JetBrains Mono', 'Fira Code', monospace;

    /* Scale — Major Third (1.250) */
    --text-xs:   0.64rem;   /* 10.24px — badges, meta                 */
    --text-sm:   0.80rem;   /* 12.80px — table data, tooltips         */
    --text-base: 1.00rem;   /* 16.00px — body copy, labels            */
    --text-md:   1.25rem;   /* 20.00px — sub-headings                 */
    --text-lg:   1.563rem;  /* 25.00px — section titles               */
    --text-xl:   1.953rem;  /* 31.25px — page headings                */
    --text-2xl:  2.441rem;  /* 39.06px — hero stat numbers            */
    --text-3xl:  3.052rem;  /* 48.83px — single big metric display    */

    /* Weights */
    --fw-light:   300;
    --fw-regular: 400;
    --fw-medium:  500;
    --fw-semibold: 600;
    --fw-bold:    700;

    /* Line heights */
    --lh-tight:  1.2;
    --lh-snug:   1.35;
    --lh-normal: 1.5;
    --lh-relaxed:1.7;
}
```

### Type Roles

| Role | Font | Size | Weight | Usage |
|---|---|---|---|---|
| Page Title | Inter | `--text-xl` | 700 | `<h1>` per page |
| Section Title | Inter | `--text-lg` | 600 | Card headers, tab labels |
| Sub-heading | Inter | `--text-md` | 600 | Group labels |
| Body | Inter | `--text-base` | 400 | Descriptions, copy |
| Label | Inter | `--text-sm` | 500 | Form labels, table headers |
| Caption | Inter | `--text-xs` | 400 | Meta info, timestamps |
| **Big Metric** | Inter | `--text-3xl` | 700 | Single station current display |
| **Stat Number** | Inter | `--text-2xl` | 700 | Dashboard summary cards |
| **Monospace** | JetBrains Mono | `--text-base` | 400 | Current readings in tables, IDs |

---

## Spacing System

```css
:root {
    /* Base unit: 4px */
    --space-1:  0.25rem;   /*  4px */
    --space-2:  0.50rem;   /*  8px */
    --space-3:  0.75rem;   /* 12px */
    --space-4:  1.00rem;   /* 16px */
    --space-5:  1.25rem;   /* 20px */
    --space-6:  1.50rem;   /* 24px */
    --space-8:  2.00rem;   /* 32px */
    --space-10: 2.50rem;   /* 40px */
    --space-12: 3.00rem;   /* 48px */
    --space-16: 4.00rem;   /* 64px */
    --space-20: 5.00rem;   /* 80px */
}
```

---

## Border Radius

```css
:root {
    --radius-sm:   4px;    /* Badges, tags, small pills           */
    --radius-md:   8px;    /* Buttons, inputs                     */
    --radius-lg:   12px;   /* Cards, modals                       */
    --radius-xl:   16px;   /* Large panels, drawers               */
    --radius-2xl:  24px;   /* Hero cards, featured sections       */
    --radius-full: 9999px; /* Pills, LED dots, avatar circles     */
}
```

---

## Elevation (Shadows)

```css
:root {
    --shadow-sm:  0 1px 3px rgba(0,0,0,0.4), 0 1px 2px rgba(0,0,0,0.3);
    --shadow-md:  0 4px 12px rgba(0,0,0,0.5), 0 2px 6px rgba(0,0,0,0.3);
    --shadow-lg:  0 8px 24px rgba(0,0,0,0.6), 0 4px 8px rgba(0,0,0,0.3);
    --shadow-xl:  0 20px 48px rgba(0,0,0,0.7);
    --shadow-glow-accent: 0 0 20px rgba(6,182,212,0.3), 0 0 40px rgba(6,182,212,0.1);
    --shadow-glow-alert:  0 0 20px rgba(239,68,68,0.4), 0 0 40px rgba(239,68,68,0.15);
    --shadow-glow-ok:     0 0 16px rgba(34,197,94,0.3);
}
```

---

## Component Specifications

### Card (`.card`)

```
┌─────────────────────────────────────────────┐  ← border: 1px solid --border-subtle
│                                             │  ← bg: --bg-surface
│   Card content                              │  ← padding: --space-6
│                                             │  ← border-radius: --radius-lg
└─────────────────────────────────────────────┘  ← shadow: --shadow-md
```

```css
.card {
    background: var(--bg-surface);
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius-lg);
    padding: var(--space-6);
    box-shadow: var(--shadow-md);
    transition: border-color 200ms ease, box-shadow 200ms ease;
}

.card:hover {
    border-color: var(--border-hover);
    box-shadow: var(--shadow-lg);
}
```

**Status Card Variants:**
```css
.card--ok      { border-color: var(--status-ok-border);
                 background: linear-gradient(145deg, var(--bg-surface), var(--status-ok-dim)); }
.card--warn    { border-color: var(--status-warn-border);
                 background: linear-gradient(145deg, var(--bg-surface), var(--status-warn-dim)); }
.card--alert   { border-color: var(--status-alert-border);
                 background: linear-gradient(145deg, var(--bg-surface), var(--status-alert-dim));
                 box-shadow: var(--shadow-glow-alert); }
.card--offline { border-color: var(--status-offline-border);
                 background: linear-gradient(145deg, var(--bg-surface), var(--status-offline-dim));
                 opacity: 0.7; }
```

---

### Button (`.btn`)

**Primary Button:**
```css
.btn-primary {
    background: var(--gradient-accent);
    color: var(--text-inverse);
    border: none;
    border-radius: var(--radius-md);
    padding: var(--space-3) var(--space-6);
    font-size: var(--text-base);
    font-weight: var(--fw-semibold);
    font-family: var(--font-sans);
    cursor: pointer;
    transition: opacity 150ms ease, box-shadow 150ms ease, transform 100ms ease;
    box-shadow: var(--shadow-glow-accent);
}

.btn-primary:hover {
    opacity: 0.9;
    box-shadow: 0 0 28px rgba(6,182,212,0.45);
    transform: translateY(-1px);
}

.btn-primary:active {
    transform: translateY(0);
    opacity: 1;
}
```

**Ghost Button:**
```css
.btn-ghost {
    background: transparent;
    color: var(--text-secondary);
    border: 1px solid var(--border-default);
    border-radius: var(--radius-md);
    padding: var(--space-3) var(--space-6);
    transition: color 150ms ease, border-color 150ms ease, background 150ms ease;
}

.btn-ghost:hover {
    color: var(--text-primary);
    border-color: var(--border-hover);
    background: var(--bg-elevated);
}
```

**Danger Button:**
```css
.btn-danger {
    background: transparent;
    color: var(--status-alert);
    border: 1px solid var(--status-alert-border);
    border-radius: var(--radius-md);
    padding: var(--space-3) var(--space-6);
    transition: background 150ms ease, box-shadow 150ms ease;
}

.btn-danger:hover {
    background: var(--status-alert-dim);
    box-shadow: var(--shadow-glow-alert);
}
```

**Icon Button:**
```css
.btn-icon {
    background: var(--bg-elevated);
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius-md);
    width: 36px;
    height: 36px;
    display: flex;
    align-items: center;
    justify-content: center;
    cursor: pointer;
    transition: background 150ms ease, border-color 150ms ease;
    color: var(--text-secondary);
}

.btn-icon:hover {
    background: var(--bg-input);
    border-color: var(--border-hover);
    color: var(--text-primary);
}
```

---

### Input (`.input`)

```css
.input {
    background: var(--bg-input);
    border: 1px solid var(--border-default);
    border-radius: var(--radius-md);
    color: var(--text-primary);
    font-family: var(--font-sans);
    font-size: var(--text-base);
    padding: var(--space-3) var(--space-4);
    width: 100%;
    transition: border-color 200ms ease, box-shadow 200ms ease;
    outline: none;
}

.input::placeholder {
    color: var(--text-muted);
}

.input:focus {
    border-color: var(--border-focus);
    box-shadow: 0 0 0 3px rgba(6, 182, 212, 0.15);
}

.input:disabled {
    opacity: 0.5;
    cursor: not-allowed;
}

/* Numeric inputs (thresholds) — monospace aligned */
.input--numeric {
    font-family: var(--font-mono);
    text-align: right;
    width: 100px;
}
```

---

### Badge / Status Pill (`.badge`)

```css
.badge {
    display: inline-flex;
    align-items: center;
    gap: var(--space-1);
    padding: 2px var(--space-2);
    border-radius: var(--radius-full);
    font-size: var(--text-xs);
    font-weight: var(--fw-semibold);
    letter-spacing: 0.04em;
    text-transform: uppercase;
}

.badge--ok      { color: var(--status-ok);      background: var(--status-ok-dim);      border: 1px solid var(--status-ok-border); }
.badge--warn    { color: var(--status-warn);     background: var(--status-warn-dim);    border: 1px solid var(--status-warn-border); }
.badge--alert   { color: var(--status-alert);    background: var(--status-alert-dim);   border: 1px solid var(--status-alert-border); }
.badge--offline { color: var(--status-offline);  background: var(--status-offline-dim); border: 1px solid var(--status-offline-border); }
.badge--info    { color: var(--accent-400);      background: var(--accent-900);          border: 1px solid rgba(6,182,212,0.25); }
```

---

### LED Status Dot (`.led`)

```css
.led {
    width: 8px;
    height: 8px;
    border-radius: var(--radius-full);
    display: inline-block;
    flex-shrink: 0;
}

.led--ok {
    background: var(--status-ok);
    box-shadow: 0 0 6px var(--status-ok);
    animation: pulse-ok 2.5s infinite;
}

.led--warn {
    background: var(--status-warn);
    box-shadow: 0 0 6px var(--status-warn);
    animation: pulse-warn 1.5s infinite;
}

.led--alert {
    background: var(--status-alert);
    box-shadow: 0 0 8px var(--status-alert);
    animation: pulse-alert 0.8s infinite;
}

.led--offline {
    background: var(--status-offline);
    box-shadow: none;
    /* No animation — device is dead */
}

@keyframes pulse-ok {
    0%, 100% { opacity: 1; }
    50%       { opacity: 0.5; }
}

@keyframes pulse-warn {
    0%, 100% { opacity: 1; transform: scale(1); }
    50%       { opacity: 0.7; transform: scale(1.2); }
}

@keyframes pulse-alert {
    0%, 100% { opacity: 1; transform: scale(1); }
    50%       { opacity: 0.4; transform: scale(1.3); }
}
```

---

### Signal Strength Bar (`.signal-bar`)

Visual indicator for modem RSSI value:
```
████████░░ -65 dBm  — Good (green)
██████░░░░ -75 dBm  — Fair (amber)
████░░░░░░ -85 dBm  — Weak (red)
██░░░░░░░░ -95 dBm  — Very weak (red)
```

```css
.signal-bar {
    display: flex;
    gap: 2px;
    align-items: flex-end;
}

.signal-bar__segment {
    width: 4px;
    border-radius: 2px;
    background: var(--bg-elevated);
    transition: background 300ms ease;
}

/* Heights increase left to right (4 segments) */
.signal-bar__segment:nth-child(1) { height: 4px; }
.signal-bar__segment:nth-child(2) { height: 8px; }
.signal-bar__segment:nth-child(3) { height: 12px; }
.signal-bar__segment:nth-child(4) { height: 16px; }

/* Color by RSSI: JS sets data-level="0|1|2|3|4" */
[data-signal="4"] .signal-bar__segment                        { background: var(--status-ok); }
[data-signal="3"] .signal-bar__segment:nth-child(-n+3)        { background: var(--status-ok); }
[data-signal="2"] .signal-bar__segment:nth-child(-n+2)        { background: var(--status-warn); }
[data-signal="1"] .signal-bar__segment:nth-child(1)           { background: var(--status-alert); }
```

RSSI → Level mapping (JS):
```javascript
function rssiToLevel(rssi) {
    if (rssi >= -65) return 4;  // Excellent
    if (rssi >= -75) return 3;  // Good
    if (rssi >= -85) return 2;  // Fair
    if (rssi >= -95) return 1;  // Weak
    return 0;                    // No signal
}
```

---

### Toast Notification (`.toast`)

Appears bottom-right, stacks upward, auto-dismisses after 4s.

```css
.toast-container {
    position: fixed;
    bottom: var(--space-6);
    right: var(--space-6);
    display: flex;
    flex-direction: column-reverse;
    gap: var(--space-3);
    z-index: 9999;
}

.toast {
    display: flex;
    align-items: center;
    gap: var(--space-3);
    padding: var(--space-4) var(--space-5);
    background: var(--bg-elevated);
    border: 1px solid var(--border-default);
    border-radius: var(--radius-lg);
    box-shadow: var(--shadow-xl);
    min-width: 280px;
    max-width: 400px;
    animation: toast-in 250ms cubic-bezier(0.34, 1.56, 0.64, 1);
}

.toast--success { border-left: 3px solid var(--status-ok); }
.toast--warn    { border-left: 3px solid var(--status-warn); }
.toast--error   { border-left: 3px solid var(--status-alert); }
.toast--info    { border-left: 3px solid var(--accent-500); }

@keyframes toast-in {
    from { opacity: 0; transform: translateX(40px) scale(0.95); }
    to   { opacity: 1; transform: translateX(0) scale(1); }
}
```

---

### Modal (`.modal`)

```css
.modal-backdrop {
    position: fixed;
    inset: 0;
    background: var(--bg-overlay);
    backdrop-filter: blur(4px);
    z-index: 1000;
    display: flex;
    align-items: center;
    justify-content: center;
    animation: fade-in 200ms ease;
}

.modal {
    background: var(--bg-surface);
    border: 1px solid var(--border-default);
    border-radius: var(--radius-xl);
    box-shadow: var(--shadow-xl);
    padding: var(--space-8);
    max-width: 520px;
    width: 100%;
    animation: modal-in 250ms cubic-bezier(0.34, 1.56, 0.64, 1);
}

@keyframes modal-in {
    from { opacity: 0; transform: translateY(20px) scale(0.97); }
    to   { opacity: 1; transform: translateY(0) scale(1); }
}
```

---

### Data Table (`.table`)

```css
.table-wrapper {
    overflow-x: auto;
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius-lg);
}

.table {
    width: 100%;
    border-collapse: collapse;
    font-size: var(--text-sm);
}

.table thead th {
    background: var(--bg-elevated);
    color: var(--text-muted);
    font-weight: var(--fw-semibold);
    font-size: var(--text-xs);
    letter-spacing: 0.06em;
    text-transform: uppercase;
    padding: var(--space-3) var(--space-4);
    text-align: left;
    border-bottom: 1px solid var(--border-subtle);
    white-space: nowrap;
}

.table tbody tr {
    border-bottom: 1px solid var(--border-subtle);
    transition: background 150ms ease;
}

.table tbody tr:last-child {
    border-bottom: none;
}

.table tbody tr:hover {
    background: var(--bg-elevated);
}

.table tbody td {
    padding: var(--space-3) var(--space-4);
    color: var(--text-secondary);
    vertical-align: middle;
}

.table tbody td.td--primary {
    color: var(--text-primary);
    font-weight: var(--fw-medium);
}

.table tbody td.td--mono {
    font-family: var(--font-mono);
    font-size: var(--text-sm);
}
```

---

## Page Layouts

### App Shell

```
┌──────────────────────────────────────────────────────────────────────┐
│  HEADER (height: 60px, sticky, z-index: 100)                        │
│  bg: --bg-surface, border-bottom: 1px solid --border-subtle          │
│  ┌─────────────────────────────────────────────────────────────────┐ │
│  │ [≡] [Logo] Pumping Station Monitor         [🔔 2] [User ▾]    │ │
│  └─────────────────────────────────────────────────────────────────┘ │
├─────────────────┬────────────────────────────────────────────────────┤
│  SIDEBAR        │  MAIN CONTENT AREA                                 │
│  (width: 240px) │  (flex-1, overflow-y: auto)                        │
│  (collapsed →   │  padding: --space-8                                │
│   60px on       │                                                    │
│   tablet)       │                                                    │
│                 │                                                    │
│  bg: --bg-      │                                                    │
│  surface        │                                                    │
│  border-right:  │                                                    │
│  1px solid      │                                                    │
│  --border-      │                                                    │
│  subtle         │                                                    │
└─────────────────┴────────────────────────────────────────────────────┘
```

### Sidebar Navigation Items

```css
.nav-item {
    display: flex;
    align-items: center;
    gap: var(--space-3);
    padding: var(--space-3) var(--space-4);
    border-radius: var(--radius-md);
    color: var(--text-secondary);
    font-size: var(--text-sm);
    font-weight: var(--fw-medium);
    cursor: pointer;
    transition: background 150ms ease, color 150ms ease;
    text-decoration: none;
    position: relative;
}

.nav-item:hover {
    background: var(--bg-elevated);
    color: var(--text-primary);
}

.nav-item.active {
    background: rgba(6, 182, 212, 0.12);
    color: var(--accent-400);
}

/* Active indicator bar */
.nav-item.active::before {
    content: '';
    position: absolute;
    left: 0;
    top: 50%;
    transform: translateY(-50%);
    width: 3px;
    height: 60%;
    background: var(--accent-500);
    border-radius: 0 2px 2px 0;
}
```

---

### Stat Summary Card (Dashboard Overview)

```
┌─────────────────────────────┐
│  ICON (24px, --accent-500)  │
│  ─────────────────────────  │
│  12                         │  ← --text-2xl, --fw-bold, --text-primary
│  Total Stations             │  ← --text-sm, --text-muted
│  ↑ 2 this month             │  ← --text-xs, --status-ok (delta)
└─────────────────────────────┘
  width: ~200px | padding: --space-6
  bg: --gradient-card
  border: 1px solid --border-subtle
  border-radius: --radius-lg
```

---

### Station Card (Overview Grid)

```
┌─────────────────────────────────────────┐  ← border-radius: --radius-lg
│  ● STATION_001          [NORMAL badge]  │  ← header row
│  Pumpstation Graz-Ost                   │  ← --text-sm, --text-muted
│  ─────────────────────────────────────  │
│                                         │
│            12.4 A                       │  ← --text-3xl, --fw-bold, --font-mono
│      Last update: 28s ago               │  ← --text-xs, --text-muted
│                                         │
│  ████████████░░░  67% of max (18A)     │  ← mini progress bar
│                                         │
│  Signal: ████░  Uptime: 24h 12m        │  ← --text-xs row
│                                    [→]  │  ← icon button to detail
└─────────────────────────────────────────┘
  min-width: 240px | padding: --space-5
  Hover: translateY(-2px), --shadow-lg
  Transition: 200ms ease all
```

Progress bar:
```css
.current-bar {
    height: 4px;
    border-radius: var(--radius-full);
    background: var(--bg-elevated);
    overflow: hidden;
}
.current-bar__fill {
    height: 100%;
    border-radius: var(--radius-full);
    background: var(--gradient-accent);
    transition: width 600ms cubic-bezier(0.34, 1.56, 0.64, 1);
}
/* Turns red when >80% of threshold */
.current-bar__fill--warn { background: linear-gradient(90deg, var(--status-warn), #fbbf24); }
.current-bar__fill--alert { background: linear-gradient(90deg, var(--status-alert), #f87171); }
```

---

## Animations & Motion

### Principles
- **Duration**: 150ms for micro (hover), 250ms for enter, 350ms for layout
- **Easing**: `cubic-bezier(0.34, 1.56, 0.64, 1)` for spring-in, `ease` for hover, `ease-in` for exit
- **Reduce motion**: wrap everything in `@media (prefers-reduced-motion: no-preference)`

### Global Keyframes

```css
@keyframes fade-in {
    from { opacity: 0; }
    to   { opacity: 1; }
}

@keyframes slide-up {
    from { opacity: 0; transform: translateY(16px); }
    to   { opacity: 1; transform: translateY(0); }
}

@keyframes slide-in-left {
    from { opacity: 0; transform: translateX(-20px); }
    to   { opacity: 1; transform: translateX(0); }
}

@keyframes scale-in {
    from { opacity: 0; transform: scale(0.94); }
    to   { opacity: 1; transform: scale(1); }
}

/* Stagger delay utility for card grids */
.stagger > *:nth-child(1) { animation-delay: 0ms; }
.stagger > *:nth-child(2) { animation-delay: 60ms; }
.stagger > *:nth-child(3) { animation-delay: 120ms; }
.stagger > *:nth-child(4) { animation-delay: 180ms; }
.stagger > *:nth-child(n+5) { animation-delay: 240ms; }
```

### Live Data Update Flash

When a new reading arrives, the metric value flashes briefly:
```css
@keyframes value-update {
    0%   { color: var(--accent-400); }
    100% { color: var(--text-primary); }
}

.value--updated {
    animation: value-update 800ms ease;
}
```

JS applies class on each new reading and removes it after:
```javascript
function flashUpdate(element) {
    element.classList.remove('value--updated');
    void element.offsetWidth; // Force reflow
    element.classList.add('value--updated');
}
```

---

## Chart Design (Chart.js)

### Live Current Chart

```javascript
const chartConfig = {
    type: 'line',
    data: {
        datasets: [{
            label: 'Current (A)',
            borderColor: '#06b6d4',
            borderWidth: 2,
            backgroundColor: 'rgba(6, 182, 212, 0.08)',
            pointRadius: 0,          // No dots on live chart
            pointHoverRadius: 4,
            tension: 0.3,            // Smooth curve
            fill: true,
        }]
    },
    options: {
        animation: { duration: 300 },
        plugins: {
            legend: { display: false },
            tooltip: {
                backgroundColor: '#182236',
                borderColor: 'rgba(255,255,255,0.12)',
                borderWidth: 1,
                titleColor: '#94a3b8',
                bodyColor: '#f0f6ff',
                bodyFont: { family: 'JetBrains Mono', size: 14 },
            },
            // High/Low threshold annotation lines
            annotation: {
                annotations: {
                    highLine: {
                        type: 'line', yMin: highThreshold, yMax: highThreshold,
                        borderColor: 'rgba(239,68,68,0.6)',
                        borderWidth: 1, borderDash: [6, 4],
                        label: { content: 'High', display: true,
                                 color: '#ef4444', font: { size: 10 } }
                    },
                    lowLine: {
                        type: 'line', yMin: lowThreshold, yMax: lowThreshold,
                        borderColor: 'rgba(245,158,11,0.6)',
                        borderWidth: 1, borderDash: [6, 4],
                        label: { content: 'Low', display: true,
                                 color: '#f59e0b', font: { size: 10 } }
                    }
                }
            }
        },
        scales: {
            x: {
                type: 'time',
                grid: { color: 'rgba(255,255,255,0.04)' },
                ticks: { color: '#4b5e77', font: { size: 11 } }
            },
            y: {
                min: 0,
                grid: { color: 'rgba(255,255,255,0.04)' },
                ticks: { color: '#4b5e77', font: { family: 'JetBrains Mono', size: 11 },
                         callback: v => v + ' A' }
            }
        }
    }
};
```

---

## Responsive Layout

### Breakpoints

```css
/* Mobile first */
/* xs: 0–479px   — stacked single column             */
/* sm: 480–767px — still stacked, slightly more room  */
/* md: 768–1023px — sidebar collapses to icon bar     */
/* lg: 1024–1279px — sidebar visible, 2-col grid      */
/* xl: 1280px+   — full layout, 3-col card grid       */

:root {
    --sidebar-width:          240px;
    --sidebar-collapsed-width: 60px;
    --header-height:           60px;
    --content-max-width:      1440px;
}

@media (max-width: 767px) {
    /* Sidebar becomes bottom navigation */
    .sidebar { display: none; }
    .bottom-nav { display: flex; }

    /* Cards: single column */
    .station-grid { grid-template-columns: 1fr; }

    /* Stat cards: 2-column */
    .stat-cards { grid-template-columns: repeat(2, 1fr); }
}

@media (min-width: 768px) and (max-width: 1023px) {
    .sidebar { width: var(--sidebar-collapsed-width); }
    .sidebar .nav-label { display: none; }
    .station-grid { grid-template-columns: repeat(2, 1fr); }
}

@media (min-width: 1024px) {
    .station-grid { grid-template-columns: repeat(3, 1fr); }
}

@media (min-width: 1280px) {
    .station-grid { grid-template-columns: repeat(4, 1fr); }
}
```

---

## Icons

Use **Lucide Icons** (inline SVG, tree-shakeable, consistent weight):
- CDN: `https://unpkg.com/lucide@latest`
- Stroke width: **1.5** throughout for a refined look
- Default size: **20px** (nav), **16px** (inline), **24px** (stat cards)

| Context | Icon Name |
|---|---|
| Dashboard overview | `layout-dashboard` |
| Station map | `map-pin` |
| Alert history | `bell` |
| Station management | `cpu` |
| User management | `users` |
| Settings / Config | `settings` |
| Online status | `wifi` |
| Offline status | `wifi-off` |
| High current | `trending-up` |
| Low current | `trending-down` |
| No current | `zap-off` |
| Signal strength | `signal` |
| Firmware | `microchip` |
| Download CSV | `download` |
| Acknowledge | `check-circle` |
| Add station | `plus-circle` |
| Edit | `pencil` |
| Delete | `trash-2` |
| Logout | `log-out` |

---

## Login Page Design

```
┌──────────────────────────────────────────────────────┐
│  bg: radial-gradient(ellipse at 40% 20%,             │
│       rgba(6,182,212,0.12) 0%, var(--bg-base) 60%)   │
│                                                      │
│                                                      │
│               ┌──────────────────────┐               │
│               │  bg: --bg-surface     │               │
│               │  blur(20px)           │               │
│               │  border: --border-    │               │
│               │  subtle               │               │
│               │                       │               │
│               │  [Pump icon — 48px]   │               │
│               │  Pumping Station      │               │
│               │  Monitor              │               │
│               │  ─────────────────    │               │
│               │  Monitor up to 400    │               │
│               │  stations in real     │               │
│               │  time.                │               │
│               │                       │               │
│               │  [G] Sign in with     │               │
│               │      Google           │               │
│               │                       │               │
│               │  v1.0.0               │               │
│               └──────────────────────┘               │
│                                                      │
└──────────────────────────────────────────────────────┘
```

Google button styling (custom, not Firebase UI default):
```css
.btn-google {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: var(--space-3);
    width: 100%;
    padding: var(--space-4);
    background: var(--bg-elevated);
    border: 1px solid var(--border-default);
    border-radius: var(--radius-md);
    color: var(--text-primary);
    font-size: var(--text-base);
    font-weight: var(--fw-medium);
    cursor: pointer;
    transition: background 150ms ease, border-color 150ms ease, box-shadow 150ms ease;
}

.btn-google:hover {
    background: var(--bg-input);
    border-color: var(--border-hover);
    box-shadow: var(--shadow-md);
}
```

---

## Accessibility

- All interactive elements have visible focus rings (`:focus-visible` only, no mouse focus ring)
- Minimum contrast ratio: **4.5:1** for body text (WCAG AA)
- Status is never conveyed by color alone — always paired with text/icon
- All icons have `aria-label` or `title`
- Form inputs have associated `<label>` elements
- Toast messages announced via `role="alert"`
- Modal traps focus while open, returns focus on close

```css
:focus-visible {
    outline: 2px solid var(--accent-500);
    outline-offset: 2px;
    border-radius: var(--radius-sm);
}
```
