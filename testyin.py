import librosa
import numpy as np 

# Load your WAV file
y, sr = librosa.load("tests/test.wav", sr=None)

# Extract pitch using YIN / pYIN method
f0, voiced_flag, voiced_probs = librosa.pyin(
    y, fmin=75, fmax=300, sr=sr, frame_length=1024, win_length=1024
)

# Remove unvoiced frames
f0_clean = f0[~np.isnan(f0)]

# Show pitch stats
print("Mean pitch (Hz):", np.mean(f0_clean))
print("Median pitch (Hz):", np.median(f0_clean))
