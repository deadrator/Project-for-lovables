import tkinter as tk
from tkinter import ttk
import math
import random
import time
import threading
import os
import sys

# --- CONFIGURATION ---
APP_TITLE = "MODUSEC // LIVEWIRE v9.1"
REFRESH_RATE = 200  # Milliseconds (Lower = smoother, higher = less CPU)
THEME = {
    "bg": "#050505",       # Void Black
    "grid": "#111111",     # Grid Lines
    "primary": "#00f3ff",  # Tron Cyan
    "secondary": "#ff0055",# Error Red
    "text": "#e0e0e0",     # Off White
    "dim": "#444444"
}

# --- SYSTEM MONITOR ENGINE (Pure Linux /proc Parsing) ---
class SystemMonitor:
    def __init__(self):
        self.last_cpu_time = self.get_cpu_times()
        self.last_net_bytes = self.get_net_bytes()
        self.last_time = time.time()
        self.iface = self.detect_active_interface()

    def detect_active_interface(self):
        # Find interface with most traffic
        best_iface = "lo"
        max_bytes = 0
        try:
            with open('/proc/net/dev', 'r') as f:
                lines = f.readlines()[2:]
                for line in lines:
                    data = line.split(':')
                    iface = data[0].strip()
                    if iface == "lo": continue
                    bytes_val = int(data[1].split()[0])
                    if bytes_val > max_bytes:
                        max_bytes = bytes_val
                        best_iface = iface
        except FileNotFoundError:
            return "sim_mode" # Fallback for non-Linux
        return best_iface

    def get_cpu_times(self):
        # Read /proc/stat for total cpu time
        try:
            with open('/proc/stat', 'r') as f:
                line = f.readline()
                data = [int(x) for x in line.split()[1:]]
                return data
        except: return [0]*10

    def get_net_bytes(self):
        # Read /proc/net/dev
        try:
            with open('/proc/net/dev', 'r') as f:
                for line in f:
                    if self.iface in line:
                        data = line.split(':')[1].split()
                        return int(data[0]), int(data[8]) # rx, tx
        except: pass
        return 0, 0

    def get_stats(self):
        # 1. Calculate CPU %
        cpu_now = self.get_cpu_times()
        # idle is index 3, total is sum of all
        delta_total = sum(cpu_now) - sum(self.last_cpu_time)
        delta_idle = cpu_now[3] - self.last_cpu_time[3]
        cpu_percent = 0
        if delta_total > 0:
            cpu_percent = 100 - (delta_idle / delta_total * 100)
        self.last_cpu_time = cpu_now

        # 2. Calculate RAM
        mem_total = 0
        mem_avail = 0
        try:
            with open('/proc/meminfo', 'r') as f:
                for line in f:
                    if "MemTotal" in line: mem_total = int(line.split()[1])
                    if "MemAvailable" in line: mem_avail = int(line.split()[1])
        except: pass
        ram_percent = 0
        if mem_total > 0:
            ram_percent = 100 - (mem_avail / mem_total * 100)

        # 3. Calculate Network Speed
        net_now = self.get_net_bytes()
        time_now = time.time()
        time_delta = time_now - self.last_time
        
        rx_speed = (net_now[0] - self.last_net_bytes[0]) / time_delta
        tx_speed = (net_now[1] - self.last_net_bytes[1]) / time_delta
        
        self.last_net_bytes = net_now
        self.last_time = time_now

        return {
            "cpu": cpu_percent,
            "ram": ram_percent,
            "rx": rx_speed,
            "tx": tx_speed,
            "iface": self.iface
        }

# --- 3D HOLO GLOBE (Visual Candy) ---
class HoloGlobe(tk.Canvas):
    def __init__(self, master, width=220, height=220, **kwargs):
        super().__init__(master, width=width, height=height, bg=THEME['bg'], highlightthickness=0, **kwargs)
        self.width, self.height = width, height
        self.center_x, self.center_y = width // 2, height // 2
        self.scale = min(width, height) // 2.8
        self.points = []
        self.angle_y = 0
        
        # Fibonacci Sphere
        num_points = 80
        phi = math.pi * (3. - math.sqrt(5.))
        for i in range(num_points):
            y = 1 - (i / float(num_points - 1)) * 2
            radius = math.sqrt(1 - y * y)
            theta = phi * i
            x = math.cos(theta) * radius
            z = math.sin(theta) * radius
            self.points.append([x, y, z])
        
        self.animate()

    def animate(self):
        self.delete("all")
        self.angle_y += 0.03
        
        # Draw Axis
        self.create_line(0, self.height/2, self.width, self.height/2, fill="#1a1a1a")
        
        transformed = []
        for x, y, z in self.points:
            # Rotate Y
            rx = x * math.cos(self.angle_y) - z * math.sin(self.angle_y)
            rz = z * math.cos(self.angle_y) + x * math.sin(self.angle_y)
            # Tilt X
            ry = y * math.cos(0.2) - rz * math.sin(0.2)
            rz = rz * math.cos(0.2) + y * math.sin(0.2)
            transformed.append((rx, ry, rz))

        transformed.sort(key=lambda p: p[2])

        for rx, ry, rz in transformed:
            factor = 300 / (300 + rz * self.scale)
            px = rx * self.scale + self.center_x
            py = ry * self.scale + self.center_y
            
            size = 1.5 if rz < 0 else 2.5
            color = THEME['dim'] if rz < 0 else THEME['primary']
            self.create_oval(px-size, py-size, px+size, py+size, fill=color, outline="")

        self.after(50, self.animate)

# --- REALTIME NETWORK GRAPH ---
class LiveGraph(tk.Canvas):
    def __init__(self, master, height=100, **kwargs):
        super().__init__(master, height=height, bg=THEME['bg'], highlightthickness=0, **kwargs)
        self.height = height
        self.rx_data = [0] * 60
        self.width = 100
        self.bind("<Configure>", self.resize)

    def resize(self, event):
        self.width = event.width

    def update_data(self, rx_speed, tx_speed):
        self.delete("all")
        
        # Shift data
        self.rx_data.pop(0)
        self.rx_data.append(rx_speed)
        
        # Auto-Scale Logic
        max_val = max(self.rx_data)
        if max_val < 1024: max_val = 1024 # Min scale 1KB
        
        step = self.width / (len(self.rx_data) - 1)
        points = []
        
        # Draw Grid & Lines
        for i, val in enumerate(self.rx_data):
            x = i * step
            # Scale height (leaving 10px padding)
            y = self.height - ((val / max_val) * (self.height - 20)) - 10
            points.append(x)
            points.append(y)
            
            # Draw vertical scanline effect
            if i % 3 == 0:
                self.create_line(x, self.height, x, y, fill="#0a1a1a")

        # Draw Connectivity Line
        if len(points) >= 4:
            self.create_line(points, fill=THEME['primary'], width=2, smooth=True)
            # Glow effect (duplicate line, thicker, semi-transparent simulation)
            self.create_line(points, fill="#005555", width=6, smooth=True) 

        # Legend / Text Stats
        self.create_text(5, 5, text=f"RX: {self.format_bytes(rx_speed)}/s", fill=THEME['primary'], anchor="nw", font=("Consolas", 9, "bold"))
        self.create_text(5, 20, text=f"TX: {self.format_bytes(tx_speed)}/s", fill=THEME['secondary'], anchor="nw", font=("Consolas", 9))
        self.create_text(self.width-5, 5, text=f"MAX: {self.format_bytes(max_val)}/s", fill=THEME['dim'], anchor="ne", font=("Consolas", 8))

    def format_bytes(self, size):
        power = 2**10
        n = 0
        power_labels = {0 : '', 1: 'K', 2: 'M', 3: 'G', 4: 'T'}
        while size > power:
            size /= power
            n += 1
        return f"{size:.1f} {power_labels[n]}B"

# --- MAIN APP UI ---
class ModuSecVisual:
    def __init__(self, root):
        self.root = root
        self.root.title(APP_TITLE)
        self.root.geometry("1100x700")
        self.root.configure(bg=THEME['bg'])
        self.monitor = SystemMonitor()
        
        self.setup_ui()
        self.update_loop()

    def setup_ui(self):
        # Top Bar
        top = tk.Frame(self.root, bg=THEME['bg'])
        top.pack(fill=tk.X, padx=10, pady=5)
        tk.Label(top, text="MODUSEC // SYSTEM MONITOR", fg=THEME['text'], bg=THEME['bg'], font=("Consolas", 12, "bold")).pack(side=tk.LEFT)
        self.lbl_iface = tk.Label(top, text=f"IFACE: {self.monitor.iface}", fg=THEME['secondary'], bg=THEME['bg'], font=("Consolas", 10))
        self.lbl_iface.pack(side=tk.RIGHT)

        # Main Split
        main = tk.Frame(self.root, bg=THEME['bg'])
        main.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)

        # Left: Files
        left = tk.Frame(main, bg=THEME['bg'], width=200)
        left.pack(side=tk.LEFT, fill=tk.Y)
        tk.Label(left, text="[ FILESYSTEM ]", fg=THEME['dim'], bg=THEME['bg'], font=("Consolas", 10)).pack(anchor="w")
        self.lst_files = tk.Listbox(left, bg=THEME['bg'], fg=THEME['text'], font=("Consolas", 9), bd=0, highlightthickness=0)
        self.lst_files.pack(fill=tk.BOTH, expand=True)
        # Fake files for aesthetic
        for i in range(20): self.lst_files.insert(tk.END, f"capture_packets_{i:03d}.pcap")

        # Right: Visuals
        right = tk.Frame(main, bg=THEME['bg'])
        right.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True, padx=(10,0))
        
        # Globe & Stats Row
        row1 = tk.Frame(right, bg=THEME['bg'])
        row1.pack(fill=tk.X)
        
        # Globe
        self.globe = HoloGlobe(row1, width=180, height=180)
        self.globe.pack(side=tk.RIGHT)
        
        # CPU/RAM Bars
        stats = tk.Frame(row1, bg=THEME['bg'])
        stats.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(0,10))
        
        self.cpu_bar = self.create_bar(stats, "CPU LOAD")
        self.ram_bar = self.create_bar(stats, "MEMORY USAGE")
        
        # Traffic Graph
        tk.Label(right, text="[ NETWORK TRAFFIC ]", fg=THEME['dim'], bg=THEME['bg'], font=("Consolas", 10)).pack(anchor="w", pady=(10,0))
        self.graph = LiveGraph(right, height=150)
        self.graph.pack(fill=tk.X, pady=5)

        # Terminal
        tk.Label(right, text="[ TERMINAL LOG ]", fg=THEME['dim'], bg=THEME['bg'], font=("Consolas", 10)).pack(anchor="w", pady=(10,0))
        term_frame = tk.Frame(right, bd=1, relief="solid", bg=THEME['dim'])
        term_frame.pack(fill=tk.BOTH, expand=True)
        self.term = tk.Text(term_frame, bg="black", fg=THEME['text'], font=("Consolas", 10), bd=0)
        self.term.pack(fill=tk.BOTH, expand=True)
        self.term.insert(tk.END, "root@modusec:~# Initializing realtime daemon...\nroot@modusec:~# Linked to /proc/net/dev\nroot@modusec:~# Monitoring active...\n")

    def create_bar(self, parent, title):
        frame = tk.Frame(parent, bg=THEME['bg'], pady=5)
        frame.pack(fill=tk.X)
        tk.Label(frame, text=title, fg=THEME['primary'], bg=THEME['bg'], font=("Consolas", 8)).pack(anchor="w")
        canvas = tk.Canvas(frame, height=15, bg="#111", highlightthickness=0)
        canvas.pack(fill=tk.X)
        return canvas

    def draw_bar(self, canvas, percent):
        canvas.delete("all")
        w = canvas.winfo_width()
        fill_w = (percent / 100) * w
        color = THEME['primary']
        if percent > 80: color = THEME['secondary']
        canvas.create_rectangle(0, 0, fill_w, 15, fill=color, outline="")
        canvas.create_text(w-5, 7, text=f"{int(percent)}%", fill="white", anchor="e", font=("Consolas", 8))

    def update_loop(self):
        # 1. Get Data
        data = self.monitor.get_stats()
        
        # 2. Update Graph
        self.graph.update_data(data['rx'], data['tx'])
        
        # 3. Update Bars
        self.draw_bar(self.cpu_bar, data['cpu'])
        self.draw_bar(self.ram_bar, data['ram'])
        
        # Loop
        self.root.after(REFRESH_RATE, self.update_loop)

if __name__ == "__main__":
    root = tk.Tk()
    app = ModuSecVisual(root)
    root.mainloop()

