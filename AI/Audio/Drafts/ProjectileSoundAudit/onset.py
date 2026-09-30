import sys, numpy as np
from scipy.io import wavfile
for p in sys.argv[1:]:
    sr, d = wavfile.read(p)
    d = d.astype(np.float64)
    if d.ndim > 1: d = d.mean(axis=1)
    d /= max(np.abs(d).max(), 1e-9)
    w = int(sr*0.002)
    env = np.sqrt(np.convolve(d**2, np.ones(w)/w, mode="same"))
    peak_t = env.argmax()/sr
    def first(db):
        idx = np.nonzero(env > 10**(db/20)*env.max())[0]
        return idx[0]/sr*1000 if len(idx) else None
    print(f"{p.split('/')[-1]:45s} sr={sr} len={len(d)/sr*1000:6.0f}ms  -40dB@{first(-40):6.1f}ms  -20dB@{first(-20):6.1f}ms  -6dB@{first(-6):6.1f}ms  peak@{peak_t*1000:6.1f}ms")
