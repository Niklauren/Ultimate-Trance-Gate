"""Builds two layered PSDs of the "Tooth Head" concept:
   UltimateTranceGate_ToothHead_Plugin.psd  - the whole plugin window (1040x400 @2x = 2080x800)
   UltimateTranceGate_ToothHead_Face.psd    - only the head (560x360 @2x = 1120x720)
Coordinates below are in 1x design pixels (same as the canvas mockup)."""
import math, os, sys
from PIL import Image, ImageDraw, ImageFont
sys.path.insert(0, os.path.dirname(__file__))
from psdwriter import Layer, Group, write_psd

S = 2            # output scale (Retina)
SS = 4           # supersampling
K = S * SS
W, H = 1040, 400
FONTS = '/home/claude/TranceGate/Resources/fonts/Figtree-%s.ttf'

C = dict(bg='#22101A', panel='#2D1621', raised='#3A1F2B', sel='#4D2A3A', line='#4F2D3D',
         text='#EAF6FF', dim='#A9C3D6', faint='#8FA7BA', blue='#D7EFFF', orange='#FF5C34', orange2='#FF8A6A',
         ink='#120810', mouth='#3A0F22', gum='#8E4A66', socket='#5E2A40', lid='#BFDDF2', white='#FFFFFF',
         tooth='#F7FCFF', wire='#8FA7BA')


def hexa(h, a=255):
    h = h.lstrip('#')
    return (int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16), a)


def new_layer():
    return Image.new('RGBA', (W * S, H * S), (0, 0, 0, 0))


def put(layer, x, y, w, h, fn, pad=0):
    """Draw fn(draw, k, img) on a supersampled tile covering (x,y,w,h) in 1x units, then place it."""
    x0, y0, w0, h0 = x - pad, y - pad, w + 2 * pad, h + 2 * pad
    tw, th = int(round(w0 * K)), int(round(h0 * K))
    tile = Image.new('RGBA', (tw, th), (0, 0, 0, 0))
    d = ImageDraw.Draw(tile)
    fn(d, K, tile, pad * K)
    small = tile.convert('RGBa').resize((int(round(w0 * S)), int(round(h0 * S))), Image.LANCZOS).convert('RGBA')
    layer.alpha_composite(small, (int(round(x0 * S)), int(round(y0 * S))))
    return layer


def rr(d, box, r, fill):
    x0, y0, x1, y1 = box
    r = max(0, min(r, (x1 - x0) / 2, (y1 - y0) / 2))
    d.rounded_rectangle(box, radius=r, fill=fill)


def box_with_border(x, y, w, h, r, fill, border, bw, corners=None):
    """Cartoon box: outer shape in border colour, inner in fill."""
    def fn(d, k, img, p):
        rr(d, (p, p, p + w * k - 1, p + h * k - 1), r * k, hexa(border) if bw else hexa(fill))
        if bw:
            rr(d, (p + bw * k, p + bw * k, p + (w - bw) * k - 1, p + (h - bw) * k - 1), max(0, r - bw) * k, hexa(fill))
    return fn


def font(weight, size):
    return ImageFont.truetype(FONTS % weight, int(round(size * S)))


def text(layer, x, y, s, weight, size, color, tracking=0.0, anchor='la'):
    d = ImageDraw.Draw(layer)
    f = font(weight, size)
    if tracking == 0:
        d.text((x * S, y * S), s, font=f, fill=hexa(color), anchor=anchor)
        return
    total = sum(f.getlength(ch) for ch in s) + tracking * size * S * (len(s) - 1)
    cx = x * S
    if anchor[0] == 'm':
        cx -= total / 2
    for ch in s:
        d.text((cx, y * S), ch, font=f, fill=hexa(color), anchor='l' + anchor[1])
        cx += f.getlength(ch) + tracking * size * S


def text_width(s, weight, size):
    return font(weight, size).getlength(s) / S


def L(name, img, visible=True, opacity=255):
    return Layer(name, img, 0, 0, visible, opacity)


# ------------------------------------------------------------------ pattern: Rage Chop
PATTERN = 'X-.X-.X.X-.X2.4.'
PLAY = 3   # the gold tooth


def parse(p):
    st = []
    for ch in p:
        if ch == '.':
            st.append(dict(on=False, level=0, r=1, tie=False))
        elif ch == '-':
            st.append(dict(on=True, level=1, r=1, tie=True))
        elif ch in '234':
            st.append(dict(on=True, level=1, r=int(ch), tie=False))
        else:
            st.append(dict(on=True, level={'X': 1, 'x': .7, 'o': .4, ',': .2}.get(ch, 1), r=1, tie=False))
    return st


STEPS = parse(PATTERN)

# ------------------------------------------------------------------ geometry (1x)
HEAD = (260, 50, 520, 330)          # x, y, w, h  (border 4)
HI = (HEAD[0] + 4, HEAD[1] + 4)      # head inner origin
MOUTH = (HI[0] + 34, HI[1] + 146, 444, 170)
MI = (MOUTH[0] + 4, MOUTH[1] + 4)    # mouth inner origin, inner size 436 x 162
GAP, COLW = 4, (412 - 15 * 4) / 16
TEETH_X, TEETH_Y = MI[0] + 12, MI[1] + 12

FACES = {
    'Smooth': dict(brow=(-4, 4), browTop=30, eyeH=54, eyeR=14, lid=0),
    'Hard':   dict(brow=(16, -16), browTop=36, eyeH=44, eyeR=10, lid=18),
    'Soft':   dict(brow=(-12, 12), browTop=34, eyeH=54, eyeR=14, lid=48),
    'Pump':   dict(brow=(-6, 6), browTop=18, eyeH=62, eyeR=31, lid=0),
}
CHAR = 'Hard'     # Rage Chop comes with Hard


# ------------------------------------------------------------------ face parts
def ear(side):
    lay = new_layer()
    x = 240 if side == 'L' else 766
    put(lay, x, 160, 34, 70, box_with_border(x, 160, 34, 70, 18, C['blue'], C['ink'], 4))
    return lay


def head_shape():
    lay = new_layer()
    x, y, w, h = HEAD
    put(lay, x, y, w, h, box_with_border(x, y, w, h, 36, C['blue'], C['ink'], 4))
    return lay


def cheek(side):
    lay = new_layer()
    x = HI[0] + (44 if side == 'L' else 512 - 44 - 54)
    put(lay, x, HI[1] + 128, 54, 26, lambda d, k, img, p: d.ellipse((p, p, p + 54 * k, p + 26 * k), fill=hexa(C['orange2'])))
    return lay


def tuft(i, on):
    lay = new_layer()
    x = 441 + i * 56
    fill = C['orange'] if on else C['sel']

    def fn(d, k, img, p):
        # top-rounded shape with 4px border on top/left/right, open bottom
        d.rounded_rectangle((p, p, p + 46 * k, p + 34 * k + 22 * k), radius=22 * k, fill=hexa(C['ink']))
        d.rounded_rectangle((p + 4 * k, p + 4 * k, p + 42 * k, p + 34 * k + 22 * k), radius=18 * k, fill=hexa(fill))
        d.rectangle((p, p + 34 * k, p + 46 * k, p + 60 * k), fill=(0, 0, 0, 0))
    put(lay, x, 20, 46, 34, fn)
    text(lay, x + 23, 20 + 20, 'ABC'[i], 'ExtraBold', 15, C['bg'] if on else C['blue'], anchor='mm')
    return lay


def brow(side, face):
    lay = new_layer()
    f = FACES[face]
    w, h = 82, 12
    x = HI[0] + (112 if side == 'L' else 512 - 112 - w)
    y = HI[1] + f['browTop']
    ang = f['brow'][0 if side == 'L' else 1]

    def fn(d, k, img, p):
        bar = Image.new('RGBA', (w * k, h * k), (0, 0, 0, 0))
        ImageDraw.Draw(bar).rounded_rectangle((0, 0, w * k - 1, h * k - 1), radius=6 * k, fill=hexa(C['ink']))
        bar = bar.rotate(-ang, resample=Image.BICUBIC, expand=True)
        img.alpha_composite(bar, (int(p + (w * k - bar.width) / 2), int(p + (h * k - bar.height) / 2)))
    put(lay, x, y, w, h, fn, pad=14)
    return lay


def eye_parts(side, face):
    f = FACES[face]
    x = HI[0] + (120 if side == 'L' else 512 - 120 - 66)
    y = HI[1] + 52
    w, h = 66, f['eyeH']
    white = new_layer()
    put(white, x, y, w, h, box_with_border(x, y, w, h, f['eyeR'], C['white'], C['ink'], 4))
    pupil = new_layer()
    px = x + w / 2 - 11 + (-11 + (PLAY / 15 - .5) * 22) + 11
    put(pupil, px, y + h / 2 - 11, 22, 22, lambda d, k, img, p: rr(d, (p, p, p + 22 * k - 1, p + 22 * k - 1), 7 * k, hexa(C['ink'])))
    lid = None
    if f['lid']:
        lid = new_layer()
        lh = h * f['lid'] / 100

        def fn(d, k, img, p):
            # clip to the eye's inner shape
            mask = Image.new('L', (w * k, h * k), 0)
            ImageDraw.Draw(mask).rounded_rectangle((4 * k, 4 * k, (w - 4) * k - 1, (h - 4) * k - 1), radius=max(0, f['eyeR'] - 4) * k, fill=255)
            part = Image.new('RGBA', (w * k, h * k), (0, 0, 0, 0))
            pd = ImageDraw.Draw(part)
            pd.rectangle((0, 0, w * k, lh * k), fill=hexa(C['ink']))
            pd.rectangle((0, 0, w * k, (lh - 4) * k), fill=hexa(C['lid']))
            part.putalpha(Image.composite(part.getchannel('A'), Image.new('L', part.size, 0), mask))
            img.alpha_composite(part, (p, p))
        put(lid, x, y, w, h, fn)
    return white, pupil, lid


def mouth_mask(k):
    w, h = 444, 170
    a = Image.new('L', (w * k, h * k), 0)
    b = Image.new('L', (w * k, h * k), 0)
    ImageDraw.Draw(a).rounded_rectangle((0, 0, w * k - 1, h * k - 1), radius=22 * k, fill=255)
    ImageDraw.Draw(b).rounded_rectangle((0, 0, w * k - 1, h * k - 1), radius=58 * k, fill=255)
    m = Image.new('L', a.size, 0)
    m.paste(a.crop((0, 0, w * k, h * k // 2)), (0, 0))
    m.paste(b.crop((0, h * k // 2, w * k, h * k)), (0, h * k // 2))
    return m


def mouth_layers():
    x, y, w, h = MOUTH
    border = new_layer()
    inside = new_layer()

    def border_fn(d, k, img, p):
        m = mouth_mask(k)
        ink = Image.new('RGBA', m.size, hexa(C['ink']))
        img.paste(ink, (p, p), m)
    put(border, x, y, w, h, border_fn)

    def inner_mask(k):
        m = mouth_mask(k)
        inner = Image.new('L', m.size, 0)
        small = m.resize(((w - 8) * k, (h - 8) * k), Image.LANCZOS)
        inner.paste(small, (4 * k, 4 * k))
        return inner

    def clipped(draw_inner):
        def fn(d, k, img, p):
            m = inner_mask(k)
            part = Image.new('RGBA', m.size, (0, 0, 0, 0))
            draw_inner(ImageDraw.Draw(part), k, part)
            part.putalpha(Image.composite(part.getchannel('A'), Image.new('L', m.size, 0), m))
            img.alpha_composite(part, (p, p))
        return fn

    put(inside, x, y, w, h, clipped(lambda d, k, im: d.rectangle((0, 0, w * k, h * k), fill=hexa(C['mouth']))))
    gum = new_layer()
    put(gum, x, y, w, h, clipped(lambda d, k, im: d.rectangle((0, 0, w * k, (4 + 16) * k), fill=hexa(C['gum']))))

    tongue = new_layer()

    def tongue_fn(d, k, im):
        tx, ty = 4 + 123, 4 + 126
        d.ellipse((tx * k, ty * k, (tx + 190) * k, (ty + 70) * k), fill=hexa(C['ink']))
        d.ellipse(((tx + 4) * k, (ty + 4) * k, (tx + 186) * k, (ty + 66) * k), fill=hexa(C['orange']))
    put(tongue, x, y, w, h, clipped(tongue_fn))

    wave = new_layer()

    def wave_fn(d, k, im):
        bx, by, bw, bh = 4 + 14, 4 + 98, 436 - 28, 30
        bar_w = (bw - 63 * 2) / 64
        for i in range(64):
            st = STEPS[i // 4]
            g = .95 if st['on'] else .12
            hh = max(6, 100 * g * (.55 + .45 * abs(math.sin(i * 1.7) * math.cos(i * .6)))) / 100 * bh
            a = int(255 * (.85 if i // 4 == PLAY else .35))
            x0 = bx + i * (bar_w + 2)
            rr(d, (x0 * k, (by + bh - hh) * k, (x0 + bar_w) * k, (by + bh) * k), 2 * k, hexa(C['blue'], a))
    put(wave, x, y, w, h, clipped(wave_fn))
    return border, inside, gum, wave, tongue


# ------------------------------------------------------------------ teeth
def tooth_shape(d, k, x, y, w, h, fill, ratchet=1):
    sub_w = (w - 2 * (ratchet - 1)) / ratchet
    for r in range(ratchet):
        sx = x + r * (sub_w + 2)
        # border 3 (no top), bottom radius 9
        d.rounded_rectangle((sx * k, (y - 10) * k, (sx + sub_w) * k - 1, (y + h) * k - 1), radius=min(9, sub_w / 2) * k, fill=hexa(C['ink']))
        b = min(3, sub_w * 0.3)
        d.rounded_rectangle(((sx + b) * k, (y - 10) * k, (sx + sub_w - b) * k - 1, (y + h - b) * k - 1), radius=min(6, sub_w / 2 - b) * k, fill=hexa(fill))


def teeth_groups():
    groups, i = [], 0
    while i < 16:
        s = STEPS[i]
        if not s['on']:
            groups.append(dict(kind='socket', a=i, b=i))
            i += 1
            continue
        n = 1
        while i + n < 16 and STEPS[i + n]['tie']:
            n += 1
        groups.append(dict(kind='tooth', a=i, b=i + n - 1, level=s['level'], r=s['r']))
        i += n
    return groups


def teeth_layers():
    out = []
    for g in teeth_groups():
        lay = new_layer()
        x = TEETH_X + g['a'] * (COLW + GAP)
        n = g['b'] - g['a'] + 1
        w = COLW * n + GAP * (n - 1)
        steps = 'step %02d' % (g['a'] + 1) + ('' if n == 1 else '-%02d' % (g['b'] + 1))
        if g['kind'] == 'socket':
            sw = w * .6
            put(lay, x + (w - sw) / 2, MI[1], sw, 7 + 12,
                lambda d, k, img, p: rr(d, (p, p - 12 * k, p + sw * k - 1, p + 19 * k - 1), 6 * k, hexa(C['socket'])))
            out.append(L(steps + ' - socket (silence)', lay))
            continue
        h = 30 + 70 * g['level']
        gold = g['a'] <= PLAY <= g['b']
        fill = C['orange'] if gold else C['tooth']
        put(lay, x, MI[1], w, 12 + h, lambda d, k, img, p: tooth_shape(d, k, p / k, 12, w, h, fill, g['r']))
        label = steps + (' - GOLD (playing)' if gold else ' - tooth') + (' - ratchet x%d' % g['r'] if g['r'] > 1 else '')
        out.append(L(label, lay))
        if n > 1:
            br = new_layer()

            def braces(d, k, img, p):
                rr(d, (p + 6 * k, p + 30 * k, p + (w - 6) * k, p + 34 * k), 2 * k, hexa(C['wire']))
                for c in range(n):
                    bx = c * (COLW + GAP) + COLW / 2 - 6
                    rr(d, (p + bx * k, p + 25 * k, p + (bx + 12) * k - 1, p + 39 * k - 1), 3 * k, hexa(C['ink']))
                    rr(d, (p + (bx + 2) * k, p + 27 * k, p + (bx + 10) * k - 1, p + 37 * k - 1), 2 * k, hexa(C['dim']))
            put(br, x, TEETH_Y, w, 45, braces)
            out.append(L(steps + ' - braces (tie)', br))
    return out


def templates():
    """Hidden single parts to redesign: one of each tooth state, drawn at a free spot over the teeth."""
    t = []
    x0, y0 = TEETH_X, TEETH_Y - 12
    spec = [('tooth 100% (template)', 1, 1, C['tooth']), ('tooth 40% (template)', .4, 1, C['tooth']),
            ('tooth GOLD playing (template)', 1, 1, C['orange']), ('tooth ratchet x2 (template)', 1, 2, C['tooth']),
            ('tooth ratchet x3 (template)', 1, 3, C['tooth']), ('tooth ratchet x4 (template)', 1, 4, C['tooth'])]
    for idx, (name, lv, r, fill) in enumerate(spec):
        lay = new_layer()
        x = x0 + idx * 2 * (COLW + GAP)
        h = 30 + 70 * lv
        put(lay, x, y0, COLW if r == 1 else COLW * 1.6, 12 + h, lambda d, k, img, p, h=h, r=r, fill=fill, w=(COLW if r == 1 else COLW * 1.6): tooth_shape(d, k, p / k, 12, w, h, fill, r))
        t.append(L(name, lay, visible=False))
    lay = new_layer()
    put(lay, x0 + 12 * (COLW + GAP), y0, COLW * .6, 19, lambda d, k, img, p: rr(d, (p, p - 12 * k, p + COLW * .6 * k - 1, p + 19 * k - 1), 6 * k, hexa(C['socket'])))
    t.append(L('socket - silence (template)', lay, visible=False))
    lay = new_layer()

    def br(d, k, img, p):
        rr(d, (p, p, p + 12 * k - 1, p + 14 * k - 1), 3 * k, hexa(C['ink']))
        rr(d, (p + 2 * k, p + 2 * k, p + 10 * k - 1, p + 12 * k - 1), 2 * k, hexa(C['dim']))
    put(lay, x0 + 14 * (COLW + GAP), y0 + 37, 12, 14, br)
    t.append(L('brace bracket (template)', lay, visible=False))
    return t


# ------------------------------------------------------------------ side panels
def panel_box(lay, x, y, w, h, r, fill, border=None, bw=1, dashed=False):
    def fn(d, k, img, p):
        if border:
            rr(d, (p, p, p + w * k - 1, p + h * k - 1), r * k, hexa(border))
            rr(d, (p + bw * k, p + bw * k, p + (w - bw) * k - 1, p + (h - bw) * k - 1), max(0, r - bw) * k, hexa(fill) if fill else (0, 0, 0, 0))
            if fill is None:
                # punch out the inside
                inner = Image.new('L', img.size, 0)
                ImageDraw.Draw(inner).rounded_rectangle((p + bw * k, p + bw * k, p + (w - bw) * k - 1, p + (h - bw) * k - 1), radius=max(0, r - bw) * k, fill=255)
                a = img.getchannel('A')
                img.putalpha(Image.composite(Image.new('L', img.size, 0), a, inner))
                if dashed:
                    a = img.getchannel('A')
                    dm = Image.new('L', img.size, 255)
                    dd = ImageDraw.Draw(dm)
                    step = 6 * k
                    for xx in range(0, img.width, step * 2):
                        dd.rectangle((xx + step, 0, xx + 2 * step - 1, img.height), fill=0)
                    for yy in range(0, img.height, step * 2):
                        pass
                    img.putalpha(Image.composite(a, Image.new('L', img.size, 0), dm))
        else:
            rr(d, (p, p, p + w * k - 1, p + h * k - 1), r * k, hexa(fill))
    put(lay, x, y, w, h, fn)


def chevron(lay, cx, cy, left=True, color=C['text']):
    def fn(d, k, img, p):
        pts = [(9 * k, 2 * k), (3 * k, 6 * k), (9 * k, 10 * k)] if left else [(3 * k, 2 * k), (9 * k, 6 * k), (3 * k, 10 * k)]
        d.line([(p + a, p + b) for a, b in pts], fill=hexa(color), width=int(2.4 * k), joint='curve')
    put(lay, cx - 6, cy - 6, 12, 12, fn, pad=2)


def left_panel():
    items = []
    lay = new_layer()
    text(lay, 20, 22, 'Ultimate Trance ', 'ExtraBold', 19, C['text'])
    text(lay, 20 + text_width('Ultimate Trance ', 'ExtraBold', 19), 22, 'Gate', 'ExtraBold', 19, C['orange'])
    items.append(L('title (live text in the plugin)', lay))
    lay = new_layer()
    text(lay, 20, 45, 'created by mikita_laurenau', 'Regular', 12, C['dim'])
    items.append(L('credit link (live text)', lay))

    lay = new_layer()
    panel_box(lay, 20, 70, 200, 44, 10, C['panel'], C['line'])
    for bx in (26, 188):
        panel_box(lay, bx, 77, 26, 30, 6, C['raised'])
    chevron(lay, 39, 92, True)
    chevron(lay, 201, 92, False)
    items.append(L('preset card + arrows', lay))
    lay = new_layer()
    text(lay, 58, 79, 'RAGE', 'ExtraBold', 9, C['dim'], tracking=.14)
    text(lay, 58, 92, 'Rage Chop', 'Bold', 13, C['text'])
    items.append(L('preset name (live text)', lay))

    fields = [('STEPS', '16'), ('RATE', '1/16'), ('EUCLID', '5'), ('ROT', '0')]
    boxes, labels = new_layer(), new_layer()
    for i, (a, b) in enumerate(fields):
        fx = 20 + (i % 2) * 103
        fy = 122 + (i // 2) * 46
        panel_box(boxes, fx, fy, 97, 40, 8, C['panel'], C['line'])
        text(labels, fx + 9, fy + 6, a, 'ExtraBold', 9, C['dim'], tracking=.14)
        text(labels, fx + 9, fy + 18, b, 'Bold', 14, C['text'])
    items.append(L('STEPS RATE EUCLID ROT boxes', boxes))
    items.append(L('STEPS RATE EUCLID ROT values (live text)', labels))

    lay = new_layer()
    panel_box(lay, 20, 216, 200, 30, 8, C['orange'])
    text(lay, 120, 231, 'RANDOM', 'ExtraBold', 12, C['bg'], tracking=.14, anchor='mm')
    items.append(L('RANDOM button', lay))

    lay = new_layer()
    tools = ['<<', '>>', 'REV', 'INV', 'x2', '/2', 'FILL', 'CLR']
    tw = (200 - 12) / 4
    for i, t in enumerate(tools):
        tx = 20 + (i % 4) * (tw + 4)
        ty = 254 + (i // 4) * 30
        panel_box(lay, tx, ty, tw, 26, 6, C['panel'], C['line'])
        text(lay, tx + tw / 2, ty + 13, t, 'Bold', 10, C['text'], anchor='mm')
    items.append(L('tool buttons', lay))

    lay = new_layer()
    panel_box(lay, 20, 352, 34, 28, 6, C['panel'], C['line'])
    panel_box(lay, 58, 352, 34, 28, 6, C['panel'], C['line'])

    def undo_icon(d, k, img, p, flip=False, col=C['text']):
        def X(v): return p + ((14 - v) if flip else v) * k
        d.line([(X(5), p + 3 * k), (X(2), p + 6 * k), (X(5), p + 9 * k)], fill=hexa(col), width=int(1.8 * k), joint='curve')
        d.arc((X(13) if flip else X(2), p + 6 * k, X(2) if flip else X(13), p + 12 * k), 270, 90, fill=hexa(col), width=int(1.8 * k)) if False else None
        d.line([(X(2), p + 6 * k), (X(10), p + 6 * k)], fill=hexa(col), width=int(1.8 * k))
        d.arc(((X(7) if not flip else X(13)), p + 6 * k, (X(13) if not flip else X(7)), p + 12 * k), 270 if not flip else 90, 90 if not flip else 270, fill=hexa(col), width=int(1.8 * k))
    put(lay, 30, 359, 14, 14, lambda d, k, img, p: undo_icon(d, k, img, p), pad=1)
    put(lay, 68, 359, 14, 14, lambda d, k, img, p: undo_icon(d, k, img, p, True, C['faint']), pad=1)
    panel_box(lay, 96, 352, 124, 28, 6, None, C['dim'], dashed=True)
    text(lay, 158, 366, 'MIDI', 'ExtraBold', 11, C['text'], tracking=.12, anchor='mm')
    items.append(L('undo redo MIDI', lay))
    return Group('LEFT panel - preset and pattern tools', list(reversed(items)))


def knob(lay, cx, cy, f):
    def fn(d, k, img, p):
        bb = (p + 4 * k, p + 4 * k, p + 32 * k, p + 32 * k)
        d.arc(bb, 135, 405, fill=hexa(C['line']), width=4 * k)
        if f > 0:
            d.arc(bb, 135, 135 + 270 * f, fill=hexa(C['blue']), width=4 * k)
        for ang, col in ((135, C['line'] if f == 0 else C['blue']), (405, C['line'] if f < 1 else C['blue'])):
            a = math.radians(ang)
            ex, ey = p + 18 * k + 14 * k * math.cos(a), p + 18 * k + 14 * k * math.sin(a)
            d.ellipse((ex - 2 * k, ey - 2 * k, ex + 2 * k, ey + 2 * k), fill=hexa(col))
        if 0 < f:
            a = math.radians(135 + 270 * f)
            ex, ey = p + 18 * k + 14 * k * math.cos(a), p + 18 * k + 14 * k * math.sin(a)
            d.ellipse((ex - 2 * k, ey - 2 * k, ex + 2 * k, ey + 2 * k), fill=hexa(C['blue']))
    put(lay, cx - 18, cy - 18, 36, 36, fn)


def right_panel():
    items = []
    lay = new_layer()
    mw = (200 - 8) / 3
    for i, (m, on) in enumerate([('VOL', True), ('LPF', False), ('HPF', False)]):
        mx = 820 + i * (mw + 4)
        if on:
            panel_box(lay, mx, 20, mw, 26, 6, C['orange'])
        else:
            panel_box(lay, mx, 20, mw, 26, 6, C['panel'], C['line'])
        text(lay, mx + mw / 2, 33, m, 'ExtraBold', 11, C['bg'] if on else C['dim'], tracking=.1, anchor='mm')
    items.append(L('mode VOL LPF HPF', lay))

    lay = new_layer()
    text(lay, 820, 56, 'CHARACTER', 'ExtraBold', 9, C['dim'], tracking=.14)
    cw = (200 - 4) / 2
    for i, ch in enumerate(['Smooth', 'Hard', 'Soft', 'Pump']):
        cx = 820 + (i % 2) * (cw + 4)
        cy = 70 + (i // 2) * 30
        on = ch == CHAR
        if on:
            panel_box(lay, cx, cy, cw, 26, 6, C['blue'])
        else:
            panel_box(lay, cx, cy, cw, 26, 6, C['panel'], C['line'])
        text(lay, cx + cw / 2, cy + 13, ch, 'Bold', 12, C['bg'] if on else C['text'], anchor='mm')
    items.append(L('CHARACTER buttons', lay))

    pan = new_layer()
    panel_box(pan, 820, 136, 200, 244, 12, C['panel'], C['line'])
    items.append(L('knob panel', pan))
    knobs = [('ATTACK', .08, '0.5 ms'), ('DECAY', .4, '120 ms'), ('SUSTAIN', .9, '90 %'), ('RELEASE', .15, '12 ms'),
             ('GATE', .8, '80 %'), ('SWING', 0, '0 %'), ('DEPTH', 1, '100 %'), ('STEREO', 0, 'Off'), ('MIX', 1, '100 %')]
    kl, kt = new_layer(), new_layer()
    cell_w, cell_h = (200 - 12 - 8) / 3, 72
    for i, (name, f, val) in enumerate(knobs):
        cx = 826 + (i % 3) * (cell_w + 4) + cell_w / 2
        cy = 144 + (i // 3) * (cell_h + 6)
        knob(kl, cx, cy + 18, f)
        text(kt, cx, cy + 41, name, 'ExtraBold', 8, C['dim'], tracking=.1, anchor='mm')
        text(kt, cx, cy + 54, val, 'Bold', 11, C['text'], anchor='mm')
    items.append(L('knobs (arc = value, drawn by code)', kl))
    items.append(L('knob labels and values (live text)', kt))
    return Group('RIGHT panel - mode, character, knobs', list(reversed(items)))


# ------------------------------------------------------------------ zones guide
def zones():
    lay = new_layer()
    d = ImageDraw.Draw(lay)
    col = (0, 200, 255, 200)

    def z(x, y, w, h, label):
        d.rectangle((x * S, y * S, (x + w) * S, (y + h) * S), outline=col, width=2)
        d.text(((x + 3) * S, (y + 2) * S), label, font=font('Bold', 8), fill=col)
    z(TEETH_X, TEETH_Y - 12, 412, 104, 'STEP GRID: click a tooth = step on/off, drag = level')
    z(441, 20, 158, 34, 'VAR A/B/C')
    z(20, 70, 200, 44, 'PRESET')
    z(820, 20, 200, 26, 'MODE')
    z(820, 70, 200, 56, 'CHARACTER')
    z(820, 136, 200, 244, 'KNOBS')
    z(20, 122, 200, 152, 'PATTERN')
    return L('ZONES - where things are clickable (hide before export)', lay, visible=False)


# ------------------------------------------------------------------ assemble
def face_tree():
    ex = {}
    for face in FACES:
        parts = []
        for side in 'LR':
            white, pupil, lid = eye_parts(side, face)
            sname = 'left' if side == 'L' else 'right'
            if lid:
                parts.append(L('eyelid %s' % sname, lid))
            parts.append(L('pupil %s (moves with the rhythm)' % sname, pupil))
            parts.append(L('eye %s' % sname, white))
        ex[face] = parts
    eyes = Group('EYES by CHARACTER', [Group('eyes %s' % f, ex[f], visible=(f == CHAR)) for f in FACES])
    brows = Group('BROWS by CHARACTER', [Group('brows %s' % f, [L('brow left', brow('L', f)), L('brow right', brow('R', f))], visible=(f == CHAR)) for f in FACES])
    border, inside, gum, wave, tongue = mouth_layers()
    mouth = Group('MOUTH = step grid', [
        Group('TEMPLATES - single parts to redesign (hidden)', templates(), visible=False, open_=False),
        Group('TEETH (Rage Chop, step 4 playing)', list(reversed(teeth_layers()))),
        L('tongue', tongue), L('signal wave (drawn live by code)', wave), L('gum', gum),
        L('mouth inside', inside), L('mouth outline', border)])
    hair = Group('HAIR TUFTS = VAR A/B/C', [
        Group('states (hidden)', [L('tuft ON template', tuft(0, True), visible=False), L('tuft OFF template', tuft(0, False), visible=False)], visible=False, open_=False),
        L('tuft C', tuft(2, False)), L('tuft B', tuft(1, False)), L('tuft A (selected)', tuft(0, True))])
    head = Group('HEAD', [
        L('cheek right', cheek('R'), opacity=115), L('cheek left', cheek('L'), opacity=115),
        L('head shape', head_shape()), L('ear right', ear('R')), L('ear left', ear('L'))])
    return [hair, mouth, eyes, brows, head]


def bg_layer():
    lay = new_layer()
    ImageDraw.Draw(lay).rectangle((0, 0, W * S, H * S), fill=hexa(C['bg']))
    return lay


def composite(tree, base):
    img = base.copy()

    def walk(items):
        for it in reversed(items):
            if isinstance(it, Group):
                if it.visible:
                    walk(it.children)
            elif it.visible:
                lay = it.image
                if it.opacity < 255:
                    a = lay.getchannel('A').point(lambda v: v * it.opacity // 255)
                    lay = lay.copy()
                    lay.putalpha(a)
                img.alpha_composite(lay)
    walk(tree)
    return img


def shift(tree, dx, dy):
    for it in tree:
        if isinstance(it, Group):
            shift(it.children, dx, dy)
        else:
            it.left, it.top = it.left + dx, it.top + dy
    return tree


if __name__ == '__main__':
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    bg = bg_layer()
    face = face_tree()
    plugin_tree = [zones(), Group('FACE - Tooth Head', face), right_panel(), left_panel(), L('background', bg)]
    comp = composite(plugin_tree, Image.new('RGBA', (W * S, H * S), (0, 0, 0, 255)))
    comp.save(os.path.join(out, 'UltimateTranceGate_ToothHead_Plugin.png'))
    write_psd(os.path.join(out, 'UltimateTranceGate_ToothHead_Plugin.psd'), W * S, H * S, plugin_tree, comp)

    # face only: crop 560x360 around the head column (x 240..800, y 20..380)
    fx, fy, fw, fh = 220, 0, 600, 400
    face2 = face_tree()
    fbg = Image.new('RGBA', (fw * S, fh * S), hexa(C['bg']))
    shift(face2, -fx * S, -fy * S)
    face_full = [Group('FACE - Tooth Head', face2), Layer('background', fbg, 0, 0)]
    fcomp = comp.crop((fx * S, fy * S, (fx + fw) * S, (fy + fh) * S))
    fcomp.save(os.path.join(out, 'UltimateTranceGate_ToothHead_Face.png'))
    write_psd(os.path.join(out, 'UltimateTranceGate_ToothHead_Face.psd'), fw * S, fh * S, face_full, fcomp)
    print('done')
