import { App } from './app';
import './style.css'; // Assuming we create a style.css that imports the old CSS files

document.addEventListener('DOMContentLoaded', () => {
  App.init();
});
