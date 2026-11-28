#!/usr/bin/env python3
"""3D の地図ビュー用の地形テクスチャを生成する。

godot_project/assets/textures/terrain/<名前>.png (128x128、継ぎ目なしで繰り返せるグレースケール) を作る。
色は 3D ビュー側で端末の地形の色を掛けて付けるので、ここでは明暗と模様だけを描く。
乱数の種は固定なので、何度実行しても同じ画像になる。

使い方: python3 tools/godot/gen-terrain-textures.py
依存: numpy, Pillow
"""

import pathlib

import numpy as np
from PIL import Image

SIZE = 128
OUT = pathlib.Path(__file__).resolve().parents[2] / "godot_project" / "assets" / "textures" / "terrain"
rng = np.random.default_rng(9290)
yy, xx = np.mgrid[0:SIZE, 0:SIZE].astype(np.float64)


def noise(scale: float, seed_offset: int = 0) -> np.ndarray:
    """継ぎ目なしで繰り返せるノイズ (周波数空間で作るので周期的)。scale が大きいほど粗い。0..1"""
    local = np.random.default_rng(9290 + seed_offset)
    spectrum = local.normal(size=(SIZE, SIZE)) + 1j * local.normal(size=(SIZE, SIZE))
    fy = np.fft.fftfreq(SIZE)[:, None]
    fx = np.fft.fftfreq(SIZE)[None, :]
    f = np.sqrt(fx * fx + fy * fy)
    f[0, 0] = 1.0
    spectrum *= np.exp(-(f * scale) ** 2) / f**0.5
    spectrum[0, 0] = 0
    field = np.real(np.fft.ifft2(spectrum))
    return (field - field.min()) / (field.max() - field.min())


def fbm(seed: int) -> np.ndarray:
    return 0.5 * noise(24, seed) + 0.3 * noise(8, seed + 1) + 0.2 * noise(2, seed + 2)


def save(name: str, value: np.ndarray) -> None:
    img = np.clip(value, 0.0, 1.0) * 255
    Image.fromarray(img.astype(np.uint8), "L").convert("RGB").save(OUT / f"{name}.png", optimize=True)
    print("wrote", OUT / f"{name}.png")


def mortar_grid(rows: int, cols: int, offset_rows: bool, width: float, jitter_seed: int, warp: float = 0.0):
    """石・煉瓦の目地 (0 = 目地, 1 = 石の中) と、石ごとの明るさのばらつきを返す。
    warp を与えると目地を継ぎ目なしのノイズで歪ませる (荒く積んだ石)"""
    global yy, xx
    orig_y, orig_x = yy, xx
    if warp > 0:
        yy = (orig_y + (noise(30, jitter_seed + 500) - 0.5) * warp) % SIZE
        xx = (orig_x + (noise(30, jitter_seed + 501) - 0.5) * warp) % SIZE
    cell_h = SIZE / rows
    cell_w = SIZE / cols
    row = np.floor(yy / cell_h)
    shift = np.where((row % 2 == 1) & offset_rows, cell_w / 2, 0.0)
    x = (xx + shift) % SIZE
    col = np.floor(x / cell_w)
    dy = np.minimum(yy % cell_h, cell_h - yy % cell_h)
    dx = np.minimum(x % cell_w, cell_w - x % cell_w)
    edge = np.minimum(dx, dy)
    stone = np.clip((edge - width * 0.5) / 1.5, 0.0, 1.0)
    local = np.random.default_rng(jitter_seed)
    tone_table = local.uniform(0.82, 1.0, size=(rows, cols))
    tone = tone_table[row.astype(int) % rows, col.astype(int) % cols]
    yy, xx = orig_y, orig_x
    return stone, tone, edge


def floor() -> np.ndarray:
    stone, tone, edge = mortar_grid(2, 2, True, 3.0, 1, warp=6.0)
    n = fbm(10)
    bevel = np.clip(edge / 5.0, 0.0, 1.0)
    return (0.45 + 0.35 * n) * tone * (0.75 + 0.25 * bevel) * (0.35 + 0.65 * stone) + 0.05


def wall() -> np.ndarray:
    # 荒く積んだ石: 高さの違う段に、長さの違う石
    n = fbm(20)
    stone, tone, edge = mortar_grid(3, 2, True, 3.5, 2, warp=16.0)
    bevel = np.clip(edge / 6.0, 0.0, 1.0)
    return (0.5 + 0.4 * n) * tone * (0.7 + 0.3 * bevel) * (0.3 + 0.7 * stone) + 0.05


def permanent() -> np.ndarray:
    stone, tone, edge = mortar_grid(2, 2, True, 2.0, 3)
    n = fbm(30)
    bevel = np.clip(edge / 8.0, 0.0, 1.0)
    return (0.55 + 0.2 * n) * tone * (0.65 + 0.35 * bevel) * (0.4 + 0.6 * stone) + 0.1


def brick() -> np.ndarray:
    stone, tone, edge = mortar_grid(8, 4, True, 2.5, 4)
    n = fbm(40)
    return (0.55 + 0.3 * n) * tone * (0.45 + 0.55 * stone) + 0.05


def vein() -> np.ndarray:
    # 岩脈: 斜めの縞と粗いノイズ
    n = fbm(50)
    streak = 0.5 + 0.5 * np.sin((xx + yy) * 2 * np.pi * 3 / SIZE + noise(16, 51) * 6)
    return 0.35 + 0.35 * n + 0.2 * streak


def door() -> np.ndarray:
    # 縦の板と 2 本の鉄の帯
    planks = 5
    w = SIZE / planks
    gap = np.minimum(xx % w, w - xx % w)
    seam = np.clip((gap - 1.0) / 1.5, 0.0, 1.0)
    grain = 0.5 + 0.5 * np.sin(xx * 2 * np.pi * 9 / SIZE + noise(10, 61) * 8)
    tone_table = np.random.default_rng(62).uniform(0.85, 1.0, size=planks)
    tone = tone_table[(xx // w).astype(int) % planks]
    value = (0.55 + 0.15 * grain + 0.15 * noise(4, 63)) * tone * (0.4 + 0.6 * seam)
    band = (np.abs(yy - SIZE * 0.25) < 5) | (np.abs(yy - SIZE * 0.75) < 5)
    value = np.where(band, 0.3 + 0.1 * noise(2, 64), value)
    rivet = band & (np.abs((xx % w) - w / 2) < 2.5) & ((np.abs(yy - SIZE * 0.25) < 2.5) | (np.abs(yy - SIZE * 0.75) < 2.5))
    return np.where(rivet, 0.75, value)


def wood_wall() -> np.ndarray:
    # 横に積んだ丸太
    rows = 6
    h = SIZE / rows
    d = (yy % h) / h
    log = np.sin(d * np.pi) ** 0.6
    grain = 0.5 + 0.5 * np.sin(yy * 2 * np.pi * 14 / SIZE + noise(12, 71) * 5)
    return (0.35 + 0.45 * log) * (0.8 + 0.2 * grain)


def stairs(down: bool) -> np.ndarray:
    # 段: 上りは手前が低く (明→暗)、下りは奥へ暗くなる
    steps = 5
    h = SIZE / steps
    k = np.floor(yy / h)
    frac = (yy % h) / h
    base = (k + 1) / steps if not down else 1.0 - k / steps
    nosing = np.where(frac < 0.12, 1.15, 1.0)
    return (0.25 + 0.6 * base) * nosing * (0.85 + 0.15 * fbm(80 + down))


def liquid() -> np.ndarray:
    ripple = 0.5 + 0.5 * np.sin((xx * 2 + yy) * 2 * np.pi * 2 / SIZE + noise(20, 91) * 7)
    return 0.55 + 0.25 * ripple + 0.15 * noise(6, 92)


def grass() -> np.ndarray:
    blades = np.random.default_rng(101).random((SIZE, SIZE))
    return 0.45 + 0.25 * noise(10, 102) + 0.25 * (blades > 0.7) * noise(1.5, 103)


def dirt() -> np.ndarray:
    pebbles = (noise(1.5, 111) > 0.8) * 0.15
    return 0.45 + 0.35 * fbm(112) + pebbles


def rubble() -> np.ndarray:
    # 重なった石塊: ボロノイの距離で丸い塊を作る
    points = np.random.default_rng(121).random((24, 2)) * SIZE
    dist = np.full((SIZE, SIZE), 1e9)
    for py, px in points:
        for oy in (-SIZE, 0, SIZE):
            for ox in (-SIZE, 0, SIZE):
                dist = np.minimum(dist, np.hypot(yy - py - oy, xx - px - ox))
    lump = np.clip(1.0 - dist / 22.0, 0.0, 1.0)
    return 0.2 + 0.6 * lump * (0.7 + 0.3 * fbm(122))


def tree() -> np.ndarray:
    # 樹皮: 縦の深い溝
    groove = 0.5 + 0.5 * np.sin(xx * 2 * np.pi * 7 / SIZE + noise(14, 131) * 9)
    return 0.3 + 0.4 * groove ** 2 + 0.25 * noise(3, 132)


def plaster() -> np.ndarray:
    # 建物の漆喰の壁
    return 0.7 + 0.2 * fbm(141)


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    textures = {
        "floor": floor(),
        "wall": wall(),
        "permanent": permanent(),
        "brick": brick(),
        "vein": vein(),
        "door": door(),
        "wood_wall": wood_wall(),
        "stairs_up": stairs(False),
        "stairs_down": stairs(True),
        "liquid": liquid(),
        "grass": grass(),
        "dirt": dirt(),
        "rubble": rubble(),
        "tree": tree(),
        "plaster": plaster(),
    }
    for name, value in textures.items():
        save(name, value)


if __name__ == "__main__":
    main()
