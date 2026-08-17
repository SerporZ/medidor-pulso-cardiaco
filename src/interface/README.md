# Interfaz de monitoreo MQTT

Esta aplicación recibe los datos enviados por el ESP32 mediante MQTT,
muestra la señal del sensor en tiempo real y presenta el valor de BPM
recibido desde el broker Mosquitto.

El programa fue desarrollado como parte de un prototipo académico de
medición y visualización de señales asociadas al pulso cardiaco.

## Requisitos

- Python 3.
- Broker Mosquitto activo.
- ESP32 conectado a la misma red del broker.
- Librerías indicadas en `requirements.txt`.

## Instalación

Desde esta carpeta, ejecuta:

```bash
pip install -r requirements.txt
```

## Configuración

Antes de ejecutar el programa, revisa estas variables en el archivo `.py`:

```python
BROKER = "localhost"
PUERTO = 1883
TOPIC_BPM = "proyecto/medidor/bpm"
TOPIC_ONDA = "proyecto/medidor/onda"
```

Si Mosquitto está instalado en otro computador, cambia `BROKER` por la
dirección local del equipo que ejecuta el broker.

## Ejecución

Ejecuta:

```bash
python nombre_del_archivo.py
```

Reemplaza `nombre_del_archivo.py` por el nombre real del archivo de la
interfaz.

## Datos recibidos

La aplicación se suscribe a estos tópicos MQTT:

- `proyecto/medidor/bpm`: valor calculado de pulsaciones por minuto.
- `proyecto/medidor/onda`: valor de la señal del sensor.

## Registro local

El programa puede guardar las mediciones en un archivo CSV local. Ese
archivo está incluido en `.gitignore` para evitar publicar datos de
medición accidentalmente.

## Aviso

Este software es un prototipo académico. No es un dispositivo médico y no
debe utilizarse para diagnóstico, tratamiento ni decisiones clínicas.
