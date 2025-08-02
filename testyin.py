import librosa
import numpy as np
import matplotlib.pyplot as plt

# Load your WAV file
y, sr = librosa.load("tests/110.wav", sr=None)

# Extract pitch using YIN / pYIN method
f0, voiced_flag, voiced_probs = librosa.pyin(
    y, fmin=75, fmax=300, sr=sr, frame_length=1024, win_length=1024
)

# Remove unvoiced frames
f0_clean = f0[~np.isnan(f0)]

# Show pitch stats
print("Mean pitch (Hz):", np.mean(f0_clean))
print("Median pitch (Hz):", np.median(f0_clean))

# Plot pitch over time
plt.figure(figsize=(10, 4))
plt.plot(f0, label='Pitch (Hz)')
plt.xlabel("Frame")
plt.ylabel("Frequency (Hz)")
plt.title("Pitch Over Time")
plt.grid()
plt.legend()
plt.show()
