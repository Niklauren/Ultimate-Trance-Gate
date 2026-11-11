"""Minimal layered PSD writer (RGB, 8-bit) with groups, hidden layers, opacity and Unicode names.
Layers are PIL RGBA images placed at (left, top). Written with PackBits (RLE) compression."""
import struct
import numpy as np


def packbits(row: bytes) -> bytes:
    out = bytearray()
    n = len(row)
    i = 0
    while i < n:
        # run of identical bytes
        j = i + 1
        while j < n and j - i < 128 and row[j] == row[i]:
            j += 1
        if j - i >= 3:
            out.append((257 - (j - i)) & 0xFF)
            out.append(row[i])
            i = j
            continue
        # literal run
        j = i
        while j < n and j - i < 128:
            if j + 2 < n and row[j] == row[j + 1] == row[j + 2]:
                break
            j += 1
        out.append(j - i - 1)
        out += row[i:j]
        i = j
    return bytes(out)


def rle_channel(plane: np.ndarray):
    """plane: HxW uint8 -> (row byte counts, packed data) for one channel"""
    counts, data = [], bytearray()
    for r in range(plane.shape[0]):
        p = packbits(plane[r].tobytes())
        counts.append(len(p))
        data += p
    return counts, bytes(data)


def pascal(name: str, pad: int) -> bytes:
    b = name.encode('mac_roman', 'replace')[:255]
    s = bytes([len(b)]) + b
    while len(s) % pad:
        s += b'\0'
    return s


def block(key: bytes, payload: bytes) -> bytes:
    if len(payload) % 2:
        payload += b'\0'
    return b'8BIM' + key + struct.pack('>I', len(payload)) + payload


def luni(name: str) -> bytes:
    u = name.encode('utf-16-be')
    return block(b'luni', struct.pack('>I', len(name)) + u)


class Layer:
    def __init__(self, name, image=None, left=0, top=0, visible=True, opacity=255):
        self.name, self.image, self.left, self.top = name, image, left, top
        self.visible, self.opacity = visible, opacity


class Group:
    def __init__(self, name, children, visible=True, opacity=255, open_=True):
        self.name, self.children, self.visible, self.opacity, self.open = name, children, visible, opacity, open_


def _flatten(items):
    """Top-first tree -> list of records bottom-first, as PSD stores them."""
    recs = []
    for it in reversed(items):          # bottom first
        if isinstance(it, Group):
            recs.append(('end', it))
            recs += _flatten(it.children)
            recs.append(('group', it))
        else:
            recs.append(('pixel', it))
    return recs


def _record(kind, it):
    if kind == 'pixel' and it.image is not None:
        a = np.asarray(it.image.convert('RGBA'))
        alpha = a[:, :, 3]
        ys, xs = np.nonzero(alpha)
        if len(ys):
            y0, y1, x0, x1 = ys.min(), ys.max() + 1, xs.min(), xs.max() + 1
            a = a[y0:y1, x0:x1]
            top, left = it.top + int(y0), it.left + int(x0)
            bottom, right = top + a.shape[0], left + a.shape[1]
            planes = {-1: a[:, :, 3], 0: a[:, :, 0], 1: a[:, :, 1], 2: a[:, :, 2]}
        else:
            top = left = bottom = right = 0
            planes = None
    else:
        top = left = bottom = right = 0
        planes = None

    chans = []
    for cid in (-1, 0, 1, 2):
        if planes is None:
            data = struct.pack('>H', 0)
        else:
            counts, packed = rle_channel(np.ascontiguousarray(planes[cid]))
            data = struct.pack('>H', 1) + struct.pack('>%dH' % len(counts), *counts) + packed
        chans.append((cid, data))

    name = '</Layer group>' if kind == 'end' else it.name
    flags = 0x08 | (0x02 if not it.visible else 0)
    if kind == 'end':
        flags = 0x08
    extra = struct.pack('>I', 0) + struct.pack('>I', 0) + pascal(name, 4) + luni(name)
    if kind == 'group':
        extra += block(b'lsct', struct.pack('>I', 1 if it.open else 2))
    elif kind == 'end':
        extra += block(b'lsct', struct.pack('>I', 3))

    rec = struct.pack('>4i', top, left, bottom, right)
    rec += struct.pack('>H', 4)
    for cid, data in chans:
        rec += struct.pack('>hI', cid, len(data))
    rec += b'8BIM' + (b'pass' if kind == 'group' else b'norm')
    rec += struct.pack('>BBBB', it.opacity if kind != 'end' else 255, 0, flags, 0)
    rec += struct.pack('>I', len(extra)) + extra
    return rec, b''.join(d for _, d in chans)


def write_psd(path, width, height, tree, composite):
    """tree: list of Layer/Group, TOP first. composite: PIL image of the flattened result."""
    recs = _flatten(tree)
    rec_bytes, chan_bytes = b'', b''
    for kind, it in recs:
        r, c = _record(kind, it)
        rec_bytes += r
        chan_bytes += c
    layer_info = struct.pack('>h', len(recs)) + rec_bytes + chan_bytes
    if len(layer_info) % 4:
        layer_info += b'\0' * (4 - len(layer_info) % 4)
    lmi = struct.pack('>I', len(layer_info)) + layer_info + struct.pack('>I', 0)

    comp = np.asarray(composite.convert('RGB'))
    all_counts, all_data = [], b''
    for ch in range(3):
        counts, packed = rle_channel(np.ascontiguousarray(comp[:, :, ch]))
        all_counts += counts
        all_data += packed

    with open(path, 'wb') as f:
        f.write(b'8BPS' + struct.pack('>H', 1) + b'\0' * 6)
        f.write(struct.pack('>HIIHH', 3, height, width, 8, 3))
        f.write(struct.pack('>I', 0))                                   # color mode data
        # image resources: resolution info (144 ppi = 2x of 72)
        res = struct.pack('>IHHIHH', 144 << 16, 1, 1, 144 << 16, 1, 1)
        ir = b'8BIM' + struct.pack('>H', 1005) + b'\0\0' + struct.pack('>I', len(res)) + res
        f.write(struct.pack('>I', len(ir)) + ir)
        f.write(struct.pack('>I', len(lmi)) + lmi)
        f.write(struct.pack('>H', 1) + struct.pack('>%dH' % len(all_counts), *all_counts) + all_data)
