export type Theme = 'light' | 'dark';

export const ThemeService = {
  getTheme(): Theme {
    const stored = localStorage.getItem('theme') as Theme | null;
    if (stored === 'light' || stored === 'dark') {
      return stored;
    }
    return 'light'; // Default theme is Light Mode
  },

  setTheme(theme: Theme): void {
    localStorage.setItem('theme', theme);
    document.documentElement.setAttribute('data-theme', theme);

    const metaTheme = document.querySelector('meta[name="theme-color"]');
    if (metaTheme) {
      metaTheme.setAttribute('content', theme === 'dark' ? '#080e1a' : '#f8fafc');
    }

    window.dispatchEvent(new CustomEvent('themechange', { detail: { theme } }));
  },

  toggleTheme(): Theme {
    const nextTheme: Theme = this.getTheme() === 'dark' ? 'light' : 'dark';
    this.setTheme(nextTheme);
    return nextTheme;
  },

  init(): void {
    const current = this.getTheme();
    this.setTheme(current);
  }
};
