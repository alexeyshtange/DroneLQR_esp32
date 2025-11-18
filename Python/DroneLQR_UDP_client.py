import socket
import threading
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

    def set_target_angles(self, roll: float, pitch: float, yaw: float):
        msg = f"ANGLES={roll},{pitch},{yaw}".encode()
        self.target = (roll, pitch, yaw)
        self.sock.sendto(msg, self.addr)

    def start(self):
        self.running = True
        threading.Thread(target=self._recv_loop, daemon=True).start()

    def stop(self):
        self.running = False

    def _recv_loop(self):
        self.sock.settimeout(0.5)
        while self.running:
            try:
                data, _ = self.sock.recvfrom(64)
                rpy = data.decode().split(',')
                if len(rpy) == 3:
                    self.measured = tuple(map(float, rpy))
            except:
                pass

# -------------------- GUI --------------------

class DroneGUI(tk.Tk):
    def __init__(self, client: DroneClient):
        super().__init__()
        self.client = client
        self.title("Drone Telemetry (Tkinter)")
        self.geometry("700x450")

        self.window_size = 100
        self.data_roll = []
        self.data_pitch = []
        self.data_yaw = []
        self.target_roll = []
        self.target_pitch = []
        self.target_yaw = []

        # Sliders
        self.sliders = {}
        for axis in ["Roll", "Pitch", "Yaw"]:
            lbl = tk.Label(self, text=f"{axis}: 0.0")
            lbl.pack()
            slider = tk.Scale(self, from_=-45.0, to=45.0, resolution=0.5,
                              orient=tk.HORIZONTAL, length=400,
                              command=lambda val, ax=axis: self.on_slider(ax, val))
            slider.pack()
            self.sliders[axis] = (lbl, slider)

        # Chart
        self.fig, self.ax = plt.subplots()
        self.lines = {
            "Roll_measured": self.ax.plot([], [], label="Roll (measured)")[0],
            "Roll_target": self.ax.plot([], [], '--', label="Roll (target)")[0],
            "Pitch_measured": self.ax.plot([], [], label="Pitch (measured)")[0],
            "Pitch_target": self.ax.plot([], [], '--', label="Pitch (target)")[0],
            "Yaw_measured": self.ax.plot([], [], label="Yaw (measured)")[0],
            "Yaw_target": self.ax.plot([], [], '--', label="Yaw (target)")[0],
        }
        self.ax.set_ylim(-50, 50)
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
        # measured
        r, p, y = self.client.measured
        self.data_roll.append(r)
        self.data_pitch.append(p)
        self.data_yaw.append(y)

        # setpoints
        tr, tp, ty = self.client.target
        self.target_roll.append(tr)
        self.target_pitch.append(tp)
        self.target_yaw.append(ty)

        # kepp last N points
        for data in [self.data_roll, self.data_pitch, self.data_yaw,
                     self.target_roll, self.target_pitch, self.target_yaw]:
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
        self.canvas.draw()
        self.after(20, self.update_graph)

# -------------------- Main --------------------

if __name__ == "__main__":
    client = DroneClient()
    client.start()
    gui = DroneGUI(client)
    gui.mainloop()
    client.stop()
