import { AuthService } from './auth';
import { Dashboard } from './dashboard';
import { StationDetail } from './station-detail';
import { MapView } from './map';
import { AlertsList } from './alerts';
import { StationManagement } from './station-mgmt';
import { UserManagement } from './user-mgmt';
import { Utils, refreshIcons } from './utils';
import { i18n } from './i18n';
import { ThemeService } from './theme';

export const App = {
  _activeView: null as { destroy?: () => void } | null,

  init(): void {
    console.log('App init...');
    ThemeService.init();
    i18n.init();
    
    // Check initial route. If none, default to dashboard.
    if (!window.location.hash || window.location.hash === '#') {
      window.location.hash = '#/';
    }

    AuthService.init((user, errorType) => {
      if (user) {
        this.renderLayout();
        this.handleRoute();
      } else {
        if (errorType === 'ACCESS_DENIED') {
          Utils.showToast(i18n.t('access_denied'), 'error');
        }
        this.renderLogin();
      }
    });

    window.addEventListener('hashchange', () => {
      if (AuthService.isAuthenticated()) {
        this.handleRoute();
      }
    });

    window.addEventListener('languagechange', () => {
      if (AuthService.isAuthenticated()) {
        this.renderLayout();
        this.handleRoute();
      } else {
        this.renderLogin();
      }
    });
  },

  renderLogin(): void {
    const appDiv = document.getElementById('app')!;
    appDiv.innerHTML = `
      <div class="login-page">
        <!-- Header Actions on Login Page -->
        <div style="position: absolute; top: var(--space-4); right: var(--space-4); display: flex; align-items: center; gap: var(--space-3); z-index: 100;">
          <button class="btn-theme-toggle" id="btn-theme-toggle-login" title="${i18n.t('toggle_theme')}" aria-label="Toggle Theme">
            <i data-lucide="${ThemeService.getTheme() === 'dark' ? 'sun' : 'moon'}"></i>
          </button>
          <div class="header__lang-menu" id="btn-lang-menu-login">
            <i data-lucide="languages" style="width: 16px; height: 16px; color: var(--text-secondary);"></i>
            <span style="font-size: 0.85rem; font-weight: 600; text-transform: uppercase;">${i18n.currentLang}</span>
            <i data-lucide="chevron-down" style="width: 12px; height: 12px; color: var(--text-secondary); transition: transform 0.2s;"></i>
            
            <div class="lang-dropdown" id="lang-dropdown-login">
              <div class="lang-dropdown__item ${i18n.currentLang === 'de' ? 'active' : ''}" data-lang="de">
                <span class="lang-flag">🇩🇪</span> Deutsch
              </div>
              <div class="lang-dropdown__item ${i18n.currentLang === 'en' ? 'active' : ''}" data-lang="en">
                <span class="lang-flag">🇬🇧</span> English
              </div>
            </div>
          </div>
        </div>

        <div class="card login-card scale-in">
          <div class="login-card__logo">
            <i data-lucide="waves" style="width: 48px; height: 48px;"></i>
          </div>
          <div class="login-card__title">
            <h2>Pumping Station IoT</h2>
            <div class="login-card__subtitle">Cloud Dashboard & Control</div>
          </div>
          <button id="btn-login" class="btn btn-google">
            <svg width="18" height="18" viewBox="0 0 18 18">
              <path fill="#4285F4" d="M17.64 9.2045c0-.6381-.0573-1.2518-.1636-1.8409H9v3.4814h4.8436c-.2086 1.125-.8427 2.0782-1.7959 2.7164v2.2581h2.9082c1.7018-1.5668 2.6836-3.874 2.6836-6.615z"/>
              <path fill="#34A853" d="M9 18c2.43 0 4.4673-.806 5.9564-2.1805l-2.9082-2.2581c-.8059.54-1.8368.859-3.0482.859-2.344 0-4.3282-1.5831-5.036-3.7104H.9574v2.3318C2.4382 15.9832 5.4818 18 9 18z"/>
              <path fill="#FBBC05" d="M3.964 10.71c-.18-.54-.2822-1.1168-.2822-1.71s.1023-1.17.2823-1.71V4.9582H.9573C.3477 6.1732 0 7.5477 0 9s.3477 2.8268.9573 4.0418L3.964 10.71z"/>
              <path fill="#EA4335" d="M9 3.5795c1.3214 0 2.5077.4541 3.4405 1.346l2.5813-2.5814C13.4632.8918 11.426 0 9 0 5.4818 0 2.4382 2.0168.9573 4.9582L3.964 7.29C4.6718 5.1627 6.6559 3.5795 9 3.5795z"/>
            </svg>
            ${i18n.t('sign_in_google')}
          </button>
          <div class="login-card__footer">
            ${i18n.t('footer_personnel')}
          </div>
        </div>
      </div>
    `;

    refreshIcons();

    document.getElementById('btn-login')?.addEventListener('click', () => {
      AuthService.loginWithGoogle();
    });

    document.getElementById('btn-theme-toggle-login')?.addEventListener('click', () => {
      ThemeService.toggleTheme();
      this.renderLogin();
    });

    const langMenuLogin = document.getElementById('btn-lang-menu-login');
    langMenuLogin?.addEventListener('click', (e) => {
      e.stopPropagation();
      langMenuLogin.classList.toggle('open');
    });

    document.querySelectorAll('#lang-dropdown-login .lang-dropdown__item').forEach(item => {
      item.addEventListener('click', (e) => {
        e.stopPropagation();
        const lang = (item as HTMLElement).dataset.lang as 'de' | 'en';
        if (lang) {
          i18n.setLanguage(lang);
        }
        langMenuLogin?.classList.remove('open');
      });
    });

    document.addEventListener('click', () => {
      langMenuLogin?.classList.remove('open');
    });
  },

  renderLayout(): void {
    const appDiv = document.getElementById('app')!;
    const adminNavs = AuthService.isAdmin()
      ? `
        <div class="nav-item" data-route="#/management/stations">
          <i data-lucide="server"></i>
          <span class="nav-label">${i18n.t('stations')}</span>
        </div>
        <div class="nav-item" data-route="#/management/users">
          <i data-lucide="users"></i>
          <span class="nav-label">${i18n.t('users')}</span>
        </div>
      `
      : '';

    const adminBottomNavs = AuthService.isAdmin()
      ? `
        <a href="#/management/stations" class="bottom-nav__item" data-route="#/management/stations">
          <i data-lucide="server"></i>
          <span>${i18n.t('stations')}</span>
        </a>
      `
      : '';

    appDiv.innerHTML = `
      <!-- Top Header -->
      <header class="header">
        <div class="header__brand">
          <i data-lucide="waves"></i>
          Pumping Station IoT
        </div>
        <div class="header__actions">
          <button class="btn-theme-toggle" id="btn-theme-toggle" title="${i18n.t('toggle_theme')}" aria-label="Toggle Theme">
            <i data-lucide="${ThemeService.getTheme() === 'dark' ? 'sun' : 'moon'}"></i>
          </button>
          <!-- Language Dropdown Menu -->
          <div class="header__lang-menu" id="btn-lang-menu">
            <i data-lucide="languages" style="width: 16px; height: 16px; color: var(--text-secondary);"></i>
            <span style="font-size: 0.85rem; font-weight: 600; text-transform: uppercase;">${i18n.currentLang}</span>
            <i data-lucide="chevron-down" style="width: 12px; height: 12px; color: var(--text-secondary); transition: transform 0.2s;"></i>
            
            <div class="lang-dropdown" id="lang-dropdown">
              <div class="lang-dropdown__item ${i18n.currentLang === 'de' ? 'active' : ''}" data-lang="de">
                <span class="lang-flag">🇩🇪</span> Deutsch
              </div>
              <div class="lang-dropdown__item ${i18n.currentLang === 'en' ? 'active' : ''}" data-lang="en">
                <span class="lang-flag">🇬🇧</span> English
              </div>
            </div>
          </div>

          <div class="header__user-menu" id="btn-user-menu">
            ${AuthService.currentUser?.photoURL ? `<img src="${AuthService.currentUser.photoURL}" style="width:24px;height:24px;border-radius:12px;">` : '<i data-lucide="user"></i>'}
            <span style="font-size: 0.85rem; margin-right: 4px;">${AuthService.currentUser?.displayName || AuthService.currentUser?.email}</span>
            <i data-lucide="chevron-down" style="width: 14px; height: 14px; color: var(--text-secondary);"></i>
          </div>
        </div>
      </header>

      <div class="main-container">
        <!-- Sidebar Navigation (Desktop) -->
        <aside class="sidebar">
          <div style="font-size: 0.75rem; text-transform: uppercase; color: var(--text-muted); font-weight: 700; margin: 10px 10px 5px 10px; letter-spacing: 0.05em;">
            ${i18n.t('monitoring')}
          </div>
          <nav class="sidebar__nav">
            <div class="nav-item" data-route="#/">
              <i data-lucide="layout-dashboard"></i>
              <span class="nav-label">${i18n.t('dashboard')}</span>
            </div>
            <div class="nav-item" data-route="#/map">
              <i data-lucide="map"></i>
              <span class="nav-label">${i18n.t('map')}</span>
            </div>
            <div class="nav-item" data-route="#/alerts">
              <i data-lucide="bell"></i>
              <span class="nav-label">${i18n.t('alerts')}</span>
            </div>

            ${AuthService.isAdmin() ? `
              <div style="font-size: 0.75rem; text-transform: uppercase; color: var(--text-muted); font-weight: 700; margin: 20px 10px 5px 10px; letter-spacing: 0.05em;">
                ${i18n.t('administration')}
              </div>
            ` : ''}
            ${adminNavs}
          </nav>
        </aside>

        <!-- Main Content Area -->
        <main class="content-area" id="main-content">
          <!-- Views will be mounted here -->
        </main>
      </div>

      <!-- Bottom Navigation (Mobile Only) -->
      <nav class="bottom-nav">
        <a href="#/" class="bottom-nav__item" data-route="#/">
          <i data-lucide="layout-dashboard"></i>
          <span>${i18n.t('home')}</span>
        </a>
        <a href="#/map" class="bottom-nav__item" data-route="#/map">
          <i data-lucide="map"></i>
          <span>${i18n.t('map_mobile')}</span>
        </a>
        <a href="#/alerts" class="bottom-nav__item" data-route="#/alerts">
          <i data-lucide="bell"></i>
          <span>${i18n.t('alerts')}</span>
        </a>
        ${adminBottomNavs}
      </nav>
    `;

    refreshIcons();

    // Theme toggle trigger
    document.getElementById('btn-theme-toggle')?.addEventListener('click', () => {
      ThemeService.toggleTheme();
      this.renderLayout();
      this.handleRoute();
    });

    // User menu logout trigger
    document.getElementById('btn-user-menu')?.addEventListener('click', () => {
      if (confirm(i18n.t('logout_confirm'))) {
        AuthService.logout();
      }
    });

    // Language Menu Dropdown triggers
    const langMenu = document.getElementById('btn-lang-menu');
    langMenu?.addEventListener('click', (e) => {
      e.stopPropagation();
      langMenu.classList.toggle('open');
    });

    document.querySelectorAll('#lang-dropdown .lang-dropdown__item').forEach(item => {
      item.addEventListener('click', (e) => {
        e.stopPropagation();
        const lang = (item as HTMLElement).dataset.lang as 'de' | 'en';
        if (lang) {
          i18n.setLanguage(lang);
        }
        langMenu?.classList.remove('open');
      });
    });

    // Close language menu on clicking outside
    document.addEventListener('click', () => {
      langMenu?.classList.remove('open');
    });

    // Sidebar navigation clicks
    const navItems = document.querySelectorAll('.nav-item');
    navItems.forEach(item => {
      item.addEventListener('click', () => {
        const route = (item as HTMLElement).dataset.route;
        if (route) window.location.hash = route;
      });
    });
  },

  handleRoute(): void {
    const hash = window.location.hash || '#/';
    const contentDiv = document.getElementById('main-content')!;

    // Cleanup previous view to avoid memory/listener leaks
    if (this._activeView && typeof this._activeView.destroy === 'function') {
      this._activeView.destroy();
    }
    contentDiv.innerHTML = '';
    this._activeView = null;

    // Update navigation active states
    document.querySelectorAll('.nav-item, .bottom-nav__item').forEach(item => {
      item.classList.remove('active');
    });

    // Mark matching nav item active
    const activeNav = document.querySelector(`.nav-item[data-route="${hash}"], .bottom-nav__item[data-route="${hash}"]`);
    if (activeNav) activeNav.classList.add('active');

    // Route logic
    if (hash === '#/') {
      Dashboard.render(contentDiv);
      this._activeView = Dashboard;
    } else if (hash === '#/map') {
      MapView.render(contentDiv);
      this._activeView = MapView;
    } else if (hash === '#/alerts') {
      AlertsList.render(contentDiv, null);
      this._activeView = AlertsList;
    } else if (hash.startsWith('#/station/')) {
      const stationId = hash.replace('#/station/', '');
      StationDetail.render(contentDiv, stationId);
      this._activeView = StationDetail;
    } else if (hash === '#/management/stations' && AuthService.isAdmin()) {
      StationManagement.render(contentDiv);
      this._activeView = StationManagement;
    } else if (hash === '#/management/users' && AuthService.isAdmin()) {
      UserManagement.render(contentDiv);
      this._activeView = UserManagement;
    } else {
      // 404 or Unauthorized
      contentDiv.innerHTML = `
        <div class="content-wrapper">
          <div class="card" style="text-align:center; padding: 60px;">
            <i data-lucide="alert-triangle" style="width: 48px; height: 48px; color: var(--status-warn); margin-bottom: 20px;"></i>
            <h3>${i18n.t('page_not_found')}</h3>
            <p style="color: var(--text-secondary); margin-top: 10px;">${i18n.t('select_valid_nav')}</p>
          </div>
        </div>
      `;
      refreshIcons();
    }
  }
};
