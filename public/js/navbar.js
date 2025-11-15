const amburguesa = document.getElementById('amburguesa');
    const lista = document.getElementById('lista');
    const content = document.getElementById('content');

    amburguesa.addEventListener('click', () => {
      lista.classList.toggle('collapsed');
      content.classList.toggle('collapsed');
    });