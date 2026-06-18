import requests
from mcp.server.fastmcp import FastMCP

mcp = FastMCP("Domotica_ESP32_Pro")

ESP32_IP = "http://192.168.0.223"  # IP correcta de tu ESP32 DEBES PONER LA IP DE TU ESP32 DESPUES DE HABER CARGADO EL CODIGO DEL ARDUINO

@mcp.tool()
def controlar_rele(pin: int, encender: bool) -> str:
    """Activa o desactiva uno de los 8 relés del sistema."""
    pines_validos = [5, 18, 19, 23, 33, 27, 26, 25]
    if pin not in pines_validos:
        return f"Error: El pin {pin} no es un relé válido."
    
    estado = 1 if encender else 0
    try:
        url = f"{ESP32_IP}/set?pin={pin}&estado={estado}"
        response = requests.get(url, timeout=3)
        if response.status_code == 200:
            msg = "ENCENDIDO" if encender else "APAGADO"
            return f"Relé en pin {pin} está ahora {msg}."
        return "El ESP32 recibió la orden pero devolvió un error."
    except Exception as e:
        return f"No se pudo conectar con el ESP32: {e}"

@mcp.tool()
def obtener_datos_sensores() -> str:
    """Lee la temperatura, humedad del aire y humedad del suelo."""
    try:
        url = f"{ESP32_IP}/sensores"
        response = requests.get(url, timeout=4)
        if response.status_code == 200:
            d = response.json()

            # Compatible con firmware nuevo (suelo_porcentaje) y viejo (suelo)
            if 'suelo_porcentaje' in d:
                pct  = d['suelo_porcentaje']
                raw  = d.get('suelo_raw', '—')
                if pct >= 80:   estado_suelo = "Muy Húmedo"
                elif pct >= 60: estado_suelo = "Húmedo"
                elif pct >= 40: estado_suelo = "Moderado"
                elif pct >= 20: estado_suelo = "Seco"
                else:           estado_suelo = "Muy Seco"
                suelo_info = f"{pct}% ({estado_suelo}) · RAW: {raw}"
            else:
                raw = d.get('suelo', 0)
                estado_suelo = "Húmedo" if raw < 2000 else "Seco"
                suelo_info = f"{raw} ({estado_suelo})"

            return (f"--- REPORTE DE SENSORES ---\n"
                    f"🌡️ Temperatura: {d['temp']}°C\n"
                    f"💧 Humedad Aire: {d['hum']}%\n"
                    f"🪴 Humedad Suelo: {suelo_info}")
        return "Error al leer los datos del JSON."
    except Exception as e:
        return f"Error de conexión con sensores: {e}"

if __name__ == "__main__":
    mcp.run()
