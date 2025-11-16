# ESP8266 Honeypot con OLED  
Mini honeypot con pantalla para tu red de laboratorio

Este proyecto convierte una placa tipo **NodeMCU ESP8266 con pantalla OLED integrada** en un pequeño
honeypot: escucha en varios servicios falsos (HTTP, Telnet, FTP, etc.), cuenta quién los toca,
muestra estadísticas en la pantalla OLED y deja todo registrado por el puerto serie.

> Usar únicamente en redes propias / de laboratorio y con fines educativos.

---

## Hardware utilizado

- Placa tipo **NodeMCU ESP8266** con:
  - Chip USB–Serie **CH340** (`1a86:7523`)
  - Pantalla **OLED 0,96" I2C (SSD1306, 128x64)**
  - PCB similar al modelo **HW-364A**
- Cable USB
- PC con **Debian** (o derivado) + Arduino IDE

En esta placa la OLED va cableada internamente así:

- `SDA → GPIO14 (D5)`
- `SCL → GPIO12 (D6)`
- Dirección I2C típica: `0x3C`

---

## Qué hace este honeypot

- Levanta cuatro puertos falsos:
  - `80`   – HTTP
  - `23`   – Telnet
  - `21`   – FTP
  - `8080` – servicio alternativo
- Por cada conexión:
  - Incrementa un contador de hits totales
  - Incrementa un contador por puerto
  - Guarda la IP del último cliente
  - Envía una respuesta de texto diferente según el puerto
- La pantalla OLED muestra en tiempo real:
  - `Honeypot Arriba...`
  - `Hits: <total>`
  - `Top port: <puerto más tocado>`
  - `Last: <última IP vista>`
  - `By Viernez13` (firma del autor)

Ejemplo de salida por el Monitor Serie:
<img width="1528" height="1528" alt="image" src="https://github.com/user-attachments/assets/23260405-7cd3-440e-b52b-2483c69131f0" />

```text
Conectado. IP: 192.168.1.25
Servidores honeypot iniciados:
 - HTTP  : 80
 - TELNET: 23
 - ALT   : 8080
 - FTP   : 21
[Hit] Port 80 desde 192.168.1.58:45402
[Hit] Port 23 desde 192.168.1.58:49164
[Hit] Port 23 desde 192.168.1.58:49182
...
```

---

## Preparación de Arduino IDE

### 1. Instalar soporte para ESP8266

1. Abrir **Arduino IDE**.
2. Ir a **Archivo → Preferencias**.
3. En “Gestor de URLs adicionales de tarjetas” añadir:

   ```text
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```

4. Ir a **Herramientas → Placa → Gestor de tarjetas…**
5. Buscar **“ESP8266 by ESP8266 Community”** e instalar.

---

### 2. Instalar librerías para la OLED

En **Programa → Incluir librería → Gestionar bibliotecas…**:

- Instalar **Adafruit SSD1306**
- Instalar **Adafruit GFX Library**

Con esto la pantalla SSD1306 queda lista para usarse.

---

### 3. Elegir la placa adecuada

En **Herramientas → Placa** seleccionar:

> `ESP8266 → NodeMCU 1.0 (ESP-12E Module)`

Este perfil funciona bien con las placas NodeMCU basadas en CH340.

---

## Detección del puerto serie en Debian

Con la placa conectada, ejecutar:

```bash
ls /dev/ttyUSB* /dev/ttyACM*
```

La salida típica será algo como:

```text
/dev/ttyUSB0
```

También se puede comprobar con:

```bash
dmesg | tail -n 20
```

La línea relevante suele ser similar a:

```text
ch341-uart converter now attached to ttyUSB0
```

En Arduino IDE ir a:

> **Herramientas → Puerto → `/dev/ttyUSB0`**

(ajustar según el nombre que aparezca en el sistema).

---

## Permisos para usar `/dev/ttyUSB0` en Debian

Para evitar problemas de permisos al subir el sketch:

```bash
sudo usermod -aG dialout TU_USUARIO
```

Reemplazar `TU_USUARIO` por el nombre de usuario de la sesión.  
Es necesario cerrar sesión y volver a entrar (o reiniciar) para que el cambio de grupo tenga efecto.

---

## Configuración de la OLED (I2C)

En esta placa concreta el OLED está en:

- `SDA = GPIO14`
- `SCL = GPIO12`

Por eso, en el código se inicializa el bus I2C así:

```cpp
// I2C: OLED en SDA=GPIO14 (D5), SCL=GPIO12 (D6)
Wire.begin(14, 12);

// Inicialización del display (dirección 0x3C)
if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("No se encuentra OLED SSD1306"));
    for (;;) ; // bucle infinito si falla
}
```

Si se cambia de módulo OLED y la pantalla no responde, es buena idea ejecutar un pequeño
“scanner I2C” para comprobar la dirección (`0x3C` / `0x3D`) y ajustar la llamada a `display.begin`.

---

## Configuración WiFi en el sketch

En el archivo principal hay un bloque similar a este:

```cpp
// ------------ CONFIGURACIÓN WIFI ------------
const char* ssid     = "TU_SSID_AQUI";
const char* password = "TU_PASSWORD_AQUI";
```

Sustituir `"TU_SSID_AQUI"` y `"TU_PASSWORD_AQUI"` por el nombre y contraseña de la red donde se va a usar el proyecto.  
No subir nunca credenciales reales a repositorios públicos.

---

## Carga del sketch

1. Abrir el archivo `.ino` con el código del proyecto.
2. Comprobar:
   - **Placa:** `NodeMCU 1.0 (ESP-12E Module)`
   - **Puerto:** el `/dev/ttyUSBX` detectado (por ejemplo `/dev/ttyUSB0`)
3. Pulsar el botón **Subir**.

Si la carga es correcta, en el Monitor Serie se verán los mensajes de conexión y, al
poco tiempo, los registros de hits en los distintos puertos.

En la pantalla OLED se verán, primero, los mensajes de arranque:

- `Booteando awaita...`
- `By Viernez13`

y luego la pantalla principal con:

- Estado del honeypot
- Número de hits
- Puerto más “popular”
- Última IP observada
- Firma final `By Viernez13`

---

## Personalización

### Mensajes de respuesta por puerto

En la función `handleServer(...)` se pueden cambiar los textos que se envían al cliente:

```cpp
if (port == 80) {
    client.print("HTTP/1.1 200 OK ...");
} else if (port == 23) {
    client.println("Telnet service closed ...");
} else {
    client.println("Hermano esta wea no prendió");
}
```

Es posible adaptar estos mensajes para que sean más serios o más sarcásticos, según el uso deseado.

### Firma en la pantalla

En `updateDisplay()` se añade la firma:

```cpp
display.setCursor(0, 52);
display.print("By Viernez13");
```

Se puede cambiar el texto o la posición si se quiere otra firma o distribución de la información.

### Nuevos puertos

Para añadir más puertos:

1. Ampliar el array `PortStat stats[]` con el nuevo puerto.
2. Declarar un nuevo `WiFiServer` con ese puerto.
3. Llamar a `handleServer(nuevoServer, NUEVO_PUERTO);` dentro del `loop()`.

---

## Solución de problemas

### La OLED no muestra nada

- Confirmar que el código usa `Wire.begin(14, 12);`.
- Verificar que la dirección en `display.begin` coincide con la que devuelve un scanner I2C
  (normalmente `0x3C` en esta placa).
- Asegurarse de que las librerías **Adafruit SSD1306** y **Adafruit GFX Library** están correctamente instaladas.

### Arduino IDE no ve el puerto serie

- Comprobar con `ls /dev/ttyUSB*` que el dispositivo existe.
- Revisar que el usuario pertenece al grupo `dialout` y que se ha iniciado sesión de nuevo tras
  ejecutar `usermod`.

### El dispositivo no se conecta a la red

- Revisar que SSID y contraseña en el sketch son correctos.
- Verificar que el punto de acceso permite la conexión del ESP8266.

---

## Notas de uso responsable

Este proyecto está pensado como herramienta educativa y para pruebas en entornos controlados:

- Úsalo para entender el comportamiento de escaneos y conexiones dentro de tu propia red.
- No lo utilices para registrar tráfico o actividades de terceros sin autorización.
- Mantén siempre el enfoque en la experimentación segura y legal.

Con eso, el ESP8266 queda convertido en un pequeño honeypot con pantalla OLED para tu laboratorio de red.
