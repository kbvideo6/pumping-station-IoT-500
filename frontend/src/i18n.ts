export type Lang = 'de' | 'en';

const translations: Record<Lang, Record<string, string>> = {
  de: {
    // Common / Auth
    access_denied: 'Ihre E-Mail ist nicht im System registriert. Bitte kontaktieren Sie den Administrator.',
    footer_personnel: 'Zugang nur für autorisiertes Personal.',
    sign_in_google: 'Sign in with Google',
    logout_confirm: 'Möchten Sie sich abmelden?',
    logout_success: 'Abgemeldet.',
    logout: 'Abmelden',
    login_failed: 'Anmeldung fehlgeschlagen: ',
    unknown_error: 'Unbekannter Fehler',

    // Sidebar
    monitoring: 'Überwachung',
    administration: 'Verwaltung',
    dashboard: 'Dashboard',
    map: 'Landkarte',
    alerts: 'Alarme',
    stations: 'Stationen',
    users: 'Benutzer',

    // Mobile Nav
    home: 'Home',
    map_mobile: 'Karte',

    // General
    page_not_found: 'Seite nicht gefunden oder Zugriff verweigert.',
    select_valid_nav: 'Bitte wählen Sie einen gültigen Bereich aus der Navigation.',
    loading: 'Lade...',
    save: 'Speichern',
    cancel: 'Abbrechen',
    actions: 'Aktionen',
    status: 'Status',
    type: 'Typ',
    offline: 'Offline',
    online: 'Online',
    alert: 'Alarm',
    normal: 'Normal',
    toggle_theme: 'Erscheinungsbild wechseln',

    // Dashboard Page
    pumping_stations_overview: 'Argus360 Übersicht',
    total_stations: 'Stationen Gesamt',
    active_alerts: 'Aktive Alarme',
    search_placeholder: 'Station suchen (Name oder ID)...',
    all_statuses: 'Alle Status',
    loading_stations: 'Lade Stationen...',

    // Alerts Page
    alert_history_station: 'Alarm-Verlauf dieser Station',
    all_system_alerts: 'Alle Systemalarme',
    all: 'Alle',
    unack: 'Unbehandelt',
    ack: 'Bestätigt',
    all_types: 'Alle Typen',
    load: 'Laden',
    col_timestamp: 'Zeitpunkt',
    col_station: 'Station',
    col_type: 'Typ',
    col_value: 'Messwert',
    col_threshold: 'Grenzwert',
    col_status: 'Status',
    col_action: 'Aktion',
    alert_filter_prompt: 'Filter wählen und auf Laden klicken.',
    alerts_load_error: 'Fehler beim Laden der Alarme.',
    no_alerts_found: 'Keine Alarme gefunden.',
    open_alert: 'Offen',
    btn_acknowledge: 'Bestätigen',
    alert_acknowledged: 'Alarm bestätigt.',
    failed_acknowledge: 'Fehler beim Bestätigen.',

    // Map Page
    pumping_stations_map: 'Argus360 Landkarte',
    locations_provisioned: 'Standorte aller provisionierten Stationen',
    warning: 'WARNUNG',
    show_details: 'Details anzeigen →',

    // Station Detail
    back: 'Zurück',
    current_amperage: 'Aktuelle Stromstärke',
    voltage: 'Netzspannung',
    power: 'Wirkleistung',
    energy_kwh: 'Energie (kWh)',
    power_factor: 'Leistungsfaktor',
    frequency_hz: 'Netzfrequenz',
    signal_strength: 'Signalstärke',
    uptime: 'Laufzeit',
    battery: 'Batterie',
    live_view: 'Live-Anzeige',
    last_50_points: 'Letzte 50 Messpunkte',
    tab_alerts: 'Alarme',
    tab_history: 'Verlauf',
    tab_config: 'Konfiguration',
    current_a: 'Strom (A)',
    voltage_v: 'Spannung (V)',
    chart_limit: 'Grenzwert',
    device_config: 'Gerätekonfiguration',
    station_name: 'Station Name',
    pump_power: 'Pumpenleistung (kW)',
    upper_threshold: 'Oberer Grenzwert (A)',
    lower_threshold: 'Unterer Grenzwert (A)',
    upper_voltage_threshold: 'Oberer Spannungsgrenzwert (V)',
    lower_voltage_threshold: 'Unterer Spannungsgrenzwert (V)',
    report_interval: 'Sendeintervall (Sekunden)',
    history_interval: 'Verlaufsintervall (Minuten)',
    config_poll_interval: 'Konfig-Abfrage (Sekunden)',
    btn_save_config: 'Konfiguration Speichern',
    config_saved: 'Konfiguration gespeichert.',
    failed_save_config: 'Fehler beim Speichern.',

    // History Page
    query_history: 'Messverlauf Abfragen',
    from: 'Von',
    to: 'Bis',
    load_data: 'Daten laden',
    export_csv: 'CSV exportieren',
    history_chart: 'Verlaufsdiagramm',
    select_range_prompt: 'Datumsbereich wählen und Daten laden.',
    select_range_warn: 'Bitte Von- und Bis-Datum auswählen.',
    yes: 'Ja',
    no: 'Nein',
    no_data_range: 'Keine Daten für diesen Zeitraum vorhanden.',
    history_load_error: 'Fehler beim Laden des Verlaufs.',
    col_voltage: 'Spannung (V)',
    col_power: 'Leistung (W)',
    col_energy: 'Energie (kWh)',
    col_pf: 'LF',

    // Station Management
    manage_stations: 'Stationen verwalten',
    add_station: 'Station hinzufügen',
    station_id: 'Station ID',
    name: 'Name',
    gps: 'GPS',
    no_stations_found: 'Keine Stationen gefunden.',
    confirm_delete_station: 'Möchten Sie die Station {id} wirklich löschen?',
    station_deleted: 'Station gelöscht.',
    failed_delete: 'Fehler beim Löschen.',
    provisioning_station: 'Station wird provisioniert...',
    station_provisioned: 'Station erfolgreich provisioniert!',
    failed_provision: 'Fehler beim Provisionieren.',
    create_new_station: 'Neue Station anlegen',
    latitude: 'Geografische Breite (Lat)',
    longitude: 'Geografische Länge (Lng)',
    create: 'Erstellen',
    fill_id_name_warn: 'Bitte füllen Sie ID und Name aus.',
    station_exists_warn: 'Station existiert bereits!',
    station_created: 'Station erstellt.',
    failed_create: 'Fehler beim Erstellen.',
    edit_station: 'Station bearbeiten',
    changes_saved: 'Änderungen gespeichert.',
    failed_edit: 'Fehler beim Bearbeiten.',

    // User Management
    no_access: 'Kein Zugriff.',
    user_management: 'Benutzerverwaltung',
    add_user: 'Benutzer hinzufügen',
    email: 'E-Mail',
    role: 'Rolle',
    last_seen: 'Zuletzt angemeldet',
    created: 'Erstellt',
    no_users_found: 'Keine Benutzer gefunden.',
    role_viewer: 'Viewer',
    role_admin: 'Admin',
    change_role: 'Rolle ändern',
    delete: 'Löschen',
    role_updated: 'Rolle aktualisiert.',
    failed_update_role: 'Fehler beim Ändern der Rolle.',
    confirm_delete_user: 'Möchten Sie den Benutzer {email} wirklich löschen?',
    user_deleted: 'Benutzer gelöscht.',
    invite_new_user: 'Neuen Benutzer einladen',
    invite: 'Einladen',
    enter_email_warn: 'Bitte E-Mail eingeben.',
    user_exists_warn: 'Benutzer existiert bereits!',
    user_invited: 'Benutzer eingeladen.',
    failed_invite: 'Fehler beim Einladen.',

    // Alert labels
    alert_HIGH_CURRENT: 'Überstrom',
    alert_LOW_CURRENT: 'Unterstrom / Trockenlauf',
    alert_NO_CURRENT: 'Stromlos / Fehler',
    alert_HIGH_VOLTAGE: 'Überspannung',
    alert_LOW_VOLTAGE: 'Unterspannung',
    alert_DEVICE_OFFLINE: 'Verbindung verloren',
    alert_SENSOR_OFFLINE: 'Sensor Offline',
    alert_UNKNOWN: 'Unbekannt',

    // Relative Uptime
    uptime_just_now: 'Gerade eben',
    uptime_seconds: 'Vor {seconds} Sek.',
    uptime_minutes: 'Vor {minutes} Min.',
    uptime_hours: 'Vor {hours} Std.',
  },
  en: {
    // Common / Auth
    access_denied: 'Your email is not registered in the system. Please contact the administrator.',
    footer_personnel: 'Access only for authorized personnel.',
    sign_in_google: 'Sign in with Google',
    logout_confirm: 'Are you sure you want to log out?',
    logout_success: 'Logged out.',
    logout: 'Logout',
    login_failed: 'Sign-in failed: ',
    unknown_error: 'Unknown error',

    // Sidebar
    monitoring: 'Monitoring',
    administration: 'Administration',
    dashboard: 'Dashboard',
    map: 'Map',
    alerts: 'Alerts',
    stations: 'Stations',
    users: 'Users',

    // Mobile Nav
    home: 'Home',
    map_mobile: 'Map',

    // General
    page_not_found: 'Page not found or access denied.',
    select_valid_nav: 'Please select a valid section from the navigation.',
    loading: 'Loading...',
    save: 'Save',
    cancel: 'Cancel',
    actions: 'Actions',
    status: 'Status',
    type: 'Type',
    offline: 'Offline',
    online: 'Online',
    alert: 'Alert',
    normal: 'Normal',
    toggle_theme: 'Toggle Theme',

    // Dashboard Page
    pumping_stations_overview: 'Argus360 Overview',
    total_stations: 'Total Stations',
    active_alerts: 'Active Alerts',
    search_placeholder: 'Search station (name or ID)...',
    all_statuses: 'All Status',
    loading_stations: 'Loading stations...',

    // Alerts Page
    alert_history_station: 'Alert history of this station',
    all_system_alerts: 'All System Alerts',
    all: 'All',
    unack: 'Active',
    ack: 'Acknowledged',
    all_types: 'All Types',
    load: 'Load',
    col_timestamp: 'Timestamp',
    col_station: 'Station',
    col_type: 'Type',
    col_value: 'Value',
    col_threshold: 'Threshold',
    col_status: 'Status',
    col_action: 'Action',
    alert_filter_prompt: 'Select filters and click Load.',
    alerts_load_error: 'Error loading alerts.',
    no_alerts_found: 'No alerts found.',
    open_alert: 'Open',
    btn_acknowledge: 'Acknowledge',
    alert_acknowledged: 'Alert acknowledged.',
    failed_acknowledge: 'Failed to acknowledge.',

    // Map Page
    pumping_stations_map: 'Argus360 Map',
    locations_provisioned: 'Locations of all provisioned stations',
    warning: 'WARNING',
    show_details: 'Show details →',

    // Station Detail
    back: 'Back',
    current_amperage: 'Current Amperage',
    voltage: 'Mains Voltage',
    power: 'Active Power',
    energy_kwh: 'Energy (kWh)',
    power_factor: 'Power Factor',
    frequency_hz: 'Mains Frequency',
    signal_strength: 'Signal Strength',
    uptime: 'Uptime',
    battery: 'Battery',
    live_view: 'Live View',
    last_50_points: 'Last 50 data points',
    tab_alerts: 'Alerts',
    tab_history: 'History',
    tab_config: 'Configuration',
    current_a: 'Current (A)',
    voltage_v: 'Voltage (V)',
    chart_limit: 'Limit',
    device_config: 'Device Configuration',
    station_name: 'Station Name',
    pump_power: 'Pump Power (kW)',
    upper_threshold: 'High Threshold (A)',
    lower_threshold: 'Low Threshold (A)',
    upper_voltage_threshold: 'High Voltage Threshold (V)',
    lower_voltage_threshold: 'Low Voltage Threshold (V)',
    report_interval: 'Report Interval (Seconds)',
    history_interval: 'History Interval (Minutes)',
    config_poll_interval: 'Config Poll Interval (Seconds)',
    btn_save_config: 'Save Configuration',
    config_saved: 'Configuration saved.',
    failed_save_config: 'Failed to save configuration.',

    // History Page
    query_history: 'Query Measurement History',
    from: 'From',
    to: 'To',
    load_data: 'Load Data',
    export_csv: 'Export CSV',
    history_chart: 'History Chart',
    select_range_prompt: 'Select date range and load data.',
    select_range_warn: 'Please select both From and To dates.',
    yes: 'Yes',
    no: 'No',
    no_data_range: 'No data available for this period.',
    history_load_error: 'Error loading history.',
    col_voltage: 'Voltage (V)',
    col_power: 'Power (W)',
    col_energy: 'Energy (kWh)',
    col_pf: 'PF',

    // Station Management
    manage_stations: 'Manage Stations',
    add_station: 'Add Station',
    station_id: 'Station ID',
    name: 'Name',
    gps: 'GPS',
    no_stations_found: 'No stations found.',
    confirm_delete_station: 'Are you sure you want to delete station {id}?',
    station_deleted: 'Station deleted.',
    failed_delete: 'Failed to delete.',
    provisioning_station: 'Provisioning station...',
    station_provisioned: 'Station successfully provisioned!',
    failed_provision: 'Failed to provision.',
    create_new_station: 'Create New Station',
    latitude: 'Latitude (Lat)',
    longitude: 'Longitude (Lng)',
    create: 'Create',
    fill_id_name_warn: 'Please fill in both ID and name.',
    station_exists_warn: 'Station already exists!',
    station_created: 'Station created.',
    failed_create: 'Failed to create.',
    edit_station: 'Edit Station',
    changes_saved: 'Changes saved.',
    failed_edit: 'Failed to edit.',

    // User Management
    no_access: 'Access denied.',
    user_management: 'User Management',
    add_user: 'Add User',
    email: 'Email',
    role: 'Role',
    last_seen: 'Last Seen',
    created: 'Created',
    no_users_found: 'No users found.',
    role_viewer: 'Viewer',
    role_admin: 'Admin',
    change_role: 'Change Role',
    delete: 'Delete',
    role_updated: 'Role updated.',
    failed_update_role: 'Failed to update role.',
    confirm_delete_user: 'Are you sure you want to delete user {email}?',
    user_deleted: 'User deleted.',
    invite_new_user: 'Invite New User',
    invite: 'Invite',
    enter_email_warn: 'Please enter an email address.',
    user_exists_warn: 'User already exists!',
    user_invited: 'User invited.',
    failed_invite: 'Failed to invite.',

    // Alert labels
    alert_HIGH_CURRENT: 'Overcurrent',
    alert_LOW_CURRENT: 'Undercurrent / Dry Run',
    alert_NO_CURRENT: 'No Current / Fault',
    alert_HIGH_VOLTAGE: 'Overvoltage',
    alert_LOW_VOLTAGE: 'Undervoltage',
    alert_DEVICE_OFFLINE: 'Connection Lost',
    alert_SENSOR_OFFLINE: 'Sensor Offline',
    alert_UNKNOWN: 'Unknown',

    // Relative Uptime
    uptime_just_now: 'Just now',
    uptime_seconds: '{seconds}s ago',
    uptime_minutes: '{minutes}m ago',
    uptime_hours: '{hours}h ago',
  },
};

export const i18n = {
  currentLang: 'de' as Lang,

  init(): void {
    const saved = localStorage.getItem('lang');
    if (saved === 'en' || saved === 'de') {
      this.currentLang = saved;
    } else {
      this.currentLang = 'de';
    }
    document.documentElement.lang = this.currentLang;
  },

  setLanguage(lang: Lang): void {
    if (lang === this.currentLang) return;
    this.currentLang = lang;
    localStorage.setItem('lang', lang);
    document.documentElement.lang = lang;
    window.dispatchEvent(new CustomEvent('languagechange'));
  },

  t(key: string, variables?: Record<string, string | number>): string {
    const langDict = translations[this.currentLang];
    let template = langDict[key] || translations['de'][key] || key;

    if (variables) {
      for (const [k, v] of Object.entries(variables)) {
        template = template.replace(new RegExp(`{${k}}`, 'g'), String(v));
      }
    }

    return template;
  },
};
