import L from 'leaflet';
import 'leaflet/dist/leaflet.css';
import { ref, onValue, type Unsubscribe } from 'firebase/database';
import { db } from './firebase';
import { Utils } from './utils';
import { i18n } from './i18n';
import type { StationsSnapshot, StationConfig, LiveData, StationStatus } from './types';

export const MapView = {
  _map: null as L.Map | null,
  _markers: {} as Record<string, L.Marker>,
  _unsubscribe: null as Unsubscribe | null,

  render(container: HTMLElement): void {
    container.innerHTML = `
      <div class="content-wrapper">
        <div class="detail-header">
          <h1>${i18n.t('pumping_stations_map')}</h1>
          <span style="font-size: 0.85rem; color: var(--text-secondary);">${i18n.t('locations_provisioned')}</span>
        </div>
        <div class="map-view-container">
          <div id="map"></div>
        </div>
      </div>
    `;

    this._initMap();
  },

  _initMap(): void {
    // Default centered on Austria (Graz region)
    this._map = L.map('map').setView([47.0707, 15.4395], 7);

    L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
      attribution: '&copy; <a href="https://www.openstreetmap.org/copyright">OpenStreetMap</a> contributors',
    }).addTo(this._map);

    this._setupRealtimePins();
  },

  _setupRealtimePins(): void {
    const createMarkerIcon = (color: 'green' | 'gold' | 'red' | 'grey'): L.Icon => {
      const urlMap: Record<string, string> = {
        green: 'https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-green.png',
        red: 'https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-red.png',
        gold: 'https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-gold.png',
        grey: 'https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-grey.png',
      };
      return new L.Icon({
        iconUrl: urlMap[color],
        shadowUrl: 'https://cdnjs.cloudflare.com/ajax/libs/leaflet/0.7.7/images/marker-shadow.png',
        iconSize: [25, 41],
        iconAnchor: [12, 41],
        popupAnchor: [1, -34],
        shadowSize: [41, 41],
      });
    };

    const greenIcon = createMarkerIcon('green');
    const goldIcon = createMarkerIcon('gold');
    const redIcon = createMarkerIcon('red');
    const greyIcon = createMarkerIcon('grey');

    this._unsubscribe = onValue(ref(db, '/stations'), (snapshot) => {
      const data: StationsSnapshot = snapshot.val() ?? {};

      for (const [id, station] of Object.entries(data)) {
        const config = station.config ?? {} as Partial<StationConfig>;
        const live = station.live ?? {} as Partial<LiveData>;
        const status = station.status ?? {} as Partial<StationStatus>;

        const name = config.stationName ?? id;
        const lat = config.lat ?? 0.0;
        const lng = config.lng ?? 0.0;

        if (lat === 0.0 && lng === 0.0) continue;

        const online = status.online ?? false;
        const hasAlert = live.alert ?? false;
        const current = live.current ?? 0.0;

        let markerIcon = greyIcon;
        let statusText = i18n.t('offline').toUpperCase();

        if (online) {
          if (hasAlert) {
            markerIcon = redIcon;
            statusText = `${i18n.t('alert').toUpperCase()}: ${Utils.getAlertLabel(live.alertType ?? null)}`;
          } else {
            const highThreshold = config.highThreshold ?? 18.0;
            if (current > highThreshold * 0.8) {
              markerIcon = goldIcon;
              statusText = i18n.t('warning');
            } else {
              markerIcon = greenIcon;
              statusText = i18n.t('normal').toUpperCase();
            }
          }
        }

        const popupHtml = `
          <div class="map-popup">
            <h4 class="map-popup__title">${name}</h4>
            <div style="font-size: 0.75rem; font-family: monospace; color: #555; margin-bottom: 5px;">ID: ${id}</div>
            <div style="font-size: 0.8rem; margin: 4px 0;">${i18n.t('status')}: <strong>${statusText}</strong></div>
            <div style="font-size: 0.8rem; margin: 4px 0;">${i18n.t('current_a')}: <strong>${online ? current.toFixed(2) + ' A' : '--'}</strong></div>
            <a href="#/station/${id}" class="map-popup__link">${i18n.t('show_details')}</a>
          </div>
        `;

        if (this._markers[id]) {
          this._markers[id].setLatLng([lat, lng]);
          this._markers[id].setIcon(markerIcon);
          this._markers[id].setPopupContent(popupHtml);
        } else {
          this._markers[id] = L.marker([lat, lng], { icon: markerIcon })
            .addTo(this._map!)
            .bindPopup(popupHtml);
        }
      }

      // Remove markers for deleted stations
      for (const id of Object.keys(this._markers)) {
        if (!data[id]) {
          this._map?.removeLayer(this._markers[id]);
          delete this._markers[id];
        }
      }
    });
  },

  destroy(): void {
    this._unsubscribe?.();
    this._unsubscribe = null;
    this._map?.remove();
    this._map = null;
    this._markers = {};
  },
};
