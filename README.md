# Mini Invernadero Inteligente con ESP32

Sistema local de monitoreo ambiental mediante ESP32, sensores y un panel web adaptable a dispositivos móviles.

> **Estado del proyecto:** prototipo académico. Las fotografías y el video indicados en este documento son espacios reservados para evidencias reales del equipo y deben incorporarse al repositorio antes de su entrega.

---

## 1. Integrantes

| Nombre completo | Programa / grupo | Responsabilidad en el proyecto |
|---|---|---|
| __________________________ | __________________ | __________________________ |
| __________________________ | __________________ | __________________________ |
| __________________________ | __________________ | __________________________ |

## 2. Descripción general

Este proyecto implementa un mini invernadero con monitoreo local. El ESP32 adquiere la temperatura y humedad del ambiente mediante un DHT22, la lectura analógica de un sensor de humedad de suelo FC-28 y la señal de una fotoresistencia (LDR) con un divisor de tensión de 10 kΩ. Los datos se visualizan en una página web alojada directamente en el microcontrolador.

El ESP32 crea su propia red Wi-Fi en modo punto de acceso (Access Point, AP); por tanto, un celular o computador puede conectarse al sistema sin router ni acceso a Internet. El navegador consulta los datos periódicamente por HTTP y actualiza las cuatro tarjetas del panel.

**Alcance implementado:** monitoreo y visualización. Esta versión no usa MQTT, no almacena históricos y no automatiza el riego, la iluminación o la ventilación.

## 3. Objetivos

- Integrar sensores ambientales y analógicos con un ESP32.
- Obtener lecturas periódicas y mostrarlas por el monitor serial.
- Configurar el ESP32 como punto de acceso Wi-Fi y servidor HTTP.
- Publicar mediciones en formato JSON mediante el endpoint `/data`.
- Crear una interfaz web adaptable a teléfonos y computadores.
- Documentar el montaje, las pruebas y las limitaciones del prototipo.

## 4. Componentes y herramientas

| Elemento | Cantidad | Función |
|---|---|---|
| ESP32 clásico / DevKit | 1 | Lectura de sensores, punto de acceso y servidor web |
| DHT22 | 1 | Medición de temperatura y humedad relativa del aire |
| Sensor de humedad de suelo FC-28 con módulo de salida analógica | 1 | Lectura relativa de humedad del sustrato |
| Fotoresistencia (LDR) | 1 | Detección relativa de iluminación |
| Resistencia de 10 kΩ | 1 | Divisor de tensión para el LDR |
| Protoboard, cables y cable USB de datos | Según montaje | Interconexión, alimentación y programación |
| Computador con Arduino IDE | 1 | Compilación, carga y monitor serial |
| Teléfono o computador con Wi-Fi | 1 | Visualización del panel web |

> Para un DHT22 de cuatro patas, sin placa adaptadora, añadir una resistencia pull-up de aproximadamente 4,7 kΩ a 10 kΩ entre `VCC` y `DATA`. Los módulos DHT22 de tres pines suelen incorporarla; verificar el modelo.

## 5. Conexiones eléctricas

### 5.1. Tabla de conexiones

| Dispositivo | Terminal | Conexión ESP32 | Observaciones |
|---|---|---|---|
| DHT22 | VCC | 3V3 | Alimentación a 3,3 V |
| DHT22 | DATA | GPIO21 | Señal digital del DHT22 |
| DHT22 | GND | GND | Tierra compartida |
| FC-28 (módulo) | VCC | 3V3 | Evitar que AO aplique más de 3,3 V al ESP32 |
| FC-28 (módulo) | AO | GPIO34 | Entrada analógica ADC1 |
| FC-28 (módulo) | GND | GND | Tierra compartida |
| FC-28 (módulo) | DO | Sin conectar | No se usa la salida digital |
| LDR | Extremo 1 | 3V3 | Parte superior del divisor de tensión |
| LDR | Extremo 2 | GPIO35 y resistencia 10 kΩ | Nodo de medición |
| Resistencia 10 kΩ | Extremo restante | GND | Parte inferior del divisor |

**Diagrama textual del LDR:**

```text
3V3 ---- [ LDR ] ----+---- GPIO35 (ADC1)
                     |
                   [10 kΩ]
                     |
                    GND
```

Con esta orientación, normalmente más luz → menor resistencia del LDR → mayor tensión en GPIO35 → mayor lectura ADC. Si se invierten LDR y resistencia, la tendencia será contraria.

**Precauciones:** utilizar GND común; no introducir 5 V directamente en GPIO21, GPIO34 ni GPIO35. GPIO34 y GPIO35 son entradas sin capacidad de salida, adecuadas para estas señales analógicas. Comprobar la distribución real de pines del DevKit usado.

## 6. Arquitectura de funcionamiento

```text
DHT22 (GPIO21) ------------┐
FC-28 AO (GPIO34) ---------+--> ESP32 --> Servidor HTTP :80 --> Navegador
LDR + 10 kΩ (GPIO35) ------┘       |             |                |
                                  |             +--> GET /       |
                                  |             +--> GET /data <-+
                                  |
                             Wi-Fi AP local
                           Invernadero-ESP32
```

1. El ESP32 inicializa el puerto serial, el ADC y el DHT22.
2. Activa una red Wi-Fi local en modo AP.
3. Inicia el servidor HTTP en el puerto 80.
4. Lee los sensores aproximadamente cada 2 segundos y conserva los últimos valores en memoria.
5. Cuando un navegador accede a `/`, entrega el HTML del dashboard.
6. El JavaScript del dashboard consulta `/data` aproximadamente cada 2 segundos, interpreta la respuesta JSON y renueva los valores mostrados.
7. El monitor serial presenta las mediciones y repite la dirección IP del AP cada 10 segundos en la versión final corregida.

El servidor web utiliza `server.handleClient()` en `loop()` para atender solicitudes entrantes. Las variables `millis()` permiten programar tareas periódicas sin bloquear continuamente el servidor con retrasos largos.

## 7. Configuración de red y API HTTP

| Parámetro | Configuración |
|---|---|
| Modo Wi-Fi | `WIFI_AP` (punto de acceso) |
| SSID | `Invernadero-ESP32` |
| Contraseña de laboratorio | `invernadero123` |
| IP habitual predeterminada del AP | `192.168.4.1` |
| Puerto HTTP | `80` |
| Panel web | http://192.168.4.1/ |
| Endpoint de datos | http://192.168.4.1/data |

La IP indicada es la predeterminada habitual del AP del ESP32; confirmar siempre la IP efectiva en el monitor serial, ya que puede modificarse por configuración.

**Ejemplo de respuesta de `/data`:**

```json
{
  "temperature": 28.5,
  "humidity": 70.2,
  "LDR": 1500,
  "soil": 2200
}
```

Los valores son ilustrativos. `temperature` está en °C y `humidity` en porcentaje de humedad relativa; `LDR` y `soil` son lecturas ADC sin calibración. Si falla la lectura del DHT22, sus campos correspondientes se devuelven como `null`.

> **Nota de implementación:** en la construcción manual del JSON, la clave `LDR` debe ir precedida de una coma:
>
> ```cpp
> json += ",\"LDR\":";
> ```
>
> De lo contrario, el JSON es inválido y el navegador no puede actualizar el panel.

## 8. Software, dependencias y preparación

Entorno: Arduino IDE con soporte de placas ESP32 de Espressif instalado. Seleccionar la placa correspondiente al hardware (por ejemplo, `ESP32 Dev Module` para un DevKit compatible).

**Bibliotecas:**

- `WiFi.h`: incluida con el núcleo Arduino para ESP32.
- `WebServer.h`: incluida con el núcleo Arduino para ESP32.
- DHT sensor library: biblioteca de Adafruit (instalar desde el gestor de bibliotecas).
- Adafruit Unified Sensor: dependencia utilizada por la biblioteca DHT de Adafruit.

El firmware principal se ubica en `invernadero_esp32/invernadero_esp32.ino` dentro de la estructura sugerida. Si el archivo real tiene otro nombre, ajustar la referencia del repositorio.

### 8.1. Cargar el firmware

1. Conectar el ESP32 al computador con un cable USB de datos.
2. Abrir Arduino IDE e instalar el soporte de placas `esp32 by Espressif Systems` si aún no está instalado.
3. Instalar las dos bibliotecas de Adafruit mencionadas.
4. Abrir el archivo `invernadero_esp32.ino`.
5. Comprobar las definiciones de pines: `DHT_PIN 21`, `SOIL_PIN 34` y `LDR_PIN 35`.
6. Seleccionar la placa y el puerto serie correctos en el IDE.
7. Compilar y cargar el programa. Si el DevKit lo requiere, mantener presionado BOOT durante el inicio de la carga y soltarlo cuando comience a escribir.
8. Abrir el Monitor serial a 115200 baudios. Si no se ve el arranque, pulsar EN/RESET en la placa.
9. Verificar que aparezcan los mensajes de creación del AP, inicio del servidor e IP.

### 8.2. Conectar el celular y abrir el dashboard

1. En el teléfono, buscar la red `Invernadero-ESP32`.
2. Introducir la contraseña `invernadero123`.
3. Permanecer conectado aunque el teléfono advierta “Sin Internet”; es normal porque se trata de una red local.
4. Abrir el navegador y escribir http://192.168.4.1/ (o la IP que reporte el monitor serial).
5. Verificar las cuatro tarjetas de temperatura, humedad ambiental, humedad de suelo e iluminación.
6. Para comprobar la API directamente, abrir http://192.168.4.1/data desde un dispositivo conectado a la misma red.

El celular debe usar la red del ESP32 para acceder al panel. Si conmuta automáticamente a datos móviles o a otra red Wi-Fi, la dirección local puede dejar de responder.

## 9. Interpretación de los sensores

| Variable | Fuente | Significado | Limitación |
|---|---|---|---|
| `temperature` | DHT22 / GPIO21 | Temperatura ambiente en °C | Puede ser `null` cuando falla la lectura |
| `humidity` | DHT22 / GPIO21 | Humedad relativa del aire, en % | No indica humedad de suelo |
| `soil` | FC-28 AO / GPIO34 | Lectura analógica de 0 a 4095 | No es aún un porcentaje de humedad |
| `LDR` | Divisor LDR/10 kΩ / GPIO35 | Lectura analógica de 0 a 4095 | No equivale directamente a lux |

El ESP32 utiliza una resolución de ADC de 12 bits en este proyecto (`analogReadResolution(12)`). La atenuación configurada (`ADC_11db`) modifica el rango de tensión medible; no debe interpretarse 4095 como una equivalencia universal y exacta con 3,3 V.

**FC-28:** para representar humedad relativa del suelo en porcentajes se requieren al menos dos lecturas experimentales de referencia (sustrato seco y húmedo) y una calibración. El FC-28 resistivo también puede presentar corrosión de electrodos con uso prolongado.

**LDR:** para el divisor utilizado, la lectura tiende a aumentar con la iluminación. Se trata de una medida relativa; la conversión a lux requiere caracterización del sensor y condiciones de medición adecuadas.

## 10. Evidencias del prototipo

Incorporar al repositorio exactamente dos fotografías y un video reales, con los nombres indicados. No utilizar imágenes ajenas o simuladas como si fueran pruebas ejecutadas.

### Evidencia 1 — Fotografía del montaje físico

**Qué debe mostrar:** ESP32, sensores, conexiones y alimentación. Procurar que el cableado del GPIO21, GPIO34, GPIO35, 3V3 y GND sea identificable.

### Evidencia 2 — Fotografía del dashboard en funcionamiento

**Qué debe mostrar:** las cuatro mediciones visibles y la interfaz web funcionando en un teléfono o computador. Si es posible, conservar visible la dirección local utilizada.

### Evidencia 3 — Video de funcionamiento

[Ver video de prueba del mini invernadero (MP4)](evidencias/video_funcionamiento.mp4)

**Qué debe mostrar:** encendido, mensajes del monitor serial (incluida la IP), conexión a la red `Invernadero-ESP32`, apertura de la página, actualización de datos y al menos una prueba observable de respuesta del LDR (iluminar/tapar) o del FC-28 (variar las condiciones de medición). Explicar verbalmente o con subtítulos qué mide cada sensor.

Los enlaces de esta sección solo funcionarán cuando se agreguen los tres archivos reales a la carpeta `evidencias/` del repositorio.

## 11. Protocolo de pruebas y validación

| Prueba | Procedimiento | Resultado esperado |
|---|---|---|
| Arranque del AP | Reiniciar ESP32 y abrir monitor serial | Se indica que la red fue creada y se muestra su IP |
| Repetición de la IP | Esperar al menos 10 segundos con el firmware corregido | La IP vuelve a mostrarse por serial |
| Lectura del DHT22 | Observar dos ciclos de medición | Temperatura y humedad numéricas, sin `nan` persistente |
| Respuesta del LDR | Iluminar y cubrir el sensor | Cambios apreciables en la lectura LDR |
| Respuesta del FC-28 | Comparar lecturas en condiciones secas y húmedas controladas | Cambios de `soil` coherentes con la calibración experimental |
| Conectividad local | Conectar celular al AP | El teléfono permanece en la red local |
| Panel HTTP | Abrir `/` | Aparecen las cuatro tarjetas del dashboard |
| API JSON | Abrir `/data` | JSON válido con las cuatro claves esperadas |
| Actualización | Observar panel durante varios ciclos | Nuevos datos aproximadamente cada 2 segundos |

Registrar las observaciones y aportar las evidencias correspondientes. No afirmar que una prueba fue aprobada si aún no se ha realizado.

## 12. Problemas frecuentes y soluciones

| Problema | Posible causa | Verificación / solución |
|---|---|---|
| No aparece la IP en el monitor serial | Se imprimió durante el arranque; monitor abierto tarde | Configurar 115200 baudios, pulsar EN/RESET; la versión corregida también la imprime cada 10 s |
| La red Wi-Fi no aparece | Fallo al iniciar AP o alimentación inestable | Revisar el mensaje de `WiFi.softAP(...)`, reiniciar y comprobar alimentación |
| El navegador no abre la página | Celular conectado a otra red o utilizando datos móviles | Conectar al SSID del ESP32 y usar `http://` con la IP informada |
| El panel indica “Sin comunicacion con ESP32” | Solicitud `/data` fallida o JSON incorrecto | Abrir `/data` directamente y verificar la coma antes de `"LDR"` |
| El DHT22 muestra `nan` | Pin equivocado, alimentación, cableado o falta de pull-up | Verificar GPIO21, tipo DHT22, 3V3/GND y resistencia según módulo |
| El LDR no cambia | Divisor mal conectado o entrada flotante | Verificar LDR–GPIO35–10 kΩ–GND e iluminar/tapar |
| El FC-28 no cambia | Se conectó DO en lugar de AO o hay mala conexión | Comprobar AO→GPIO34 y probar condiciones secas/húmedas |
| Las lecturas ADC no parecen porcentajes | El programa entrega valores crudos | Calibrar antes de convertir a porcentaje o lux |

## 13. Estructura recomendada del repositorio

```text
mini-invernadero-esp32/
├── README.md
├── invernadero_esp32/
│   └── invernadero_esp32.ino
└── evidencias/
    ├── foto_montaje.jpg
    ├── foto_dashboard.jpg
    └── video_funcionamiento.mp4
```

La carpeta `evidencias/` debe contener los archivos originales aportados por el equipo. Conservar los mismos nombres para que las referencias del README funcionen correctamente en GitHub.

## 14. Seguridad, limitaciones y trabajo futuro

- La red del ESP32 utiliza una contraseña de demostración compartida. Cambiarla si el proyecto se despliega fuera del laboratorio; no presentar esta configuración como segura para producción.
- El servidor emplea HTTP sin cifrado, adecuado únicamente para la práctica en una red local controlada.
- No existe autenticación adicional de usuarios en el dashboard.
- No se implementó MQTT, acceso remoto por Internet, almacenamiento persistente ni alertas.
- No hay calibración a porcentaje de humedad de suelo ni a unidades físicas de iluminación.
- No se incluyen actuadores ni control automático; las decisiones de riego u otras acciones quedan fuera de esta versión.
- El FC-28 resistivo puede degradarse por corrosión; para funcionamiento prolongado conviene evaluar un sensor capacitivo.

**Mejoras propuestas:** calibración documentada, filtrado de lecturas, registro histórico, alarmas por umbrales, autenticación adecuada y eventual incorporación de actuadores con protecciones eléctricas.

## 15. Referencias técnicas

- [Arduino-ESP32: Wi-Fi API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/wifi.html)
- [Arduino-ESP32: ADC API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html)
- [Adafruit DHT Sensor Library](https://github.com/adafruit/DHT-sensor-library)
- [Adafruit Unified Sensor](https://github.com/adafruit/Adafruit_Sensor)
- [Arduino-ESP32: WebServer library](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/webserver.html)
