PWM Senoidal en ESP32 (GPIO25)

Genera una señal PWM en el GPIO25 del ESP32 cuyo ciclo de trabajo sigue una senoide. La modulación está construida de forma que:

0 % de duty = mínimo de la onda

50 % de duty = cero de la onda

100 % de duty = máximo de la onda

Con un filtro RC pasa-bajas externo, el promedio de la PWM se convierte en una senoide analógica (con offset ~1.65 V si alimentas a 3.3 V).

Características

Plataforma: ESP32 DevKit V1 (Arduino Core 2.x y 3.x soportados).

Salida PWM: GPIO25 usando LEDC (portadora por defecto 20 kHz, 10 bits).

Onda objetivo: seno a 60 Hz por defecto, generada a partir de una tabla de 256 puntos.

Temporización estable: timer hardware que actualiza el duty a SINE_POINTS × SINE_FREQ_HZ.

Código con detección de versión del core para mantener compatibilidad con APIs 2.x y 3.x.

Mensajes de diagnóstico por Serial (115200 bps).

¿Cómo funciona?

Se construye una tabla con 256 muestras de sin(θ) y se mapea a duty:

duty
=
sin
⁡
(
𝜃
)
+
1
2
∈
[
0
,
1
]
  
⇒
  
0
%
…
50
%
…
100
%
duty=
2
sin(θ)+1
	​

∈[0,1]⇒0%…50%…100%

Un timer interrumpe a la frecuencia de muestreo Fs = SINE_POINTS × SINE_FREQ_HZ (p. ej., 256×60 = 15.36 kHz).

En cada interrupción, se escribe el duty correspondiente en LEDC (portadora PWM 20 kHz), produciendo una PWM cuyo promedio temporal es una senoide.

Con un filtro RC externo, se elimina la componente de alta frecuencia (portadora) y se obtiene una senoide suavizada.

Conexión rápida

Salida: GPIO25 → (opcional) Filtro RC → osciloscopio/entrada analógica de alta impedancia.

Alimentación: 5 V al DevKit (USB). PWM sale a 3.3 V lógicos.

Filtro RC recomendado (visualización limpia)

Dos etapas RC en cascada con 
𝑓
𝑐
≈
1
 
kHz
f
c
	​

≈1kHz:

R1 = 3.3 kΩ, C1 = 47 nF

R2 = 3.3 kΩ, C2 = 47 nF

Esto atenúa la portadora de 20 kHz ~−52 dB (2 polos) y apenas afecta 60 Hz.

Si solo quieres una etapa: R=3.3 kΩ, C=47 nF (−26 dB a 20 kHz).
Si aún ves rizado: baja 
𝑓
𝑐
f
c
	​

 (p. ej., C=100 nF) o agrega una tercera etapa.

Nota sobre el offset

El promedio de la PWM se centra en ~1.65 V (mitad de 3.3 V). Para visualizar centrado en 0 V en el osciloscopio, usa acoplamiento AC o añade un capacitor en serie (p. ej., 10 µF) con R=100 kΩ a GND (HPF ~0.16 Hz).

Parámetros principales (ajustables en el código)

PWM_PIN = 25 — pin de salida.

PWM_FREQ_HZ = 20000 — frecuencia de la portadora PWM (15–25 kHz recomendado).

PWM_RES_BITS = 10 — resolución (8–12 bits típicamente).

SINE_POINTS = 256 — muestras por ciclo de seno (512 para más suavidad).

SINE_FREQ_HZ = 60.0 — frecuencia de la onda seno.

Cambiar SINE_FREQ_HZ modifica la frecuencia de la senoide. La ISR ajusta la tasa de actualización automáticamente a SINE_POINTS × SINE_FREQ_HZ.

Salida por Serial

Al iniciar, el programa imprime:

Frecuencia y resolución PWM.

Frecuencia de la senoide y número de puntos.

Frecuencia de muestreo efectiva del timer.

Compatibilidad con Arduino Core 2.x y 3.x

LEDC

Core 3.x: ledcAttach(pin, freq, bits) + ledcWrite(pin, duty)

Core 2.x: ledcSetup(ch, freq, bits) + ledcAttachPin(pin, ch) + ledcWrite(ch, duty)

Timers

Core 3.x: timerBegin(base_freq_hz) + timerAlarm(timer, value, autoreload, reload_count) + timerStart()

Core 2.x: timerBegin(id, divider, countUp) + timerAlarmWrite() + timerAlarmEnable()

El código usa #if ESP_ARDUINO_VERSION_MAJOR para seleccionar la API correcta según tu instalación.

Limitaciones

La salida es PWM filtrada, no un DAC perfecto. Para aplicaciones de audio/precisión, considera un DAC dedicado o el DAC interno del ESP32 (GPIO25/26) si tu caso lo permite (resolución/linealidad limitadas).

Con alimentación single-supply (3.3 V), la senoide “real” no puede ser negativa. El centrado en 0 V sólo es de visualización (acoplamiento AC) o usando referencia virtual.

Solución de problemas

No compila en Core 3.x: asegúrate de tener ESP32 core ≥ 3.0.0. Este repo incluye ambas rutas de compilación; si ves errores de “muy pocos argumentos” en timerAlarm, probablemente tenías la firma antigua (borra caché y vuelve a compilar).

Mucho rizado en la salida: baja 
𝑓
𝑐
f
c
	​

 del filtro, añade más polos, o sube la portadora PWM si tu resolución lo permite.

Onda “aplastada” al cargar: añade buffer con op-amp rail-to-rail (p. ej., MCP6002/TLV9062).# Electronica-Potencia
software para un convertidor de CD-CA
