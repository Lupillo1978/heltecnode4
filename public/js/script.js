





// Mostrar reloj en hora de Sinaloa (UTC-7)
  setInterval(() => {
    const now = new Date();
    const sinaloaOffset = -7; // UTC-7 (ajústalo si hay horario de verano)
    const localTime = new Date(now.getTime() + sinaloaOffset * 60 * 60 * 1000);
    clock.textContent = localTime.toISOString().substr(11, 8);
  }, 1000);




const menuToggle = document.getElementById('menu-toggle');
const navLinks = document.getElementById('nav-links');

menuToggle.addEventListener('click', () => {
  navLinks.classList.toggle('show');
});

