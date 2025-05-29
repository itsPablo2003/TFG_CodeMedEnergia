# TFG_CodeMedEnergia
**Monitorización y control remoto del consumo energético**  
Trabajo Fin de Grado · Pablo De Unamuno Tomás · ETSIT-UPM

---

## Resumen del proyecto
Este repositorio contiene el firmware, esquemas y documentación del sistema IoT desarrollado para medir el consumo energético de forma remota y controlar la conexión de una toma de corriente.

- Medición de corriente con sensor no invasivo SCT-013-030
- Cálculo RMS con media móvil para limpieza de señal
- Control de un relé SSR (Fotek) para corte de corriente
- Conectividad WiFi mediante ESP-01S (comandos AT)
- Envío de datos a **ThingSpeak** cada 60 segundos
- Servidor web básico para control del relé desde navegador local
- Compatible con Arduino Pro Micro (ATmega32U4)

>  Web de control: [https://itspablo2003.github.io/TFG_CodeMedEnergia/](https://itspablo2003.github.io/TFG_CodeMedEnergia/)

---

## Estructura del repositorio

```
TFG_CodeMedEnergia/
├── firmware/         Código Arduino (.ino)
├── index.html        Página para controlador Relé
└── README.md
```

---

## Hardware utilizado

| Componente             | Cantidad | Descripción                        |
|------------------------|----------|------------------------------------|
| Arduino Pro Micro      | 1        | MCU ATmega32U4 5V/16MHz            |
| Sensor SCT-013-030     | 1        | Sensor de corriente no invasivo    |
| Módulo WiFi ESP-01S    | 1        | Firmware AT, 115200 bps            |
| Relé SSR Fotek SSR-25DA| 1        | Corte monofásico 220V AC           |
| Fuente de 9V DC        | 1        | Alimentación externa de batería    |

---

## Cómo empezar

### 1. Requisitos
- Arduino IDE ≥ 1.8.x
- Drivers de SparkFun Pro Micro
- Cuenta en ThingSpeak (https://thingspeak.com/)
- Acceso a red WiFi 2.4GHz

### 2. Configura ThingSpeak
- Crea un **canal** nuevo llamado `TFG_Consumo`
- Añade campos: `Potencia`, `Rele`
- Copia tu **Write API Key**

### 3. Carga el código
1. Abre el archivo `firmware/TFG_CodeMedEnergia.ino`
2. Edita las siguientes líneas:
   ```
   const char* ssid = "SSID";
   const char* password = "PASSWORD";
   const char* apiKey = "THINGSPEAK_API_KEY";
   ```
3. Selecciona la placa **SparkFun Pro Micro 5V/16MHz**
4. Sube el sketch y conecta el hardware

---

## Control web

Desde un navegador, accede a la Web y usa los siguientes endpoints:

| Acción           | URL local                                                                     |
|------------------|-------------------------------------------------------------------------------|
| Encender relé    | `https://api.thingspeak.com/update?api_key=[THINGSPEAK_API_KEY]&field2=1`     |
| Apagar relé      | `https://api.thingspeak.com/update?api_key=[THINGSPEAK_API_KEY]&field2=0`     |

---

## Publicación en ThingSpeak

Cada 60 segundos el sistema envía:

- Potencia estimada (W)
- Encendido o Apagado de Rele

Esto se puede visualizar en tiempo real en el panel de control de ThingSpeak.

---

## Mejoras futuras

- Paso a ESP32 + MQTT
- Medición de tensión real para cálculo de potencia activa

---

## Licencia

Distribuido bajo licencia MIT.

---

## Agradecimientos

- María Calderón (Tutora TFG)
- Comunidad Arduino
- Plataforma ThingSpeak
- SparkFun / SCT-013 documentación
