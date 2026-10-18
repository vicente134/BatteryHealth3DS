# 🔋 BatteryHealth3DS 

> **Creado y desarrollado por [ElVichoPo (@vicente134)](https://github.com/vicente134)**  
> ⭐ **¿Te gustó la aplicación o te ayudó a diagnosticar tu consola? ¡No olvides dejarle una Estrella (Star) de favorito arriba a la derecha en el repositorio para apoyar el proyecto!** ⭐

---

**BatteryHealth3DS** es una herramienta interactiva de diagnóstico de batería y analizador de resistencia interna (*Voltage Sag Test*) diseñada en C (`libctru` + `citro2d` + `NDSP`) para toda la familia **Nintendo 3DS y 2DS** (`Old 3DS`, `Old 3DS XL`, `Old 2DS`, `New 3DS`, `New 3DS XL` y **`New 2DS XL`**).

Cuenta con **Bati Bot**, un mini asistente interactivo animado en la pantalla táctil que mueve los ojos, parpadea, reproduce efectos de sonido (`"Woupp!"`), incluye música relajante de fondo (`ON / OFF`), detecta automáticamente el idioma de tu consola (`ES / EN / PT / FR`) y te da **10 consejos de emergencia y cuidado** para tu batería.

---

## 📷 Instalación Directa por Código QR (FBI)

Este código QR está alojado directamente en GitHub y apunta siempre a la última versión oficial del archivo `.cia`:

<p align="center">
  <img src="assets/qr_fbi_install.png" alt="QR de Instalación FBI - BatteryHealth3DS" width="260" />
</p>

### Pasos para instalar con la cámara en FBI:
1. Abre **FBI** en tu consola Nintendo 3DS / 2DS.
2. Entra en **`Remote Install`** -> **`Scan QR Code`**.
3. Apunta la cámara al código QR de arriba y presiona **`A`** para confirmar la instalación.
4. Sal al **Menú HOME**, abre el regalo de **BatteryHealth3DS** y disfruta junto a **Bati Bot**.

---

## 📥 Descarga Directa desde GitHub (Sin cámara)

Si prefieres no usar la cámara o quieres pasar los archivos manualmente a tu tarjeta MicroSD (o por FTPD / `3dslink`):

- 📦 **Descargar instalador `.CIA` (Menú HOME):**  
  👉 [**Descargar `BatteryHealth3DS.cia` (Enlace Directo)**](https://github.com/vicente134/BatteryHealth3DS/releases/latest/download/BatteryHealth3DS.cia)
- 🎮 **Descargar ejecutable `.3DSX` (The Homebrew Launcher):**  
  👉 [**Descargar `BatteryHealth3DS.3dsx` (Enlace Directo)**](https://github.com/vicente134/BatteryHealth3DS/releases/latest/download/BatteryHealth3DS.3dsx)
- 📂 **Ver todas las versiones y notas de lanzamiento:**  
  👉 [**Ir a la página de Releases en GitHub**](https://github.com/vicente134/BatteryHealth3DS/releases/latest)

---

## ✨ Características Principales

- 🔍 **Detección Exacta de Consola y Batería de Fábrica (`CFGU`):**
  - `Old 3DS` / `Old 2DS` / `New 2DS XL` $\rightarrow$ **CTR-003 (1300 mAh / 3.7V)**
  - `Old 3DS XL` / `New 3DS XL` $\rightarrow$ **SPR-003 (1750 mAh / 3.7V)**
  - `New 3DS` estándar $\rightarrow$ **KTR-003 (1400 mAh / 3.7V)**
- ⚡ **Telemetría Dual Precisa (`mcu::HWC` + `ptm:u`) y Gráfica en Vivo:**
  - Lee directamente el registro I2C del MCU para obtener el **voltaje real en milivoltios ($\text{mV}$)** y el porcentaje exacto de carga ($0\text{–}100\%$), dibujando una curva en vivo en la pantalla superior.
- 🔥 **Test de Resistencia Interna / Caída de Tensión (*Voltage Sag Test*):**
  - Mide el voltaje en reposo durante 1 segundo, activa el modo **Turbo 804 MHz** en consolas *New* y somete la CPU/FPU a 5 segundos de carga máxima para medir la caída $\Delta V = V_{\text{reposo}} - V_{\text{mínimo}}$:
    - **$\Delta V \le 60\text{ mV}$:** 🟢 **Salud Excelente** (baja resistencia interna).
    - **$60\text{ mV} < \Delta V \le 150\text{ mV}$:** 🟡 **Desgaste Moderado** (envejecimiento químico normal).
    - **$\Delta V > 150\text{ mV}$ (o caída de barras):** 🔴 **Salud Crítica** (recomendación amigable de reemplazo).
- 🤖 **Asistente Interactivo "Bati Bot":**
  - Tócalo con el lápiz táctil para escuchar su sonido `"Woupp!"` y recorrer sus **10 consejos de emergencia y cuidado**.
  - Música ambiental integrada con botón táctil **`♪ ON / OFF [Y]`**.
  - Soporte multilenguaje con detección automática del sistema (**Español, Inglés, Portugués y Francés**) y cambio rápido con **`SELECT`**.
  - Pantalla de despedida positiva al salir con **`START`**.

---

---

## ❤️ Créditos y Apoyo

- **Autor / Creador:** **ElVichoPo** ([@vicente134](https://github.com/vicente134))
- ⭐ **¡Si te gusta BatteryHealth3DS y Bati Bot, regálale una estrella (`Star`) al repositorio en GitHub!**
