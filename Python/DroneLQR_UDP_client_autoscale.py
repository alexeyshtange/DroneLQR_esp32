import socket
import threading
import time
import tkinter as tk
import matplotlib
matplotlib.use("TkAgg")
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import matplotlib.pyplot as plt


class DroneClient:
    def __init__(self, ip="192.168.10.1", port=5005):
        self.addr = (ip, port)
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.measured = (0.0, 0.0, 0.0)
        self.target = (0.0, 0.0, 0.0)
        self.running = False
        self._lock = threading.Lock()

    def set_target_angles(self, roll: float, pitch: float, yaw: float):
        with self._lock:
            self.target = (roll, pitch, yaw)

    def send_disconnect(self):
        self.sock.sendto(b"DISCONNECT", self.addr)

    def start(self):
        self.running = True
        threading.Thread(target=self._recv_loop, daemon=True).start()
        threading.Thread(target=self._send_loop, daemon=True).start()

    def stop(self):
        self.running = False
        self.send_disconnect()

    def _recv_loop(self):
        self.sock.settimeout(0.5)
        while self.running:
            try:
                data, _ = self.sock.recvfrom(64)
                msg = data.decode()
                if msg.startswith("ANGLES="):
                    rpy = msg[7:].split(',')
                    if len(rpy) == 3:
                        self.measured = tuple(map(float, rpy))
            except Exception:
                pass

    def _send_loop(self):
        while self.running:
            with self._lock:
                r, p, y = self.target
                msg = f"ANGLES={r},{p},{y}".encode()
            self.sock.sendto(msg, self.addr)
            time.sleep(0.05)


class DroneGUI(tk.Tk):
    def __init__(self, client: DroneClient):
        super().__init__()
        self.client = client
        self.title("Drone Telemetry")
        self.geometry("700x450")

        self.window_size = 100
        self.data_roll = []
        self.data_pitch = []
        self.data_yaw = []
        self.target_roll = []
        self.target_pitch = []
        self.target_yaw = []

        self.sliders = {}
        for axis in ["Roll", "Pitch", "Yaw"]:
            lbl = tk.Label(self, text=f"{axis}: 0.0")
            lbl.pack()
            slider = tk.Scale(
                self,
                from_=-45.0,
                to=45.0,
                resolution=0.5,
                orient=tk.HORIZONTAL,
                length=400,
                command=lambda val, ax=axis: self.on_slider(ax, val),
            )
            slider.pack()
            self.sliders[axis] = (lbl, slider)

        self.fig, self.ax = plt.subplots()
        self.lines = {
            "Roll_measured": self.ax.plot([], [], label="Roll (measured)")[0],
            "Roll_target": self.ax.plot([], [], "--", label="Roll (target)")[0],
            "Pitch_measured": self.ax.plot([], [], label="Pitch (measured)")[0],
            "Pitch_target": self.ax.plot([], [], "--", label="Pitch (target)")[0],
            "Yaw_measured": self.ax.plot([], [], label="Yaw (measured)")[0],
            "Yaw_target": self.ax.plot([], [], "--", label="Yaw (target)")[0],
        }

        self.ax.set_xlim(0, self.window_size)
        self.ax.grid(True)
        self.ax.legend()

        self.canvas = FigureCanvasTkAgg(self.fig, master=self)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)

        self.after(20, self.update_graph)

    def on_slider(self, axis, val):
        val_f = float(val)
        self.sliders[axis][0].config(text=f"{axis}: {val_f:.1f}")
        roll = self.sliders["Roll"][1].get()
        pitch = self.sliders["Pitch"][1].get()
        yaw = self.sliders["Yaw"][1].get()
        self.client.set_target_angles(roll, pitch, yaw)

    def update_graph(self):
        r, p, y = self.client.measured
        self.data_roll.append(r)
        self.data_pitch.append(p)
        self.data_yaw.append(y)

        tr, tp, ty = self.client.target
        self.target_roll.append(tr)
        self.target_pitch.append(tp)
        self.target_yaw.append(ty)

        for data in [
            self.data_roll,
            self.data_pitch,
            self.data_yaw,
            self.target_roll,
            self.target_pitch,
            self.target_yaw,
        ]:
            if len(data) > self.window_size:
                data[:] = data[-self.window_size:]

        x = list(range(len(self.data_roll)))
        self.lines["Roll_measured"].set_data(x, self.data_roll)
        self.lines["Pitch_measured"].set_data(x, self.data_pitch)
        self.lines["Yaw_measured"].set_data(x, self.data_yaw)
        self.lines["Roll_target"].set_data(x, self.target_roll)
        self.lines["Pitch_target"].set_data(x, self.target_pitch)
        self.lines["Yaw_target"].set_data(x, self.target_yaw)

        self.ax.set_xlim(0, self.window_size)

        all_data = (
            self.data_roll
            + self.data_pitch
            + self.data_yaw
            + self.target_roll
            + self.target_pitch
            + self.target_yaw
        )
        if all_data:
            y_min = min(all_data) - 5
            y_max = max(all_data) + 5
            self.ax.set_ylim(y_min, y_max)

        self.canvas.draw()
        self.after(20, self.update_graph)


if __name__ == "__main__":
    client = DroneClient()
    client.start()
    gui = DroneGUI(client)
    gui.mainloop()
    client.stop()
