# /// script
# requires-python = ">=3.11"
# dependencies = ["numpy", "scipy"]
# ///
"""
The mechanical kit: metal snaps, hits, air and machine loops synthesised to the anatomy of the mechanical intro
reference (Music/SFX/Reference/MechanicalIntro/README.md), each sound on its own.

Run outside the editor: uv run AI/Python/Audio/mech_kit.py [ship]
Writes every one-shot and loop as SFX_Mech_<Name>.wav into DRAFT, or into SourceArt/Audio/Mech with `ship`.
Scores import it for its sound functions, and hex_intro_mech_score.py builds the hex boss intro from them.
"""
import pathlib
import sys
import zlib
from fractions import Fraction

import numpy as np
from scipy.io import wavfile
from scipy.signal import fftconvolve, resample_poly

PROJECT = pathlib.Path(__file__).resolve().parents[3]
DRAFT = PROJECT / "Saved" / "Audio" / "Mech" / "Draft"
SHIPPED = PROJECT / "SourceArt" / "Audio" / "Mech"
PACKAGE = "/Game/Art/SFX/Mech"
SAMPLE_RATE = 48000
SEED = 20260928
PEAK = 10.0 ** (-1.0 / 20.0)
VARIANTS = 4
LOOP_SECONDS = 4.0
OCTAVES = [125, 250, 500, 1000, 2000, 4000, 8000, 16000]
SIZES = {"Low": 1.3, "Mid": 1.0, "High": 0.8}  # a part's size: every frequency divided by it, the largest lowest

# Per octave in OCTAVES: (level dB, seconds to fall 20 dB), or None
TICK_SNAP = [(-6, 0.017), (-19, 0.022), (-21, 0.016), (-16, 0.035), (-14, 0.06), (-6, 0.035), (-10, 0.02), (-18, 0.02)]
# (Hz, level dB, seconds to fall 20 dB)
TICK_LINES = [(1535, -12, 0.08), (4660, -12, 0.03), (8514, -14, 0.012), (13811, -16, 0.01)]
CLICK_SNAP = [(-15, 0.02), (-21, 0.025), (-16, 0.03), (-8, 0.02), (-3, 0.017), (-3, 0.02), (0, 0.013), (-8, 0.012)]
HIT_SNAP = [(-4, 0.03), (-6, 0.02), (-11, 0.04), (-5, 0.037), (-3, 0.036), (-3, 0.038), (-5, 0.026), (-15, 0.024)]
HIT_STRIKES = (4, 8)      # snaps in one hit's clatter
HIT_CLATTER = 0.045       # seconds the clatter spans
HIT_SMEAR = (-8, 0.2)     # the 2–8 kHz smear: level dB, seconds to fall 20 dB
HIT_LINES = [(1250, -10, 0.22), (2121, -16, 0.14), (5713, -20, 0.09), (6914, -20, 0.09), (7898, -18, 0.07)]
KNOCK = (40, 250, 0.06)   # a landing's knock: low Hz, high Hz, seconds to fall 20 dB — unpitched, never a drum
KNOCK_LEVEL = -4.0        # under the first hit, so the snap leads
FLURRY = (3, 6)           # hits clattering after a landing's first
FLURRY_GAP = (0.03, 0.09)
SPARKLE_SNAP = [None, None, None, (-12, 0.01), (-6, 0.012), (-3, 0.01), (0, 0.008), (-4, 0.006)]
RISER_LINE = (3600.0, -42.0)  # a held line under the sparkle
AIR_COMB = [1200, 2400, 3600, 4800, 6000, 7200]  # a harmonic comb, each line 3 dB under the last
AIR_BURSTS = 4
AIR_RATE = 8.0            # stutters a second
AIR_BLAST = [(-10, 0.5), (-6, 0.55), (-3, 0.5), (0, 0.45), (0, 0.4), (-2, 0.3), (-6, 0.2), (-10, 0.15)]
AIR_HOLD = 0.12
# (low Hz, high Hz, level dB): the sub loudest, a faint air tail above it
DRONE_BANDS = [(26, 46, 0.0), (55, 90, -3.0), (90, 165, -5.0), (165, 350, -14.0), (350, 700, -27.0), (700, 1400, -31.0),
               (1400, 2800, -39.0), (2800, 5600, -51.0), (5600, 11200, -62.0), (11200, 16000, -70.0)]
DRONE_TONES = [(36.0, -2.0), (41.0, -6.0)]
HUM_LINES = [(521.0, 0.0), (1230.0, -6.0), (2197.0, -10.0)]
HUM_BEAT = 2.0            # Hz between a line and its copy
HUM_LOW = 0.75            # the lines' pitch at no pressure, as a share of full
ENGINE_WOOF = (1.0, 4.0)  # woofs a second at rest and flat out: a vane on the big ring sweeping past once a turn
ENGINE_GEAR = 30          # rotor turns per woof, so pitch and woof rate move together: 30 Hz at rest, 120 flat out
ENGINE_ORDERS = 8         # harmonics of the rotor, each 4 dB under the last
ENGINE_CRUISE = 0.8       # the speed the engine settles at and holds through the fight, as in the intro's build
ENGINE_LOOP_WOOFS = 12    # woofs in the fight loop, a multiple of 4 so every hum line closes the loop
ENGINE_WOOF_WIDTH = 0.15  # a woof's width, as a share of a turn
ENGINE_WOOF_DEPTH = 0.7   # how far the level sinks between woofs, the high orders furthest
ENGINE_DOPPLER = 0.015    # the pitch bending up as the vane comes and down as it goes
ENGINE_WOOF_AIR = -10.0   # the air each woof pushes, 80-900 Hz, in dB
ENGINE_FLOOR = 0.25       # its level at rest, as a share of full speed's
WHIR_TICK = [None, None, (-24, 0.016), (-14, 0.035), (-8, 0.05), (-4, 0.03), (-10, 0.02), (-18, 0.02)]  # no knock
WHIR_JITTER = 0.006       # seconds a running tick strays from its tooth
WHIR_WEAK = -6.0          # every other tooth, the second pawl
WHIR_DRIVE_HZ = 110.0     # the drive's tone: its harmonics in WHIR_COMB are the held comb of mid lines
WHIR_COMB = range(5, 19)
WHIR_COMB_LEVEL = -26.0   # the comb against a tick's peak, dB, each harmonic up to 12 dB under it
WHIR_LINES = [(2940.0, -26.0), (5410.0, -32.0)]  # held high lines, dB against a tick's peak
WHIR_HISS = (500, 8000, -40.0)
SURGE_SIZE = 1.6          # the whole machine locking at once, lower than any ring
SURGE_WHUMP = (50, 500, -6.0, 0.5)  # the air pushed out: low Hz, high Hz, level dB, seconds to fall 20 dB
TONE_TAIL = [(1300, -4, 1.3), (3400, 0, 1.6), (4800, -3, 1.2), (7600, -8, 0.8), (8300, -9, 0.7)]


def generator(name):
    """Each sound's own random stream, so re-voicing one never re-rolls another."""
    return np.random.default_rng([SEED, zlib.crc32(name.encode())])


def seconds(value):
    return int(round(value * SAMPLE_RATE))


def timeline(length):
    return np.arange(length) / SAMPLE_RATE


def band_noise(length, low, high, rng):
    """White noise kept between `low` and `high` Hz at unit deviation; circular, so a loop's end runs into its start."""
    spectrum = np.fft.rfft(rng.standard_normal(length))
    frequencies = np.fft.rfftfreq(length, 1.0 / SAMPLE_RATE)
    spectrum[(frequencies < low) | (frequencies > high)] = 0.0
    noise = np.fft.irfft(spectrum, length)
    return noise / noise.std()


def falling(length, fall):
    """An envelope dropping 20 dB every `fall` seconds."""
    return 10.0 ** (-timeline(length) / fall)


def rising(length, attack):
    return np.clip(timeline(length) / attack, 0.0, 1.0)


def decibels(level):
    return 10.0 ** (level / 20.0)


def snap(profile, length, rng):
    """A broadband snap: noise per octave under its own decay, struck within half a millisecond."""
    sound = np.zeros(length)
    for centre, band in zip(OCTAVES, profile):
        if band:
            level, fall = band
            high = min(centre * np.sqrt(2.0), SAMPLE_RATE / 2.0)
            sound += decibels(level) * band_noise(length, centre / np.sqrt(2.0), high, rng) * falling(length, fall)

    return sound * rising(length, 0.0005)


def ring(lines, length, rng):
    """Damped sine lines, each struck at a random phase."""
    time = timeline(length)
    sound = np.zeros(length)
    for frequency, level, fall in lines:
        sound += decibels(level) * np.sin(2 * np.pi * frequency * time + rng.uniform(0, 2 * np.pi)) * falling(length, fall)

    return sound * rising(length, 0.001)


def detuned(lines, rng, spread):
    return [(frequency * rng.uniform(1 - spread, 1 + spread), level, fall) for frequency, level, fall in lines]


def sized(sound, size):
    """The same sound from a part `size` times larger: every frequency divided by it, every time multiplied."""
    ratio = Fraction(size).limit_denominator(20)
    return resample_poly(sound, ratio.numerator, ratio.denominator)


def add(track, sound, start, gain=1.0, wrap=False):
    """Adds `sound` from sample `start`; with `wrap`, what runs past the end comes back in at the start, as in a loop."""
    start = int(start)
    if wrap:
        indices = (start + np.arange(len(sound))) % len(track)
        np.add.at(track, indices, gain * sound)
    else:
        end = min(len(track), start + len(sound))
        track[start:end] += gain * sound[:end - start]


def poisson(rate, rng):
    """Sample indices of random events whose rate a second follows the per-sample curve `rate`."""
    events, time = [], rng.exponential(1.0 / max(rate[0], 0.5))
    while time * SAMPLE_RATE < len(rate):
        index = int(time * SAMPLE_RATE)
        if rng.uniform() < rate[index] / max(rate.max(), 1e-9):
            events.append(index)
        time += rng.exponential(1.0 / max(rate.max(), 1e-9))

    return events


# One-shots

def tick(rng, profile=TICK_SNAP):
    """A ratchet pawl: a bright snap dead within ~20 ms over a short low knock, with short rings high up."""
    length = seconds(0.12)
    return snap(profile, length, rng) + ring(detuned(TICK_LINES, rng, 0.05), length, rng)


def click(rng):
    """A chain link: a dry snap, flat from 200 Hz up and dead within ~17 ms, sometimes with a second link on its heels."""
    length = seconds(0.06)
    sound = snap(CLICK_SNAP, length, rng)
    if rng.uniform() < 0.5:
        add(sound, snap(CLICK_SNAP, length, rng), seconds(rng.uniform(0.002, 0.006)), decibels(rng.uniform(-12, -6)))

    return sound


def hit(rng):
    """A metal hit: a clatter of snaps within ~35 ms, the first hardest, over a 2–8 kHz smear and short rings — no body."""
    length = seconds(0.4)
    sound = np.zeros(length)
    for index in range(rng.integers(*HIT_STRIKES)):
        start = 0 if index == 0 else seconds(rng.uniform(0.003, HIT_CLATTER))
        add(sound, snap(HIT_SNAP, length - start, rng), start, 1.0 if index == 0 else decibels(rng.uniform(-14, -5)))
    smear = band_noise(length, 2000, 8000, rng) * falling(length, HIT_SMEAR[1]) * rising(length, 0.004)
    return sound + decibels(HIT_SMEAR[0]) * smear + ring(detuned(HIT_LINES, rng, 0.04), length, rng)


def knock(rng):
    """An unpitched low knock, the mass of a heavy part meeting another."""
    low, high, fall = KNOCK
    length = seconds(0.3)
    return band_noise(length, low, high, rng) * falling(length, fall) * rising(length, 0.001)


def impact(rng):
    """A landing: a heavy first hit over a low knock, then a few hits clattering within ~300 ms, each softer."""
    sound = np.zeros(seconds(0.9))
    add(sound, hit(rng), 0)
    add(sound, knock(rng), 0, decibels(KNOCK_LEVEL))
    start = 0.0
    for index in range(rng.integers(*FLURRY)):
        start += rng.uniform(*FLURRY_GAP)
        add(sound, hit(rng), seconds(start), decibels(-5.0 - 2.5 * index - rng.uniform(0, 3)))

    return sound


def riser(length, rng):
    """A sting rising into a hard cut: a sub swelling in, sparkle ticks thickening, a high whistle diving to hold."""
    time = timeline(length)
    progress = time / time[-1]
    sub = decibels(-2) * np.sin(2 * np.pi * 36.0 * time) + decibels(-8) * band_noise(length, 26, 60, rng)
    sound = sub * progress ** 2
    sound += decibels(RISER_LINE[1]) * np.clip(progress / 0.3, 0.0, 1.0) * np.sin(2 * np.pi * RISER_LINE[0] * time)
    dive = np.clip((progress - 0.6) / 0.2, 0.0, 1.0)
    whistle = 10000.0 * (5250.0 / 10000.0) ** dive
    sound += decibels(-40 + 12 * progress) * (progress > 0.35) * np.sin(2 * np.pi * np.cumsum(whistle) / SAMPLE_RATE)
    for start in poisson(np.clip(4.0 + 36.0 * (progress - 0.15) / 0.85, 0.0, None) * (progress > 0.15), rng):
        add(sound, snap(SPARKLE_SNAP, seconds(0.03), rng), start, decibels(-22 + 10 * progress[start]
                                                                           + rng.uniform(-3, 3)))
    return sound * rising(length, 0.02) * np.clip((time[-1] - time) / 0.015, 0.0, 1.0)


def grind(rng):
    """Two plates dragging: stick-slip bursts 40–70 a second in noisy mids, swelling in and dying over ~0.7 s."""
    length = seconds(0.7)
    time = timeline(length)
    pulses = np.zeros(length)
    start = 0.0
    while start < time[-1]:
        pulses[seconds(start)] = rng.uniform(0.4, 1.0)
        start += 1.0 / rng.uniform(40, 70)
    grip = fftconvolve(pulses, falling(seconds(0.03), 0.006))[:length]
    noise = band_noise(length, 400, 3000, rng) + decibels(-8) * band_noise(length, 150, 400, rng)
    swell = np.clip(time / 0.06, 0.0, 1.0) * np.clip((time[-1] - time) / 0.35, 0.0, 1.0)
    return noise * (0.3 + grip) * swell


def air_stutter(rng):
    """An air tool stuttering ~8 times a second, each burst broadband over a harmonic comb on 1.2 kHz."""
    length = seconds(AIR_BURSTS / AIR_RATE + 0.16)
    sound = np.zeros(length)
    burst_length = seconds(0.16)
    for index in range(AIR_BURSTS):
        comb = sum(decibels(-3 * line) * band_noise(burst_length, hz * 0.97, hz * 1.03, rng)
                   for line, hz in enumerate(AIR_COMB))
        burst = (comb + decibels(-4) * band_noise(burst_length, 300, 14000, rng)) * falling(burst_length, 0.09)
        add(sound, burst * rising(burst_length, 0.004), seconds(index / AIR_RATE + rng.uniform(0, 0.012)),
            decibels(rng.uniform(-3, 0)))
    return sound


def air_blast(rng):
    """Air blowing out broadband up to 16 kHz, darkening as it dies, loose parts clicking after it."""
    length = seconds(1.2)
    sound = np.zeros(length)
    for centre, (level, fall) in zip(OCTAVES, AIR_BLAST):
        high = min(centre * np.sqrt(2.0), SAMPLE_RATE / 2.0)
        band = decibels(level) * band_noise(length, centre / np.sqrt(2.0), high, rng)
        sound += band * np.concatenate([np.ones(seconds(AIR_HOLD)), falling(length - seconds(AIR_HOLD), fall)])
    sound *= rising(length, 0.008)
    for start in poisson(np.linspace(20.0, 2.0, length) * (timeline(length) > AIR_HOLD), rng):
        add(sound, click(rng), start, decibels(-14 + rng.uniform(-4, 2)))

    return sound


def boom(rng):
    """The bed slamming back in: a sub at 38 Hz and a low noise hit, unpitched above, no glide."""
    length = seconds(3.0)
    time = timeline(length)
    sub = np.sin(2 * np.pi * 38.0 * time) * falling(length, 1.0) + decibels(-12) * np.sin(
        2 * np.pi * 76.0 * time) * falling(length, 0.8)
    low = decibels(-2) * band_noise(length, 25, 140, rng) * falling(length, 0.9)
    fade = np.clip((time[-1] - time) / 0.5, 0.0, 1.0)
    return np.tanh((sub + low) * rising(length, 0.003)) * fade


def surge(rng):
    """The machine coming alive: one heavy hit locking home over a deep knock and a low push of air; no debris, no
    hiss, no ringing tail, so it lands whole rather than breaking."""
    length = seconds(1.5)
    sound = np.zeros(length)
    struck = sized(hit(rng), SURGE_SIZE)
    add(sound, struck * np.clip((len(struck) - np.arange(len(struck))) / seconds(0.25), 0.0, 1.0), 0)
    add(sound, sized(knock(rng), SURGE_SIZE), 0)
    low, high, level, fall = SURGE_WHUMP
    return sound + decibels(level) * band_noise(length, low, high, rng) * falling(length, fall) * rising(length, 0.03)


def break_apart():
    """A machine blowing apart: a landing's flurry, the boom, air blasting out with loose parts clicking, a ring left
    hanging; the same parts as their own files, mixed as the hex boss intro's first release, kept for an outro."""
    sound = np.zeros(seconds(3.0))
    parts = [(sized(impact(generator("Impact_Low")), SIZES["Low"]), 0.0, 1.0), (boom(generator("Boom")), 0.0, 1.0),
             (air_blast(generator("AirBlast")), 0.0, 0.9), (tone_tail(generator("ToneTail")), 0.1, 0.2)]
    for part, start, gain in parts:
        add(sound, normalised(fade_edges(part)), seconds(start), gain)

    return sound


def tone_tail(rng):
    """Pure lines ringing on after a big hit, swelling in as it clears."""
    length = seconds(2.0)
    fade = np.clip((timeline(length)[-1] - timeline(length)) / 0.5, 0.0, 1.0)
    return ring(detuned(TONE_TAIL, rng, 0.01), length, rng) * rising(length, 0.015) * fade


# Continuous sounds: each follows per-sample curves, so a score drives it from motion and a loop holds it steady

def drone(level, rng):
    """The bed: a sub drone at 26–46 Hz with a bass band over it, loudest of everything."""
    length = len(level)
    time = timeline(length)
    sound = sum(decibels(db) * band_noise(length, low, high, rng) for low, high, db in DRONE_BANDS)
    sound += sum(decibels(db) * np.sqrt(2) * np.sin(2 * np.pi * hz * time + rng.uniform(0, 2 * np.pi))
                 for hz, db in DRONE_TONES)
    return level * sound


def hum(amount, rng):
    """Held lines at 521, 1230 and 2197 Hz, each beating against a copy, rising to pitch with the pressure."""
    pitch = HUM_LOW + (1.0 - HUM_LOW) * amount
    sound = np.zeros(len(amount))
    for hz, db in HUM_LINES:
        for offset in (0.0, HUM_BEAT):
            sound += decibels(db) * np.sin(2 * np.pi * np.cumsum((hz + offset) * pitch) / SAMPLE_RATE
                                           + rng.uniform(0, 2 * np.pi))
    return amount * sound


def engine(speed, rng):
    """An electric engine turning at `speed`, 0 to 1: a rotor buzz and the hum lines over it, woofing each time a vane
    sweeps past — louder, brighter, bending in pitch, pushing air; pitch, woof rate and level all follow the speed."""
    length = len(speed)
    full = ENGINE_WOOF[1]
    rate = engine_rate(speed)
    turn = np.cumsum(rate) / SAMPLE_RATE % 1.0 - 0.5
    woof = np.exp(-(turn / ENGINE_WOOF_WIDTH) ** 2)
    bend = 1.0 - ENGINE_DOPPLER * turn / ENGINE_WOOF_WIDTH * woof
    phase = 2 * np.pi * ENGINE_GEAR * np.cumsum(rate * bend) / SAMPLE_RATE
    sound = np.zeros(length)
    for order in range(1, ENGINE_ORDERS + 1):
        depth = ENGINE_WOOF_DEPTH * (0.5 + 0.5 * order / ENGINE_ORDERS)
        sound += decibels(-4.0 * (order - 1)) * (1.0 - depth + depth * woof) * np.sin(
            order * phase + rng.uniform(0, 2 * np.pi))
    for hz, db in HUM_LINES:
        sound += decibels(db - 6.0) * (1.0 - ENGINE_WOOF_DEPTH + ENGINE_WOOF_DEPTH * woof) * np.sin(
            hz / (ENGINE_GEAR * full) * phase + rng.uniform(0, 2 * np.pi))
    sound += decibels(ENGINE_WOOF_AIR) * woof ** 2 * band_noise(length, 80, 900, rng)
    return engine_level(speed) * sound


def engine_rate(speed):
    """Woofs a second at `speed`; every pitch of the engine is a fixed multiple of it."""
    rest, full = ENGINE_WOOF
    return rest + (full - rest) * speed


def engine_level(speed):
    """The engine's level at `speed`: its floor at rest, rising to 1 flat out, and silent when stopped."""
    return (ENGINE_FLOOR + (1.0 - ENGINE_FLOOR) * speed) * np.clip(speed / 0.05, 0.0, 1.0)


def engine_loop_speed():
    """The fight loop's speed and length: the cruise, nudged so the loop holds whole woofs."""
    rest, full = ENGINE_WOOF
    length = seconds(ENGINE_LOOP_WOOFS / engine_rate(ENGINE_CRUISE))
    return (ENGINE_LOOP_WOOFS * SAMPLE_RATE / length - rest) / (full - rest), length


def rattle(amount, rng, size=1.0, wrap=False):
    """Chain links rattling, 4–26 clicks a second and louder as `amount` climbs to 1."""
    sound = np.zeros(len(amount))
    for start in poisson(4.0 + 22.0 * amount, rng):
        add(sound, sized(click(rng), size), start, np.sqrt(amount[start]) * decibels(rng.uniform(-6, 0)), wrap)

    return sound


def whir(teeth, drive, size, rng, wrap=False):
    """A gear train running: a light tick per tooth, strong and weak in turn, over a held comb of mid lines, high
    lines and a hiss, all following `drive`, the speed from 0 to 1."""
    length = len(drive)
    sound = np.zeros(length)
    for index, start in enumerate(teeth):
        level = decibels((WHIR_WEAK if index % 2 else 0.0) + rng.uniform(-4, 2)) * (0.3 + 0.7 * drive[start])
        add(sound, sized(tick(rng, WHIR_TICK), size), max(0, start + seconds(rng.normal(0, WHIR_JITTER))), level, wrap)
    pitch = (0.85 + 0.15 * drive) / size
    lines = [(WHIR_DRIVE_HZ * harmonic, WHIR_COMB_LEVEL - rng.uniform(0, 12)) for harmonic in WHIR_COMB] + WHIR_LINES
    for hz, level in lines:
        frequency = hz * pitch
        if wrap:  # whole cycles over the loop, so each line runs into its own start
            frequency = np.round(frequency * length / SAMPLE_RATE) * SAMPLE_RATE / length
        sound += decibels(level) * drive * np.sin(2 * np.pi * np.cumsum(frequency) / SAMPLE_RATE
                                                  + rng.uniform(0, 2 * np.pi))
    low, high, level = WHIR_HISS
    return sound + decibels(level) * drive * band_noise(length, low / size, high / size, rng)


def one_shots():
    shots = {"Riser": riser(seconds(1.0), generator("Riser")), "Grind": grind(generator("Grind")),
             "AirStutter": air_stutter(generator("AirStutter")), "AirBlast": air_blast(generator("AirBlast")),
             "Boom": boom(generator("Boom")), "ToneTail": tone_tail(generator("ToneTail")),
             "Surge": surge(generator("Surge")), "Break": break_apart()}
    for index in range(VARIANTS):
        for name, sound in (("Tick", tick), ("Click", click), ("Hit", hit)):
            shots[f"{name}_{index + 1}"] = sound(generator(f"{name}_{index + 1}"))
    for size_name, size in SIZES.items():
        shots[f"Impact_{size_name}"] = sized(impact(generator(f"Impact_{size_name}")), size)

    return shots


def loops():
    """Seamless loops: a drone, a hum, a rattle, a gear train and the engine at its cruise, each held steady."""
    length = seconds(LOOP_SECONDS)
    steady = np.ones(length)
    teeth = [seconds(index / 10.0) for index in range(int(LOOP_SECONDS * 10))]
    cruise, cruise_length = engine_loop_speed()
    return {"Engine_Loop": engine(np.full(cruise_length, cruise), generator("Engine_Loop")),
            "Drone_Loop": drone(steady, generator("Drone_Loop")), "Hum_Loop": hum(steady, generator("Hum_Loop")),
            "Rattle_Loop": rattle(0.8 * steady, generator("Rattle_Loop"), wrap=True),
            "Whir_Loop": whir(teeth, steady, 1.0, generator("Whir_Loop"), wrap=True)}


def normalised(sound):
    return sound / np.abs(sound).max() * PEAK


def save(path, sound):
    """Writes 16-bit stereo at −1 dBFS peak."""
    wavfile.write(path, SAMPLE_RATE, np.round(np.stack([sound, sound], axis=1) * 32767).astype(np.int16))
    return path


def fade_edges(sound):
    """3 ms fades, for a one-shot; a loop keeps its edges."""
    edge = np.clip(np.minimum(np.arange(len(sound)), np.arange(len(sound))[::-1]) / seconds(0.003), 0.0, 1.0)
    return sound * edge


def write_all(folder):
    folder.mkdir(parents=True, exist_ok=True)
    paths = {}
    for name, sound in one_shots().items():
        paths[name] = save(folder / f"SFX_Mech_{name}.wav", normalised(fade_edges(sound)))
    for name, sound in loops().items():
        paths[name] = save(folder / f"SFX_Mech_{name}.wav", normalised(sound))

    return paths


if __name__ == "__main__":
    for path in write_all(SHIPPED if sys.argv[1:] == ["ship"] else DRAFT).values():
        print(path)
