
//Importa los modulos necesarios
const express = require('express');                   // Framework para servidor web
const bodyParser = require('body-parser');            // Para parsear datos del formulario
const SerialPort = require('serialport').SerialPort;  // Para comunicación serial con ESP32
const path = require('path');  

const app = express();
const port = 3000;                                    // Puerto en el que corre el servidor web

// Puerto serial (ajusta el puerto a tu ESP32)
// Configura el puerto serial para comunicarse con el ESP32
const esp32 = new SerialPort({
  path: 'COM3', // O COM3 en Windows  // Puerto serie (ajusta según tu sistema, ejemplo 'COM3' en Windows)
  baudRate: 9600,      // Velocidad de comunicación (debe coincidir con ESP32)
});

// Enviar hora actual automáticamente cada 5 minutos (300000 ms)
setInterval(() => {
  const now = new Date();
  // Obtiene horas y minutos con formato de dos dígitos
  const hora = now.getHours().toString().padStart(2, '0');
  const min = now.getMinutes().toString().padStart(2, '0');
  const timeString = `time:${hora}:${min}`;   // Formato esperado por el nodo esclavo
  // Envía la hora al ESP32 por puerto serial
  esp32.write(timeString + '\n');
  console.log("⏰ Hora actual reenviada automáticamente:", timeString);
}, 5 * 60 * 1000);  // Intervalo: 5 minutos

// Middleware para procesar peticiones HTTP
app.use(bodyParser.urlencoded({ extended: false }));  // Para datos de formulario x-www-form-urlencoded
app.use(express.static('public'));                    // Para servir archivos estáticos (index.html, css, js)

//Modificacion1 Modificación para aceptar JSON en requests
app.use(express.json());


app.use(express.static(path.join(__dirname, 'public')));

// Página web para controlar el ESP32 Ruta principal para servir la página web
app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname + '/public/horarios.html'));
});

// Recibir datos desde el formulario
//Ruta para recibir datos simples de temporizador (no usada en tu index.html, pero definida)
app.post('/set-timer', (req, res) => {
  const {nodoID, onTime, offTime } = req.body;  // Extrae datos del cuerpo de la petición
  const message = `${nodoID},${onTime},${offTime}\n`;  // Mensaje que se envía al ESP32

  // Envía el mensaje por puerto serial al ESP32
  esp32.write(message, (err) => {
    if (err) {
      console.error('Error al enviar al ESP32:', err.message);
      res.status(500).send('Error al enviar al ESP32');
    } else {
      console.log(`Enviado al ESP32: ${message}`);
      res.send('Tiempos enviados al nodo' + nodoID);
    }
  });
});


//Modificacion2 Ruta para recibir la configuración completa de bloques horarios desde el frontend
app.post('/send-schedule', (req, res) => {
  const { nodoID, blocks } = req.body; // Desestructura nodoID y array de bloques

  // Serializar los bloques a texto compactado para enviar por LoRa
  // Formato: b1:09:00-11:00,5,20;b2:11:00-13:00,10,10;... + ID
  const messageParts = blocks.map(block => {
    return `b${block.id}:${block.start}-${block.end},${block.on},${block.off}`;
  });

  const message = `schedule:${nodoID};${messageParts.join(';')}\n`;  // Mensaje completo

  // Envía la configuración al ESP32 (nodo maestro)
  esp32.write(message, (err) => {
    if (err) {
      console.error('Error al enviar configuración al maestro:', err.message);
      res.status(500).send('Error al enviar al maestro');
    } else {
      console.log(`Bloques enviados al maestro: ${message}`);
      res.send(`Configuración enviada al nodo ${nodoID}`);
    }
  });
});  

// Envía la configuración al ESP32 (nodo maestro)
app.post('/send-time', (req, res) => {
  const { hora } = req.body;  // Extrae la hora enviada
  const message = `time:${hora}\n`;  // Formato esperado por el nodo esclavo

  // Envía la hora por serial al ESP32
  esp32.write(message, (err) => {
    if (err) {
      console.error('Error al enviar hora al maestro:', err.message);
      res.status(500).send('Error al enviar hora');
    } else {
      console.log(`Hora enviada al maestro: ${message.trim()}`);
      res.send(`Hora actual enviada: ${hora}`);
    }
  });
});

// Inicia el servidor web en el puerto definido
app.listen(port, () => {
  console.log(`Servidor web escuchando en http://localhost:${port}`);
});
