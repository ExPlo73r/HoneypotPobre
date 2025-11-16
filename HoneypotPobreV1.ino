#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ------------ CONFIGURACIÓN WIFI ------------
const char* ssid     = "TuRed";
const char* password = "TuClave";

// ------------ CONFIGURACIÓN OLED ------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1       // Compartido en la mayoría de módulos I2C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ------------ SERVIDORES / PUERTOS ------------
WiFiServer serverHTTP(80);
WiFiServer serverTELNET(23);
WiFiServer serverALT(8080);
WiFiServer serverFTP(21);

struct PortStat {
  uint16_t port;
  uint32_t hits;
};

PortStat stats[] = {
  {80,    0},
  {23,    0},
  {8080,  0},
  {21,    0}
};
const uint8_t NUM_PORTS = sizeof(stats) / sizeof(stats[0]);

// ------------ ESTADO DE MÉTRICAS ------------
uint32_t totalHits = 0;
IPAddress lastHitIP(0, 0, 0, 0);
uint16_t lastHitPort = 0;
bool hasHit = false;
unsigned long lastDisplayUpdate = 0;
const unsigned long DISPLAY_INTERVAL = 1000; // ms

// ------------ FUNCIONES AUXILIARES ------------
void updatePortStats(uint16_t port) {
  for (uint8_t i = 0; i < NUM_PORTS; i++) {
    if (stats[i].port == port) {
      stats[i].hits++;
      return;
    }
  }
}

uint16_t getTopPort() {
  uint16_t topPort = 0;
  uint32_t topHits = 0;

  for (uint8_t i = 0; i < NUM_PORTS; i++) {
    if (stats[i].hits > topHits) {
      topHits = stats[i].hits;
      topPort = stats[i].port;
    }
  }
  return topPort;
}

void handleServer(WiFiServer &srv, uint16_t port) {
  WiFiClient client = srv.available();
  if (!client) return;

  IPAddress rip = client.remoteIP();
  uint16_t rport = client.remotePort();

  totalHits++;
  updatePortStats(port);
  lastHitIP = rip;
  lastHitPort = port;
  hasHit = true;

  // Logging por Serial
  Serial.print("[Hit] Port ");
  Serial.print(port);
  Serial.print(" desde ");
  Serial.print(rip);
  Serial.print(":");
  Serial.println(rport);

  // Respuesta simple según el puerto
  if (port == 80) {
    client.print(
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: text/plain\r\n"
      "Connection: close\r\n\r\n"
      "Coff Coff. ¿Se te perdio algo? \r\n"
    );
  } else if (port == 23) {
    client.println("Telnet service closed Chupala.");
  } else {
    client.println("Hermano esta wea no prendio");
  }

  delay(10);
  client.stop();
}

// ------------ DISPLAY (AQUÍ VA LA FIRMA) ------------
void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Línea 1: estado general
  display.setCursor(0, 0);
  display.println("Honeypot Arriba...");

  // Línea 2: total de hits
  display.setCursor(0, 16);
  display.print("Hits: ");
  display.println(totalHits);

  // Línea 3: puerto con más hits
  display.setCursor(0, 28);
  display.print("Top port: ");
  uint16_t topPort = getTopPort();
  if (topPort == 0) {
    display.println("--");
  } else {
    display.println(topPort);
  }

  // Línea 4: última IP
  display.setCursor(0, 40);
  display.print("Last: ");
  if (!hasHit) {
    display.println("--");
  } else {
    char ipStr[16];
    snprintf(ipStr, sizeof(ipStr), "%d.%d.%d.%d",
             lastHitIP[0], lastHitIP[1], lastHitIP[2], lastHitIP[3]);
    display.println(ipStr);
  }

  // Línea 5: firma
  display.setCursor(0, 52);
  display.print("Viernez13 Cybeersecurity");

  display.display();
}

// ------------ SETUP ------------
void setup() {
  Serial.begin(115200);
  delay(200);

  // I2C: en la HW-364A el OLED va en SDA=GPIO14 (D5), SCL=GPIO12 (D6)
  Wire.begin(14, 12);

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // si no funciona, probar 0x3D
    Serial.println(F("No se encuentra OLED SSD1306"));
    for (;;) ; // Bucle infinito si falla
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Booteando awaita...");
  display.setCursor(0, 16);
  display.println("By Viernez13 for CybeerSecurity");
  display.display();

  // WIFI
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  display.setCursor(0, 32);
  display.println("Conectando WiFi...");
  display.display();

  Serial.print("Conectando a ");
  Serial.println(ssid);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Conectado. IP: ");
  Serial.println(WiFi.localIP());

  display.setCursor(0, 44);
  display.print("IP: ");
  display.println(WiFi.localIP());
  display.display();
  delay(1500);

  // Iniciar servidores
  serverHTTP.begin();
  serverTELNET.begin();
  serverALT.begin();
  serverFTP.begin();

  Serial.println("Servidores honeypot iniciados:");
  Serial.println(" - HTTP  : 80");
  Serial.println(" - TELNET: 23");
  Serial.println(" - ALT   : 8080");
  Serial.println(" - FTP   : 21");

  updateDisplay();
}

// ------------ LOOP PRINCIPAL ------------
void loop() {
  // Revisar conexiones entrantes en cada puerto
  handleServer(serverHTTP, 80);
  handleServer(serverTELNET, 23);
  handleServer(serverALT, 8080);
  handleServer(serverFTP, 21);

  // Refrescar OLED cada DISPLAY_INTERVAL
  unsigned long now = millis();
  if (now - lastDisplayUpdate > DISPLAY_INTERVAL) {
    updateDisplay();
    lastDisplayUpdate = now;
  }
}
