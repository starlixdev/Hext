"""Inspect pixels exported by the native renderer; requires Pillow and numpy."""
from pathlib import Path
import numpy as np
from PIL import Image

root = Path(__file__).resolve().parents[1] / "dist"

def ink_center(file, rect, threshold=0.25):
    x, y, w, h = rect
    image = np.array(Image.open(root / file).convert("RGB"), dtype=float)
    crop = image[y + 4:y + h - 4, x + 8:x + w - 8]
    background = np.median(crop, axis=(0, 1))
    delta = np.max(np.abs(crop - background), axis=2)
    mask = delta > max(15, float(delta.max()) * threshold)
    rows, columns = np.where(mask)
    assert len(rows), f"No visible glyphs: {file}"
    return (x + 8 + (columns.min() + columns.max() + 1) / 2,
            y + 4 + (rows.min() + rows.max() + 1) / 2)

for name, file, rect in [
    ("Cancelar", "color-wheel.png", (410, 401, 134, 32)),
    ("Aplicar", "color-wheel.png", (558, 401, 134, 32)),
    ("Conectar", "assistant-connect.png", (220, 326, 160, 32)),
]:
    actual = ink_center(file, rect)
    x, y, w, h = rect
    offset = (actual[0] - x - w / 2, actual[1] - y - h / 2)
    assert max(abs(v) for v in offset) <= 1.1, (name, offset)
    print(f"PASS: {name} visible text centered, pixel offset {offset}")

def bright_bounds(file, rect):
    x, y, w, h = rect
    image = np.array(Image.open(root / file).convert("RGB"))
    crop = image[y:y + h, x:x + w]
    rows, columns = np.where(np.min(crop, axis=2) > 150)
    assert len(rows)
    return (x + (columns.min() + columns.max() + 1) / 2,
            y + (rows.min() + rows.max() + 1) / 2)

icon = bright_bounds("preview-0.png", (430, 170, 56, 54))
label = bright_bounds("preview-0.png", (204, 236, 508, 30))
assert abs(icon[0] - 458) <= 1.1 and abs(label[0] - 458) <= 1.1
assert abs(icon[0] - label[0]) <= 1.1
print("PASS: Assistant icon and empty-state message share content center")

for file in ["assistant-send.png", "assistant-stop.png"]:
    image = np.array(Image.open(root / file).convert("RGB"))
    crop = image[380:402, 676:698]
    rows, columns = np.where(np.max(crop, axis=2) < 100)
    assert len(rows)
    center = (676 + (columns.min() + columns.max() + 1) / 2,
              380 + (rows.min() + rows.max() + 1) / 2)
    assert abs(center[0] - 687) <= 1.1 and abs(center[1] - 391) <= 1.1
    print(f"PASS: {file} vector glyph centered at {center}")
