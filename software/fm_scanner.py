##
# @file fm_scanner.py
# @brief FM Radio Scanner — Serial visualizer and control dashboard.
#
# Reads RSSI data from the Arduino via UART and plots a live frequency
# spectrum graph. A Tkinter dashboard provides full radio control
# (volume, mute, frequency tuning and scan triggering) without
# needing to touch the physical buttons.
#
# @author J.P.Santos
# @date 2025
#
# Serial data format (Arduino -> Python):
# @code
# SCAN_INFO: tipo=Total passo=10 amostras=10
# SCAN_INFO: tipo=Central centro=9320 passo=10 amostras=10
# Freq: 93.2 MHz | RSSI: 15, 16, 14 | Media RSSI: 15
# VOL:7
# MUTE:ON / MUTE:OFF
# FREQ_OK:9320
# @endcode
#
# Commands sent (Python -> Arduino):
# @code
# VOL+   VOL-   MUTE
# FREQ:<xxxx>
# SCAN_TOTAL:<step>:<samples>
# SCAN_CENTER:<centre>:<step>:<samples>
# @endcode
#
# Usage:
# @code
# python fm_scanner.py
# python fm_scanner.py --port COM3
# @endcode
#
# Graph keyboard shortcuts:
#   Space -> Pause/resume | C -> Colour | S -> Save | R -> Reset

import re
import time
import argparse
import sys
import os
import tkinter as tk

import serial
import serial.tools.list_ports
import numpy as np
import matplotlib
matplotlib.use("TkAgg")
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
from matplotlib.animation import FuncAnimation
from matplotlib.backend_bases import NavigationToolbar2
from datetime import datetime


# ── Regex patterns ────────────────────────────────────────────────────────────

## Matches a complete data line from the Arduino.
# Group 1 = frequency (MHz), Group 2 = RSSI samples, Group 3 = log average.
LINE_PATTERN = re.compile(
    r"Freq:\s*([\d.]+)\s*MHz\s*\|\s*RSSI:\s*([\d,\s]+)\s*\|\s*M[eé]dia RSSI:\s*(-?\d+)"
)

## Matches a total-scan header line: SCAN_INFO: tipo=Total passo=PP amostras=AA
INFO_TOTAL = re.compile(
    r"SCAN_INFO:\s*tipo=Total\s+passo=(\d+)\s+amostras=(\d+)"
)

## Matches a central-scan header line: SCAN_INFO: tipo=Central centro=FF passo=PP amostras=AA
INFO_CENTRAL = re.compile(
    r"SCAN_INFO:\s*tipo=Central\s+centro=(\d+)\s+passo=(\d+)\s+amostras=(\d+)"
)

## Matches a volume feedback line: VOL:<n>
VOL_PATTERN  = re.compile(r"VOL:(\d+)")

## Matches a mute feedback line: MUTE:ON or MUTE:OFF
MUTE_PATTERN = re.compile(r"MUTE:(ON|OFF)")

## Matches a frequency confirmation line: FREQ_OK:<xxxx>
FREQ_PATTERN = re.compile(r"FREQ_OK:(\d+)")

## Colour palette for overlaid scans. Each entry is (line_colour, fill_colour, label).
COLOURS = [
    ("#7b0028", "#e75480", "Scan 1 - Vermelho"),
    ("#00529b", "#4fa3e0", "Scan 2 - Azul"),
    ("#1a7a1a", "#5cbf5c", "Scan 3 - Verde"),
    ("#7b5500", "#e0a830", "Scan 4 - Amarelo"),
    ("#5a008a", "#b36bd4", "Scan 5 - Roxo"),
    ("#008a7a", "#30d4c4", "Scan 6 - Ciano"),
]


def parse_line(line):
    ##
    # @brief Parse one serial data line from the Arduino.
    #
    # Extracts the frequency, individual RSSI samples and the logarithmic
    # average from a line of the form:
    # "Freq: 93.2 MHz | RSSI: 15, 16, 14 | Media RSSI: 15"
    #
    # @param line Raw string received from the serial port.
    # @return Tuple (freq_mhz, samples_list, avg_rssi) on success, or None.
    m = LINE_PATTERN.search(line)
    if not m:
        return None
    freq    = float(m.group(1))
    samples = [int(x.strip()) for x in m.group(2).split(",") if x.strip()]
    avg     = int(m.group(3))
    return freq, samples, avg


def parse_scan_info(line):
    ##
    # @brief Parse a SCAN_INFO header line sent by the Arduino before each scan.
    #
    # Returns a dict that tells the graph what X-axis range to pre-set:
    # - Total scan  -> fixed 80.0 to 108.0 MHz.
    # - Central scan -> centre +/- 10 steps, clamped to 80-108 MHz.
    #
    # @param line Raw string received from the serial port.
    # @return Dict with keys 'tipo', 'xmin', 'xmax' (and 'centro' for Central),
    #         or None if the line does not match.
    m = INFO_TOTAL.search(line)
    if m:
        return {"tipo": "Total", "xmin": 80.0, "xmax": 108.0}
    m = INFO_CENTRAL.search(line)
    if m:
        centro = int(m.group(1)) / 100.0
        passo  = int(m.group(2)) / 100.0
        xmin   = max(80.0,  centro - 10 * passo)
        xmax   = min(108.0, centro + 10 * passo)
        return {"tipo": "Central", "xmin": round(xmin, 2),
                "xmax": round(xmax, 2), "centro": centro}
    return None


def find_arduino_port():
    ##
    # @brief Auto-detect the Arduino serial port.
    #
    # Scans all available COM ports and returns the first one whose
    # description or manufacturer string contains a known Arduino/USB-serial
    # keyword (ch340, cp210, ftdi, esp32, etc.).
    # Falls back to the first available port if none match.
    #
    # @return Port device string (e.g. "COM3" or "/dev/ttyUSB0"), or None.
    ports    = serial.tools.list_ports.comports()
    keywords = ["arduino", "ch340", "cp210", "ftdi", "usb serial", "esp32", "esp"]
    for p in ports:
        desc = (p.description  or "").lower()
        mfr  = (p.manufacturer or "").lower()
        if any(k in desc or k in mfr for k in keywords):
            return p.device
    if ports:
        return ports[0].device
    return None


def do_save(fig, scans, raw_lines):
    ##
    # @brief Save the current graph as PNG and the scan data as TXT.
    #
    # Both files are written to the working directory with a shared
    # timestamp so they are easy to match (e.g. fm_scan_20250601_143022).
    #
    # The TXT file contains one section per scan colour with columns:
    # Freq (MHz), individual RSSI samples, and logarithmic average.
    #
    # @param fig       The matplotlib Figure object to save as PNG.
    # @param scans     List of scan dicts (as built by get_or_create_scan()).
    # @param raw_lines List of every raw line received from the Arduino.
    has_data = any(len(s["freqs"]) > 0 for s in scans)
    if not has_data:
        print("[!] No data yet - nothing to save.")
        return
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")

    png_name = f"fm_scan_{ts}.png"
    fig.savefig(png_name, dpi=150, bbox_inches="tight")
    print(f"[OK] Graph saved -> {png_name}")

    txt_name = f"fm_scan_{ts}.txt"
    with open(txt_name, "w", encoding="utf-8") as f:
        f.write("FM Scanner - Data Log\n")
        f.write(f"Saved: {ts}\n")
        f.write(f"{'=' * 50}\n\n")
        for scan in scans:
            if not scan["freqs"]:
                continue
            f.write(f"--- {scan['label']} ---\n")
            f.write(f"{'Freq (MHz)':<15} {'Amostras RSSI':<35} {'Media RSSI'}\n")
            f.write(f"{'-' * 65}\n")
            for j in range(len(scan["freqs"])):
                samples = ", ".join(str(x) for x in scan["all_samples"][j])
                f.write(f"{scan['freqs'][j]:<15.1f} {samples:<35} {scan['avgs'][j]}\n")
            f.write("\n")
    print(f"[OK] Data saved  -> {txt_name}")
    print(f"[i]  Folder      -> {os.path.abspath('.')}")


# ─────────────────────────────────────────────────────────────────────────────

##
# @class Dashboard
# @brief Tkinter control dashboard window for the FM Radio Scanner.
#
# Provides native OS buttons (volume, mute, frequency, scan) that are
# guaranteed to respond to clicks. Instead of calling the serial port
# directly, each button appends a command string to state["cmd_queue"].
# The matplotlib animation loop drains the queue each frame and sends
# the commands over serial — keeping all serial I/O on one thread.
class Dashboard(tk.Tk):

    BG     = "#2b2d3a"   ##< Main background colour
    BG2    = "#3a3c4e"   ##< Entry/textbox background colour
    FG     = "#e0e0e0"   ##< Primary text colour
    FG_DIM = "#aaaaaa"   ##< Secondary/label text colour
    GREEN  = "#2e7d32"   ##< Volume button colour
    RED    = "#c62828"   ##< Mute button colour
    BLUE   = "#1565c0"   ##< Scan button colour
    ORANGE = "#e65100"   ##< Frequency button colour
    YELLOW = "#f9c74f"   ##< Frequency display colour

    def __init__(self, state):
        ##
        # @brief Construct and render the dashboard window.
        # @param state Shared state dict (see live_mode() for keys).
        super().__init__()
        self.state = state
        self.title("FM Radio - Dashboard")
        self.configure(bg=self.BG)
        self.resizable(False, False)
        self._build()

    def _section(self, parent, text):
        ##
        # @brief Add a bold section header label.
        # @param parent Tkinter parent widget.
        # @param text   Header string to display.
        tk.Label(parent, text=text, bg=self.BG, fg=self.FG,
                 font=("Segoe UI", 10, "bold")).pack(pady=(14, 4))

    def _btn(self, parent, text, color, command, width=18):
        ##
        # @brief Create and pack a full-width styled button.
        # @param parent  Tkinter parent widget.
        # @param text    Button label.
        # @param color   Button background colour hex string.
        # @param command Callback function called on click.
        # @param width   Button width in characters.
        # @return The Button widget.
        b = tk.Button(parent, text=text, bg=color, fg="white",
                      font=("Segoe UI", 9, "bold"),
                      activebackground="#555577", activeforeground="white",
                      relief="flat", bd=0, padx=8, pady=6,
                      width=width, cursor="hand2", command=command)
        b.pack(fill="x", padx=10, pady=2)
        return b

    def _row_btns(self, parent, items):
        ##
        # @brief Create a row of two side-by-side buttons.
        # @param parent Tkinter parent widget.
        # @param items  List of (label, colour, callback) tuples (exactly 2).
        row = tk.Frame(parent, bg=self.BG)
        row.pack(fill="x", padx=10, pady=2)
        for text, color, cmd in items:
            b = tk.Button(row, text=text, bg=color, fg="white",
                          font=("Segoe UI", 9, "bold"),
                          activebackground="#555577", activeforeground="white",
                          relief="flat", bd=0, padx=4, pady=6,
                          cursor="hand2", command=cmd)
            b.pack(side="left", expand=True, fill="x", padx=2)

    def _labelled_entry(self, parent, label_text, default):
        ##
        # @brief Create an Entry widget with a label above it.
        # @param parent     Tkinter parent widget.
        # @param label_text Text shown above the entry box.
        # @param default    Initial value string.
        # @return StringVar bound to the entry.
        tk.Label(parent, text=label_text, bg=self.BG, fg=self.FG_DIM,
                 font=("Segoe UI", 8)).pack(anchor="w", padx=12)
        var = tk.StringVar(value=default)
        e = tk.Entry(parent, textvariable=var,
                     bg=self.BG2, fg="white", insertbackground="white",
                     font=("Segoe UI", 10), relief="flat", bd=4)
        e.pack(fill="x", padx=12, pady=(0, 4))
        return var

    def _build(self):
        ##
        # @brief Construct all dashboard widgets.
        #
        # Sections: Volume, Frequency, Scan, Graph controls.
        # Called once from __init__.

        # Volume
        self._section(self, "VOLUME")
        self.vol_label = tk.Label(self, text="Vol: 5 / 15",
                                  bg=self.BG, fg=self.FG,
                                  font=("Segoe UI", 10))
        self.vol_label.pack()
        self.mute_label = tk.Label(self, text="",
                                   bg=self.BG, fg="#ff6b6b",
                                   font=("Segoe UI", 9, "bold"))
        self.mute_label.pack()
        self._row_btns(self, [
            ("VOL +", self.GREEN, lambda: self.state["cmd_queue"].append("VOL+")),
            ("VOL -", self.GREEN, lambda: self.state["cmd_queue"].append("VOL-")),
        ])
        self._btn(self, "MUTE", self.RED,
                  lambda: self.state["cmd_queue"].append("MUTE"))

        # Frequency
        self._section(self, "FREQUENCIA")
        self.freq_label = tk.Label(self, text="93.2 MHz",
                                   bg=self.BG, fg=self.YELLOW,
                                   font=("Segoe UI", 13, "bold"))
        self.freq_label.pack(pady=(0, 6))
        self.freq_var = self._labelled_entry(self, "Frequencia (MHz):", "93.2")
        self._btn(self, "SET FREQ", self.ORANGE, self._on_freq_set)
        self._row_btns(self, [
            ("+ 0.1 MHz", self.ORANGE, self._on_freq_up),
            ("- 0.1 MHz", self.ORANGE, self._on_freq_down),
        ])

        # Scan
        self._section(self, "SCAN")
        self.step_var = self._labelled_entry(self, "Step:", "10")
        self.samp_var = self._labelled_entry(self, "Samples:", "10")
        self._btn(self, "SCAN TOTAL",   self.BLUE, self._on_scan_total)
        self._btn(self, "SCAN CENTRAL", self.BLUE, self._on_scan_center)

        # Graph controls
        self._section(self, "GRAPH")
        self.colour_label = tk.Label(self, text=f"Colour: {COLOURS[0][2]}",
                                     bg=self.BG, fg=COLOURS[0][0],
                                     font=("Segoe UI", 9))
        self.colour_label.pack(pady=(0, 4))
        self._row_btns(self, [
            ("Change Colour", "#555577",
             lambda: self.state["cmd_queue"].append("CHANGE_COLOUR")),
            ("Reset Graph",   "#773333",
             lambda: self.state["cmd_queue"].append("RESET")),
        ])
        self._row_btns(self, [
            ("Pause / Resume", "#555555",
             lambda: self.state["cmd_queue"].append("PAUSE")),
            ("Save PNG + TXT", "#226622",
             lambda: self.state["cmd_queue"].append("SAVE")),
        ])
        tk.Frame(self, bg=self.BG, height=10).pack()

    def _on_freq_set(self):
        ##
        # @brief Validate and queue a FREQ: command from the entry box.
        #
        # Converts the MHz value typed by the user to RDA units
        # (multiply by 100) and validates the 80-108 MHz range before queuing.
        try:
            val   = float(self.freq_var.get())
            units = int(round(val * 100))
            if 8000 <= units <= 10800:
                self.state["cmd_queue"].append(f"FREQ:{units}")
            else:
                print("[!] Frequency out of range (80.0-108.0 MHz)")
        except ValueError:
            print("[!] Invalid frequency value")

    def _on_freq_up(self):
        ##
        # @brief Queue a +0.1 MHz frequency step command.
        self.state["cmd_queue"].append("FREQ_UP")

    def _on_freq_down(self):
        ##
        # @brief Queue a -0.1 MHz frequency step command.
        self.state["cmd_queue"].append("FREQ_DOWN")

    def _on_scan_total(self):
        ##
        # @brief Validate inputs and queue a SCAN_TOTAL command.
        try:
            step    = int(self.step_var.get())
            samples = int(self.samp_var.get())
            self.state["cmd_queue"].append(f"SCAN_TOTAL:{step}:{samples}")
        except ValueError:
            print("[!] Invalid step or samples")

    def _on_scan_center(self):
        ##
        # @brief Validate inputs and queue a SCAN_CENTER command.
        #
        # Uses the currently tuned frequency (from state) as the centre.
        try:
            step    = int(self.step_var.get())
            samples = int(self.samp_var.get())
            freq    = self.state["frequency"]
            self.state["cmd_queue"].append(f"SCAN_CENTER:{freq}:{step}:{samples}")
        except ValueError:
            print("[!] Invalid step or samples")

    def refresh(self, state):
        ##
        # @brief Synchronise dashboard labels with the current shared state.
        #
        # Called once per animation frame (every 200 ms) from the matplotlib
        # update function to keep volume, mute, frequency and colour labels
        # in sync with what the Arduino is actually doing.
        #
        # @param state The shared state dict.
        self.vol_label.config(text=f"Vol: {state['volume']} / 15")
        self.mute_label.config(text="[ MUTED ]" if state["muted"] else "")
        self.freq_label.config(text=f"{state['frequency']/100:.1f} MHz")
        idx = state["colour_idx"]
        self.colour_label.config(text=f"Colour: {COLOURS[idx][2]}",
                                 fg=COLOURS[idx][0])


def live_mode(port, baud=115200):
    ##
    # @brief Open the serial port, build the graph and dashboard, and run the main loop.
    #
    # Architecture:
    # - A Tkinter Dashboard window provides the control buttons.
    # - A matplotlib figure (TkAgg backend) shows the live RSSI graph.
    # - A FuncAnimation callback fires every 200 ms:
    #     1. Drains state["cmd_queue"] and sends each command over serial.
    #     2. Reads all waiting serial lines and parses them.
    #     3. Calls dash.refresh() to keep dashboard labels up to date.
    #     4. Redraws all scan lines and shaded fills on the graph.
    #
    # @param port Serial port string (e.g. "COM3" or "/dev/ttyUSB0").
    # @param baud Baud rate — must match Serial.begin() in the Arduino sketch.
    print(f"[->] Opening serial port {port} at {baud} baud ...")

    try:
        ser = serial.Serial(port, baud, timeout=2)
    except serial.SerialException as e:
        print(f"[X] Could not open port: {e}")
        sys.exit(1)

    time.sleep(2)
    ser.reset_input_buffer()

    def send(cmd):
        ##
        # @brief Send a command string to the Arduino over serial.
        # @param cmd Command string (a newline is appended automatically).
        ser.write((cmd + "\n").encode("utf-8"))
        print(f"[->] Sent: {cmd}")

    # Shared state — read/written by both the dashboard callbacks and the animation loop
    state = {
        "cmd_queue" : [],   ##< Commands queued by the dashboard, drained by the animation loop
        "scans"     : [],   ##< List of scan dicts, one per colour used
        "raw_lines" : [],   ##< Every raw serial line received (for TXT save)
        "scan_info" : None, ##< Latest parsed SCAN_INFO dict
        "colour_idx": 0,    ##< Index into COLOURS for the current scan
        "paused"    : False,##< True when the live update is paused
        "muted"     : False,##< True when audio is muted
        "volume"    : 5,    ##< Current volume level (0-15)
        "frequency" : 9320, ##< Current tuned frequency in RDA units
    }

    dash = Dashboard(state)
    dash.update()

    # Graph setup
    fig, ax = plt.subplots(figsize=(13, 7))
    fig.patch.set_facecolor("#e8e8e8")
    ax.set_facecolor("#ffffff")
    for spine in ax.spines.values():
        spine.set_edgecolor("#cccccc")

    title_obj = ax.set_title("FM Scan - Waiting for scan...",
                             fontsize=13, fontweight="bold", color="#222222")
    ax.set_xlabel("Frequencia (MHz)", fontsize=11, color="#333333")
    ax.set_ylabel("RSSI",             fontsize=11, color="#333333")
    ax.set_ylim(0, 40)
    ax.set_xlim(80, 108)
    ax.xaxis.set_major_formatter(ticker.FormatStrFormatter("%.1f"))
    ax.tick_params(colors="#444444")
    ax.grid(axis="y", linestyle="--", linewidth=0.5, alpha=0.5, color="#bbbbbb")
    plt.tight_layout(rect=[0, 0.04, 1, 1])

    status_text = fig.text(0.01, 0.01, "LIVE", ha="left", va="bottom",
                           fontsize=9, color="#2a9d2a", fontweight="bold")
    colour_text = fig.text(0.50, 0.01, f"  {COLOURS[0][2]}", ha="center", va="bottom",
                           fontsize=9, style="italic", color=COLOURS[0][0])
    fig.text(0.01, 0.975, "SPACE=pause  |  C=colour  |  S=save  |  R=reset",
             ha="left", va="top", fontsize=8, color="#555555", style="italic")

    def update_colour_label():
        ##
        # @brief Refresh the colour indicator text below the graph.
        idx = state["colour_idx"]
        colour_text.set_text(f"  {COLOURS[idx][2]}")
        colour_text.set_color(COLOURS[idx][0])

    def get_or_create_scan():
        ##
        # @brief Return the scan dict for the current colour, creating it if needed.
        #
        # Each unique colour_idx gets its own scan dict containing the data
        # arrays and the matplotlib Line2D / fill_between objects.
        #
        # @return Scan dict with keys: freqs, avgs, all_samples, colour_idx,
        #         line_col, fill_col, label, line_obj, fill_obj.
        idx = state["colour_idx"]
        for s in state["scans"]:
            if s["colour_idx"] == idx:
                return s
        line_col, fill_col, label = COLOURS[idx]
        line_obj, = ax.plot([], [], color=line_col, linewidth=2.2,
                            label=label, zorder=5)
        new_scan = {
            "freqs": [], "avgs": [], "all_samples": [],
            "colour_idx": idx,
            "line_col": line_col, "fill_col": fill_col,
            "label": label, "line_obj": line_obj, "fill_obj": None,
        }
        state["scans"].append(new_scan)
        ax.legend(fontsize=8, loc="upper right")
        return new_scan

    def apply_scan_info(info):
        ##
        # @brief Pre-set the X axis as soon as a SCAN_INFO line is received.
        #
        # For a Total scan the axis is fixed at 80-108 MHz.
        # For a Central scan it is set to centre +/- 10 steps.
        #
        # @param info Dict returned by parse_scan_info().
        ax.set_xlim(info["xmin"] - 0.2, info["xmax"] + 0.2)
        ax.set_ylim(0, 40)
        if info["tipo"] == "Total":
            title_obj.set_text("FM Scan - Banda Total (80-108 MHz)")
        else:
            c = info["centro"]
            title_obj.set_text(f"FM Scan - Central ({c:.1f} MHz +/- 10 passos)")

    def do_reset():
        ##
        # @brief Clear all scans and reset the graph to its initial state.
        for s in state["scans"]:
            s["line_obj"].remove()
            if s["fill_obj"] is not None:
                s["fill_obj"].remove()
        state["scans"].clear()
        state["raw_lines"].clear()
        state["scan_info"] = None
        state["colour_idx"] = 0
        ax.set_xlim(80, 108)
        ax.set_ylim(0, 40)
        title_obj.set_text("FM Scan - Waiting for scan...")
        try:
            ax.get_legend().remove()
        except Exception:
            pass
        update_colour_label()
        ser.reset_input_buffer()
        status_text.set_text("LIVE")
        status_text.set_color("#2a9d2a")
        state["paused"] = False

    original_save = NavigationToolbar2.save_figure

    def custom_save(toolbar_self, *args, **kwargs):
        ##
        # @brief Override the toolbar floppy-disk button to save PNG + TXT.
        do_save(fig, state["scans"], state["raw_lines"])

    NavigationToolbar2.save_figure = custom_save

    def on_key(event):
        ##
        # @brief Handle keyboard shortcuts on the graph window.
        #
        # Keys: Space (pause), C (colour), S (save), R (reset).
        # Appends the equivalent command to cmd_queue so the animation
        # loop handles it on the next tick.
        #
        # @param event Matplotlib key-press event.
        if   event.key == ' ':  state["cmd_queue"].append("PAUSE")
        elif event.key == 'c':  state["cmd_queue"].append("CHANGE_COLOUR")
        elif event.key == 's':  state["cmd_queue"].append("SAVE")
        elif event.key == 'r':  state["cmd_queue"].append("RESET")

    fig.canvas.mpl_connect("key_press_event", on_key)

    def update(_frame):
        ##
        # @brief Main animation callback — called every 200 ms by FuncAnimation.
        #
        # Steps performed each tick:
        # 1. Drain cmd_queue: send serial commands or act on graph commands.
        # 2. Read all pending serial lines and update shared state.
        # 3. Refresh dashboard labels via dash.refresh().
        # 4. Redraw each scan's line and shaded fill on the graph axes.
        #
        # @param _frame Frame index supplied by FuncAnimation (unused).

        # 1. Process commands from dashboard / keyboard
        while state["cmd_queue"]:
            cmd = state["cmd_queue"].pop(0)
            if cmd == "PAUSE":
                state["paused"] = not state["paused"]
                status_text.set_text("PAUSED - zoom/pan active" if state["paused"] else "LIVE")
                status_text.set_color("#cc7700" if state["paused"] else "#2a9d2a")
            elif cmd == "CHANGE_COLOUR":
                state["colour_idx"] = (state["colour_idx"] + 1) % len(COLOURS)
                update_colour_label()
            elif cmd == "RESET":
                do_reset()
            elif cmd == "SAVE":
                do_save(fig, state["scans"], state["raw_lines"])
            elif cmd == "FREQ_UP":
                send(f"FREQ:{min(state['frequency']+10, 10800)}")
            elif cmd == "FREQ_DOWN":
                send(f"FREQ:{max(state['frequency']-10, 8000)}")
            else:
                send(cmd)  # VOL+, VOL-, MUTE, FREQ:xxxx, SCAN_*

        # 2. Read serial
        while ser.in_waiting:
            raw = ser.readline()
            try:
                text = raw.decode("utf-8", errors="replace").strip()
            except Exception:
                continue
            if not text:
                continue
            print(f"  {text}")
            state["raw_lines"].append(text)

            info = parse_scan_info(text)
            if info:
                state["scan_info"] = info
                apply_scan_info(info)
                continue

            m = VOL_PATTERN.match(text)
            if m:
                state["volume"] = int(m.group(1))
                continue

            m = MUTE_PATTERN.match(text)
            if m:
                state["muted"] = (m.group(1) == "ON")
                continue

            m = FREQ_PATTERN.match(text)
            if m:
                state["frequency"] = int(m.group(1))
                continue

            result = parse_line(text)
            if result:
                freq, samples, avg = result
                scan = get_or_create_scan()
                scan["freqs"].append(freq)
                scan["avgs"].append(avg)
                scan["all_samples"].append(samples)

        # 3. Refresh dashboard
        try:
            dash.refresh(state)
            dash.update_idletasks()
        except Exception:
            pass

        # 4. Redraw graph
        if state["paused"]:
            return

        for scan in state["scans"]:
            if len(scan["freqs"]) < 2:
                continue
            f  = np.array(scan["freqs"])
            a  = np.array(scan["avgs"])
            mn = np.array([min(s) for s in scan["all_samples"]])
            mx = np.array([max(s) for s in scan["all_samples"]])
            if scan["fill_obj"] is not None:
                scan["fill_obj"].remove()
            scan["fill_obj"] = ax.fill_between(
                f, mn, mx, color=scan["fill_col"], alpha=0.20)
            scan["line_obj"].set_data(f, a)
            if mx.max() + 5 > ax.get_ylim()[1]:
                ax.set_ylim(0, mx.max() + 5)

    ani = FuncAnimation(fig, update, interval=200, cache_frame_data=False)
    plt.show()

    NavigationToolbar2.save_figure = original_save
    ser.close()
    try:
        dash.destroy()
    except Exception:
        pass
    print("[OK] Session ended.")


def main():
    ##
    # @brief Entry point — parse arguments and start live mode.
    #
    # Accepts --port and --baud as optional command-line arguments.
    # If --port is not given, auto-detection is attempted via find_arduino_port().
    parser = argparse.ArgumentParser(description="FM Scanner - Dashboard + Visualizer")
    parser.add_argument("--port", help="Serial port (e.g. COM3 or /dev/ttyUSB0)")
    parser.add_argument("--baud", type=int, default=115200,
                        help="Baud rate (default 115200)")
    args = parser.parse_args()

    port = args.port or find_arduino_port()
    if not port:
        print("[X] No serial port found. Use --port COM3")
        sys.exit(1)

    print(f"[OK] Using port: {port}")
    live_mode(port, baud=args.baud)


if __name__ == "__main__":
    main()