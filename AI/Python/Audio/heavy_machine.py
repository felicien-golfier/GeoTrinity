"""
Heavy futuristic machine sounds: the building blocks and the one-shots of the Machine family.

Imported by AI/Python/Audio/hex_intro_score.py, which writes the one-shots beside its stems. Weight comes from the
low end and from dense resonances dying fast (AI/MCP/Audacity/GameSoundDesign.md, Weight and scale): every part has
a size, and every frequency of that part is divided by it.
"""
import zlib

import numpy as np
from scipy.signal import butter, fftconvolve, lfilter, sosfilt

SAMPLE_RATE = 48000
HIGH_CUT_HZ = 16500.0

# (name, size): larger parts lower
SIZES = [("Low", 1.3), ("Mid", 1.0), ("High", 0.8)]
GEAR_THUNK_VARIANTS = 4
# An electric motor under load, matched by ear to an electric car accelerating
SERVO_FULL_SPEED = 640.0  # degrees a second, the fastest ring
SERVO_FULL_LOAD = 300.0   # degrees a second squared at which the motor pulls hardest
SERVO_FULL_HZ = 480.0     # the motor's first order at full speed, divided by the part's size
SERVO_REST = 0.35         # its first order at rest, as a share of full speed's
SERVO_PITCH_LAG = 0.4     # seconds
SERVO_LEVEL_LAG = 0.03
SERVO_WHINE = 0.3         # the orders against the drive rumble
# (order, level in dB): the last one is electromagnetic, off the harmonic series
SERVO_ORDERS = [(1.0, 0.0), (2.0, -6.0), (3.0, -8.0), (4.7, -4.0)]
# (pitch against the first rotor, level in dB): two rotors a little apart beat against each other
SERVO_ROTORS = [(1.0, 0.0), (1.06, -4.0)]
SLAM_SEAT_MS = 22.0  # latch catch to the mass seating, times the part's size
SEED = 3


def generator(name):
    """The random stream of one named sound, so re-voicing one sound never re-rolls another."""
    return np.random.default_rng([SEED, zlib.crc32(name.encode())])


def seconds(length):
    return np.arange(int(length * SAMPLE_RATE)) / SAMPLE_RATE


def delayed(sound, delay_ms):
    return np.concatenate([np.zeros(int(delay_ms / 1000.0 * SAMPLE_RATE)), sound])[:len(sound)]


def burst(low_hz, high_hz, level_db, decay_ms, length, rng, attack_ms=0.0):
    """Band-passed noise struck and decaying exponentially."""
    time = seconds(length)
    noise = sosfilt(butter(4, [low_hz, min(high_hz, HIGH_CUT_HZ)], "bp", fs=SAMPLE_RATE, output="sos"),
                    rng.normal(size=len(time)))
    rise = np.clip(time / (attack_ms / 1000.0), 0.0, 1.0) if attack_ms else 1.0
    return 10 ** (level_db / 20) * noise / noise.std() * np.exp(-time / (decay_ms / 1000.0)) * rise


def modes(low_hz, high_hz, count, loss, length, rng):
    """A part's resonances spread between two frequencies, each dying over 1 / (pi * Hz * loss) s: dense and lossy
    reads as a heavy mass, sparse and ringing as a bell."""
    time = seconds(length)
    frequencies = np.exp(rng.uniform(np.log(low_hz), np.log(high_hz), count))
    levels = -3.0 * np.log2(frequencies / low_hz) + rng.normal(0.0, 3.0, count)
    out = np.zeros_like(time)
    for frequency, level in zip(frequencies[frequencies < HIGH_CUT_HZ], levels[frequencies < HIGH_CUT_HZ]):
        out += 10 ** (level / 20) * np.exp(-time * np.pi * frequency * loss) * np.sin(
            2 * np.pi * frequency * time + rng.uniform(0, 2 * np.pi))

    return out * np.clip(time / 0.0002, 0.0, 1.0) / np.sqrt(count)


def thud(start_hz, end_hz, glide_ms, decay_ms, length):
    """A heavy mass's fundamental, sagging in pitch as it dies: the weight under a strike."""
    time = seconds(length)
    frequency = end_hz + (start_hz - end_hz) * np.exp(-time / (glide_ms / 1000.0))
    return (np.sin(2 * np.pi * np.cumsum(frequency) / SAMPLE_RATE) * np.exp(-time / (decay_ms / 1000.0))
            * np.clip(time / 0.0015, 0.0, 1.0))


def tone(frequency, cutoff_hz, harmonics=24):
    """A band-limited sawtooth following a per-sample pitch, its harmonics rolled off above a per-sample cutoff."""
    phase = 2 * np.pi * np.cumsum(frequency) / SAMPLE_RATE
    out = np.zeros_like(phase)
    for k in range(1, harmonics + 1):
        partial = k * frequency
        out += np.where(partial < HIGH_CUT_HZ, 1.0 / k / np.sqrt(1.0 + (partial / cutoff_hz) ** 4), 0.0) * np.sin(
            k * phase)

    return out


def saturate(sound, drive):
    """Soft clipping: thickens a layer with harmonics and holds its body up against its peak."""
    return np.tanh(drive * sound / np.abs(sound).max()) / np.tanh(drive)


def low_band(low_hz, high_hz, length, rng):
    return sosfilt(butter(2, [low_hz, high_hz], "bp", fs=SAMPLE_RATE, output="sos"), rng.normal(size=int(length)))


def latch(size, length, rng):
    """A latch catching: a bright snap over a few short metal modes."""
    return burst(1500, 9000, 0, 0.8, length, rng) + 0.4 * modes(1200 / size, 5000 / size, 10, 0.04, length, rng)


def thunk(size, length, rng):
    """A heavy mass seating: an unpitched low knock and dense modes dying within tens of milliseconds, no ring."""
    time = seconds(length)
    knock = low_band(50 / size, 350 / size, len(time), rng)
    return (knock / knock.std() * np.clip(time / 0.0015, 0, 1) * np.exp(-time / 0.04)
            + 0.7 * modes(120 / size, 900 / size, 30, 0.06, length, rng))


def slam(size, rng):
    """Two heavy parts clipping together, a ker-chunk: the latch catches, the mass seats a beat later under the full
    latch, the panel rattles briefly after; the air pushed ahead of it swells in just before the seat."""
    length = 0.45
    seat_ms = SLAM_SEAT_MS * size
    time = seconds(length)
    air = low_band(40, 200, len(time), rng)
    push = np.clip(time / (seat_ms / 1000.0), 0, 1) ** 3 * (time < seat_ms / 1000.0)
    rattle = modes(300 / size, 1500 / size, 12, 0.05, length, rng)
    return saturate(0.5 * latch(size, length, rng)
                    + delayed(thunk(size, length, rng) + latch(size, length, rng), seat_ms)
                    + 0.15 * delayed(rattle, seat_ms + 35.0)
                    + 0.3 * air / air.std() * push, 1.5)


def hiss(rng):
    """A pneumatic seal venting: bright air swelling in over 25 ms and dying over a quarter second."""
    return burst(2500, 9000, 0, 180, 0.9, rng, attack_ms=25) + burst(900, 3000, -8, 260, 0.9, rng, attack_ms=40)


def blast(rng):
    """The machine's release: a deep sub drop, a broadband strike, a huge low hull and its debris of air."""
    return saturate(thud(70, 26, 250, 900, 3.0)
                    + 0.9 * modes(45, 700, 36, 0.006, 3.0, rng)
                    + burst(80, 16000, 0, 60, 3.0, rng)
                    + burst(200, 3000, -10, 500, 3.0, rng, attack_ms=10), 3.0)


def gear_thunk(rng):
    """One heavy gear tooth dropping into mesh: a dull low clunk with the barest metal edge."""
    return (modes(110, 650, 12, 0.035, 0.25, rng) + 0.5 * thud(90, 70, 5, 40, 0.25)
            + burst(1500, 6000, -18, 1.5, 0.25, rng))


def groan(amount, size, rng):
    """Metal under strain, per sample of `amount` (0 to 1): stick-slip pulses, faster and harder as the strain grows,
    each ringing a large plate's low resonances."""
    rate = (18.0 + 40.0 * amount) * rng.uniform(0.7, 1.3, len(amount))
    pulses = np.zeros(len(amount))
    slips = np.flatnonzero(np.diff(np.floor(np.cumsum(rate) / SAMPLE_RATE)))
    pulses[slips] = amount[slips] * rng.uniform(0.6, 1.0, len(slips))
    plate = modes(80 / size, 700 / size, 24, 0.004, 0.5, rng)
    return fftconvolve(pulses, plate)[:len(amount)] + 0.1 * amount * low_band(60 / size, 600 / size, len(amount), rng)


def hum(amount, size):
    """A holding field's low electric hum, per sample of `amount` (0 to 1)."""
    frequency = np.full(len(amount), 50.0 / size)
    return amount * (tone(frequency, 500.0) + 0.6 * tone(frequency * 1.006, 500.0))


def lagged(values, seconds_to_follow):
    """`values` followed through a one-pole lag: a heavy part's inertia."""
    step = 1.0 - np.exp(-1.0 / (seconds_to_follow * SAMPLE_RATE))
    return lfilter([step], [1.0, step - 1.0], values, zi=[values[0] * (1.0 - step)])[0]


def servo(speed, size, rng):
    """A heavy electric motor turning a part, per sample of its turn speed in degrees a second: a wide low drive
    rumble under one family of orders locked to the speed, from two rotors beating a little apart. The orders swell
    while the motor pulls the speed up and sink while it coasts. The pitch trails the speed through the motor's
    inertia; the level follows it closely."""
    length = len(speed)
    time = np.arange(length) / SAMPLE_RATE
    turning = lagged(np.abs(speed), SERVO_PITCH_LAG)
    drive = np.clip(turning / SERVO_FULL_SPEED, 0.0, 1.0)
    level = np.sqrt(np.clip(lagged(np.abs(speed), SERVO_LEVEL_LAG) / SERVO_FULL_SPEED, 0.0, 1.0)) * np.clip(
        time / 0.02, 0.0, 1.0)
    load = np.clip(lagged(np.gradient(turning) * SAMPLE_RATE / SERVO_FULL_LOAD, 0.1), 0.0, 1.0)
    drift = 1.0 + 0.003 * np.sin(2 * np.pi * rng.uniform(0.2, 0.7) * time + rng.uniform(0, 2 * np.pi))
    first = SERVO_FULL_HZ / size * (SERVO_REST + (1.0 - SERVO_REST) * drive) * drift
    orders = np.zeros(length)
    for rotor, rotor_db in SERVO_ROTORS:
        phase = 2 * np.pi * np.cumsum(rotor * first) / SAMPLE_RATE
        for order, order_db in SERVO_ORDERS:
            orders += 10 ** ((rotor_db + order_db) / 20) * np.sin(order * phase + rng.uniform(0, 2 * np.pi))
    rumble = low_band(35 / size, 170 / size, length, rng)
    air = low_band(2000, 6000, length, rng)
    return level * (rumble / rumble.std() * (0.6 + 0.4 * drive) + SERVO_WHINE * (0.5 + load) * orders
                    + 0.05 * air / air.std())


def rumble(amount, rng):
    """The whole body shaking, per sample of `amount` (0 to 1): low noise swelling with the shake."""
    return amount * low_band(30, 200, len(amount), rng)


def one_shots():
    """Every one-shot of the family by name."""
    sounds = {f"Slam_{name}": slam(size, generator(f"Slam_{name}")) for name, size in SIZES}
    sounds["Hiss"] = hiss(generator("Hiss"))
    sounds["Blast"] = blast(generator("Blast"))
    for index in range(GEAR_THUNK_VARIANTS):
        sounds[f"GearThunk_{index + 1}"] = gear_thunk(generator(f"GearThunk_{index + 1}"))

    return sounds
