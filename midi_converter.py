import tkinter as tk
import math
from rand_period import Melody
import sympy as sp
import numpy as np
import pretty_midi

# ----------------------------
# Music Constants
# ----------------------------

KEYS = [
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
]

SCALES = {
    "Major": [2, 2, 1, 2, 2, 2, 1], 
    "Minor": [2, 1, 2, 2, 1, 2, 2]
}

# Logic pro has 9 ovtaces
# Goes from C-2 (0) to G8 (127)
NOTES_PER_OCTAVE = 12 # 0-127

# ----------------------------
# Minimizing note search space
# ----------------------------

def get_scale_notes(key="C", scale="Major"):
    intervals = SCALES[scale]
    start_index = KEYS.index(key)
    notes = [start_index]
    current_index = start_index
    for interval in intervals:
        current_index = (current_index + interval) % NOTES_PER_OCTAVE
        notes.append(current_index)
    return notes[:-1] # Exclude the octave duplication

def available_notes(key="C", scale="Major", octave_range=(2, 4)):
    scale_notes = get_scale_notes(key, scale)
    notes = []
    for octave in range(octave_range[0], octave_range[1]):
        for note in scale_notes:
            midi_note = note + octave * NOTES_PER_OCTAVE
            if 0 <= midi_note <= 127:
                notes.append(midi_note)
    return notes

# --------------------------------------
# Random function for melody generation.
# --------------------------------------

def rasterize(m: Melody, available_notes, BPM=120):
    """
    Convert continuous trig function to discrete notes.
    Returns a list of (time, note_index) tuples.
    """

    # Create symbolic function
    m = Melody(0.1, 3)
    # Lambdify using NUMPY backend (symbolic -> numeric)
    m_func = sp.lambdify(m.x, m.f, modules='numpy')

    # for a 4/4 time signature bar length is 60/BPM * 4
    # So the end time can't go over that for a single bar.
    width = 128   # The time axis
    height = len(available_notes) # Number of notes

    notes = []
    for x in range(width):
        t = float(x) / width  # normalized time [0, 1)
        y = float(m_func(t))       # evaluate numeric function
        # Map y in [-1, 1] to note index [0, height-1]
        note_idx = int((y + 1) / 2 * (height - 1))
        notes.append((x, note_idx))
    return notes

def visualize_rest(m: Melody, available):
    notes = rasterize(m, available)

    root = tk.Tk()
    root.title("Rasterized Trig Function")

    canvas_width = 1024
    canvas_height = 512
    canvas = tk.Canvas(root, width=canvas_width, height=canvas_height, bg="white")
    canvas.pack()

    # Draw axes
    canvas.create_line(40, canvas_height - 40, canvas_width - 20, canvas_height - 40, arrow=tk.LAST)
    canvas.create_line(40, canvas_height - 40, 40, 20, arrow=tk.LAST)

    # Plot rasterized points
    for x, note_idx in notes:
        px = 40 + (x / 128) * (canvas_width - 60)
        py = canvas_height - 40 - (note_idx / 49) * (canvas_height - 60)
        canvas.create_oval(px - 2, py - 2, px + 2, py + 2, fill="blue")

    root.mainloop()


def rast_to_midi(rast_notes, available):
    """
    Convert rasterized notes to MIDI format.
    """
    midi_notes = []
    for x, note_idx in rast_notes:
        if 0 <= note_idx < len(available):
            pitch = available[note_idx]
            start_time = x * 0.1
            end_time = start_time + 0.5
            midi_notes.append((pitch, start_time, end_time))
    return midi_notes

# --------------------------------------
# Building midi.
# --------------------------------------

def build_midi(m: Melody):
    """
    Build a simple piano MIDI file.
    Test on browser with https://cifkao.github.io/html-midi-player/
    """
    # Create a PrettyMIDI object
    midi = pretty_midi.PrettyMIDI()

    # Create a Piano instrument (program number 0 is a piano)
    piano = pretty_midi.Instrument(program=0)

    notes = available_notes("C", "Major", (0, 7))
    # rast_notes = rast_to_midi(rasterize(m, notes), notes)
    melody = rast_to_midi(rasterize(m, notes), notes)
    # for i in range(len(notes)):
    #     # Define notes: (pitch: 0-127, start_time, end_time)
    #     melody.append((notes[i], 0.0+(0.1*i), 0.5+(0.1*i)))

    # Add notes to the piano instrument
    for pitch, start, end in melody:
        note = pretty_midi.Note(velocity=100, pitch=pitch, start=start, end=end)
        piano.notes.append(note)

    # Add instrument to the MIDI
    midi.instruments.append(piano)

    # Write to file
    midi.write('piano_melody.mid')
    print("🎵 Saved piano_melody.mid")



if __name__ == "__main__":
    m = Melody(1, 4)
    available = available_notes("C", "Major", (2, 7))

    # rasterize(m, available)
    # visualize_rest(m, available)

    build_midi(m)

"""
Psuedo code for generating midi from trig:
1. Pick a key and scale.
- Get all available notes for all octaves in a certain range like 2-4.
2. Generate a Melody object
3. Somehow map trig to the available notes by evaluating it.
- Cant just evaluate 32nd notes and do 32 evals because I want it to be more realistic
  and have nates in between lines. So maybe find a way to block out a trig function/polynomial.
- Maybe add a random chance of silence so that it is not always continuous playing. 
4. Create and save a midi file with the notes.
5. Bass, lead, and counter melody functions.
Future work:
1. Make drum generation function.
2. Create a way that melodies are built based off of each other.
3. Use AI to learn better melodies, but with unsepervised training and not off of copyrighted music. 
"""