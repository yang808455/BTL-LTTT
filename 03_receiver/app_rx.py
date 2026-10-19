import asyncio
import json
import threading
from typing import Optional
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.responses import HTMLResponse
import serial
import serial.tools.list_ports
import uvicorn

app = FastAPI(title="Node 3 - Receiver Dashboard")

ser: Optional[serial.Serial] = None
ser_lock = threading.Lock()
active_websockets = []
serial_thread: Optional[threading.Thread] = None
running = False

current_config = {
    "port": "",
    "baud": 115200,
    "connected": False
}

def serial_reader_task():
    global ser, running
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    
    while running:
        if ser and ser.is_open:
            try:
                if ser.in_waiting > 0:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line.startswith("{") and line.endswith("}"):
                        try:
                            data = json.loads(line)
                            for ws in list(active_websockets):
                                asyncio.run_coroutine_threadsafe(ws.send_json(data), loop)
                        except json.JSONDecodeError:
                            pass
            except Exception as e:
                print(f"[Rx Serial Error] {e}")
        threading.Event().wait(0.01)

@app.on_event("startup")
def startup_event():
    global running, serial_thread
    running = True
    serial_thread = threading.Thread(target=serial_reader_task, daemon=True)
    serial_thread.start()

@app.on_event("shutdown")
def shutdown_event():
    global running, ser
    running = False
    if ser and ser.is_open:
        ser.close()

@app.get("/", response_class=HTMLResponse)
async def get_index():
    with open("templates/index.html", "r", encoding="utf-8") as f:
        return f.read()

@app.get("/api/ports")
async def list_ports():
    ports = [p.device for p in serial.tools.list_ports.comports()]
    return {"ports": ports, "current": current_config["port"], "connected": current_config["connected"]}

@app.post("/api/connect")
async def connect_port(data: dict):
    global ser
    port_name = data.get("port")
    if not port_name:
        return {"status": "error", "message": "No port specified"}
    
    with ser_lock:
        if ser and ser.is_open:
            ser.close()
        try:
            ser = serial.Serial(port_name, current_config["baud"], timeout=0.1)
            current_config["port"] = port_name
            current_config["connected"] = True
            return {"status": "ok", "message": f"Connected to {port_name}"}
        except Exception as e:
            current_config["connected"] = False
            return {"status": "error", "message": str(e)}

@app.post("/api/disconnect")
async def disconnect_port():
    global ser
    with ser_lock:
        if ser and ser.is_open:
            ser.close()
            ser = None
        current_config["connected"] = False
    return {"status": "ok", "message": "Disconnected"}

@app.post("/api/reset_metrics")
async def reset_metrics():
    global ser
    with ser_lock:
        if ser and ser.is_open:
            ser.write((json.dumps({"cmd": "RESET_METRICS"}) + "\n").encode('utf-8'))
    return {"status": "ok"}

@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await websocket.accept()
    active_websockets.append(websocket)
    try:
        while True:
            await websocket.receive_text()
    except WebSocketDisconnect:
        active_websockets.remove(websocket)

if __name__ == "__main__":
    uvicorn.run("app_rx:app", host="0.0.0.0", port=8003, reload=True)
