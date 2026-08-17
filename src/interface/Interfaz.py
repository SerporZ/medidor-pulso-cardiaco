import paho.mqtt.client as mqtt
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import csv
from datetime import datetime
import collections

# --- 1. CONFIGURACIÓN MQTT ---
BROKER = "localhost"
PUERTO = 1883
TOPIC_BPM = "proyecto/medidor/bpm"
TOPIC_ONDA = "proyecto/medidor/onda"

# --- 2. MEMORIA DE LA APLICACIÓN ---
# Usamos 'deque' para mantener siempre los últimos 100 puntos de la onda en pantalla
y_data = collections.deque([0]*100, maxlen=100)
bpm_actual = 0
estado_salud = "Esperando lectura..."

# Creamos/Abrimos el archivo de registro médico (Historial de todo instante)
archivo_historial = "registro_pacientes.csv"
with open(archivo_historial, mode='a', newline='') as file:
    writer = csv.writer(file)
    writer.writerow(["Fecha_Hora", "BPM", "Diagnostico_Automatico"]) # Encabezados

def evaluar_salud(bpm):
    if bpm < 60: return "⚠️ Bradicardia (Bajo)"
    elif bpm > 100: return "⚠️ Taquicardia (Alto)"
    else: return "✅ Saludable"

# --- 3. RECEPCIÓN DE DATOS POR MQTT (VERSIÓN MEJORADA) ---
def on_message(client, userdata, msg):
    global bpm_actual, estado_salud
    topic = msg.topic
    mensaje = msg.payload.decode("utf-8")

    try:
        if topic == TOPIC_ONDA:
            y_data.append(int(mensaje))
            
        elif topic == TOPIC_BPM:
            bpm_actual = int(mensaje)
            estado_salud = evaluar_salud(bpm_actual)
            hora_actual = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            
            # Intento de guardado con manejo de errores de Windows
            try:
                # Usamos encoding utf-8 para no tener problemas con los caracteres en Excel
                with open(archivo_historial, mode='a', newline='', encoding='utf-8') as file:
                    writer = csv.writer(file)
                    writer.writerow([hora_actual, bpm_actual, estado_salud])
                    
                print(f"[{hora_actual}] Guardado: {bpm_actual} BPM -> {estado_salud}")
                
            except PermissionError:
                # Si Excel tiene abierto el archivo, lanzamos esta alerta en lugar de fallar
                print("⚠️ ERROR DE SISTEMA: Por favor, cierra el archivo en Excel. Está bloqueando la escritura.")
                
    except ValueError:
        pass
# Inicializamos el cliente MQTT
cliente = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
cliente.on_message = on_message
cliente.connect(BROKER, PUERTO, 60)
cliente.subscribe([(TOPIC_BPM, 0), (TOPIC_ONDA, 0)])

# Usamos loop_start() para que MQTT corra en segundo plano mientras se dibuja la gráfica
cliente.loop_start() 

# --- 4. CONFIGURACIÓN DE LA INTERFAZ GRÁFICA (MATPLOTLIB) ---
fig, ax = plt.subplots(figsize=(10, 5))
fig.canvas.manager.set_window_title('Monitor IoT - Ritmo Cardíaco')

line, = ax.plot(y_data, color='red', linewidth=2.5) # Línea roja de hospital

# Ajustamos los límites del eje Y según los valores reales de tu sensor (aprox. 100 a 400)
ax.set_ylim(0, 450) 
ax.set_title("Telemedicina: Fotopletismografía en Tiempo Real", fontsize=14, fontweight='bold')
ax.set_ylabel("Amplitud de Señal (ADC)", fontsize=11)
ax.set_xticks([]) # Ocultamos los números del eje X para que parezca osciloscopio continuo
ax.grid(True, linestyle='--', alpha=0.6)

# Caja de texto en la gráfica para mostrar el BPM en vivo
texto_pantalla = ax.text(0.02, 0.88, "", transform=ax.transAxes, fontsize=14, 
                         bbox=dict(facecolor='black', alpha=0.8, edgecolor='white', boxstyle='round,pad=0.5'),
                         color='lime', fontweight='bold')

# Función que anima y refresca la gráfica cada 40 milisegundos
def actualizar_grafica(frame):
    line.set_ydata(y_data)
    texto_pantalla.set_text(f" BPM: {bpm_actual} \n ESTADO: {estado_salud}")
    return line, texto_pantalla

ani = animation.FuncAnimation(fig, actualizar_grafica, interval=40, blit=True)

print("Iniciando App Médica... Por favor, pon tu dedo en el sensor.")
plt.show() # Abre la ventana de la App

# Al cerrar la ventana con la 'X', detenemos el proceso de forma segura
cliente.loop_stop()
cliente.disconnect()
print("Aplicación cerrada. Puedes revisar tu archivo 'registro_pacientes.csv' para ver el historial.")