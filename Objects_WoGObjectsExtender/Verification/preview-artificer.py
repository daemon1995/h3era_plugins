h3_NullString"Offline layout preview using installed game DEFs (not an in-game screenshot).h3_NullString"
from pathlib import Path
import json
import struct
import zlib
from PIL import Image, ImageDraw, ImageFont, ImageEnhance

ROOT = Path(__file__).resolve().parent.parent
GAME = Path('D:/games/HoMM 3 ERA')
OUT = ROOT / 'build' / 'preview'
OUT.mkdir(parents=True, exist_ok=True)


def assets(path, wanted):
    result = {}
    with path.open('rb') as f:
        header = f.read(92)
        count = struct.unpack_from('<I', header, 8)[0]
        entries = f.read(count * 32)
        for i in range(count):
            name, offset, size, _, packed = struct.unpack_from('<16s4I', entries, i * 32)
            name = name.split(b'\0')[0].decode('ascii').lower()
            if name not in wanted:
                continue
            f.seek(offset)
            data = f.read(packed or size)
            result[name] = zlib.decompress(data) if packed else data
    return result


data = assets(GAME / 'Data/H3sprite.lod', {'artifact.def', 'resource.def', 'dlgbox.def'})
data.update(assets(GAME / 'Mods/WoG/Data/hmm35wog.pac', {'artifact.def', 'resource.def', 'zartif1.def'}))
data.update(assets(GAME / 'Data/H3bitmap.lod', {'diboxbck.pcx'}))
buttons = {'ecptsml1.def','rmg_okay.def','rmg_cncl.def','rmg_help.def','scnrblf.def','scnrbrt.def','gspbut2.def'}
data.update(assets(GAME / 'Data/H3sprite.lod', buttons))
for name in ('era_InterfaceAssets.pac','era_InterfaceAssets2.pac'):
    data.update(assets(GAME / 'Mods/WoG/Data'/name, buttons))
assert buttons <= data.keys(), buttons-data.keys()


def frame(name, index):
    blob = data[name]
    group, count, _, _ = struct.unpack_from('<4I', blob, 784)
    assert index < count
    offset = struct.unpack_from('<I', blob, 800 + 13 * count + 4 * index)[0]
    size, fmt, fw, fh, w, h, left, top = struct.unpack_from('<8I', blob, offset)
    base = offset + 32
    pixels = bytearray(w * h)
    if fmt == 0:
        pixels[:] = blob[base:base+w*h]
    elif fmt == 1:
        for y in range(h):
            p = base + struct.unpack_from('<I', blob, base + 4*y)[0]
            row = bytearray()
            while len(row) < w:
                color, length = blob[p:p+2]
                p += 2
                length += 1
                row.extend(blob[p:p+length] if color == 255 else bytes([color])*length)
                if color == 255:
                    p += length
            assert len(row) == w
            pixels[y*w:(y+1)*w] = row
    elif fmt == 3:
        chunks = (w + 31) // 32
        for y in range(h):
            for chunk in range(chunks):
                p = base + struct.unpack_from('<H', blob, base + (y*chunks+chunk)*2)[0]
                row = bytearray()
                target = min(32, w - chunk*32)
                while len(row) < target:
                    packed = blob[p]
                    p += 1
                    color, length = packed >> 5, (packed & 31) + 1
                    row.extend(blob[p:p+length] if color == 7 else bytes([color])*length)
                    if color == 7:
                        p += length
                assert len(row) == target
                pixels[y*w+chunk*32:y*w+chunk*32+target] = row
    else:
        raise ValueError(fmt)
    rgba = []
    for color in pixels:
        rgb = tuple(blob[16 + color*3:19 + color*3])
        rgba.append((0, 0, 0, 70) if 0 < color < 8 else rgb + ((0,) if color == 0 else (255,)))
    image = Image.new('RGBA', (w, h))
    image.putdata(rgba)
    return image


def pcx(name):
    blob = data[name]
    size, w, h = struct.unpack_from('<3I', blob)
    image = Image.frombytes('P', (w, h), blob[12:12+w*h])
    image.putpalette(blob[-768:])
    return image.convert('RGB')


gold = (224, 190, 108)
regular = (225, 216, 190)
font = ImageFont.truetype('C:/Windows/Fonts/arial.ttf', 12)
medium = ImageFont.truetype('C:/Windows/Fonts/arial.ttf', 15)
big = ImageFont.truetype('C:/Windows/Fonts/arialbd.ttf', 22)


def preview(sale=False):
    canvas = Image.new('RGB', (780, 560), (35, 30, 24))
    tile = pcx('diboxbck.pcx')
    for y in range(0, 560, tile.height):
        for x in range(0, 780, tile.width):
            canvas.paste(tile, (x, y))
    draw = ImageDraw.Draw(canvas)
    draw.rectangle((1, 1, 778, 558), outline=(153, 123, 65), width=4)
    draw.rectangle((7, 7, 772, 552), outline=(85, 66, 36))
    for box in [(18, 14, 125, 95), (18, 176, 441, 475), (456, 176, 761, 475)]:
        canvas.paste(ImageEnhance.Brightness(canvas.crop(box)).enhance(.55), box[:2])
        draw.rectangle(box, outline=(133, 103, 52))

    def text(x, y, w, h, value, f=font, color=regular, align='left'):
        lines = []
        for paragraph in value.split('\n'):
            line = ''
            for word in paragraph.split(' '):
                candidate = line + (' ' if line else '') + word
                if draw.textlength(candidate, font=f) > w and line:
                    lines.append(line)
                    line = word
                else:
                    line = candidate
            lines.append(line)
        line_height = f.size + 2
        assert len(lines) * line_height <= h + 3, (value, w, h, len(lines))
        yy = y + max(0, (h-len(lines)*line_height)//2)
        for line in lines:
            xx = x + ((w-draw.textlength(line,font=f))/2 if align == 'center' else 0)
            draw.text((xx, yy), line, font=f, fill=color)
            yy += line_height

    def button(x, y, w, label, active=False, enabled=True, asset='gspbut2.def', height=28, caption=True):
        image=frame(asset,1 if active else 0)
        if not enabled: image=ImageEnhance.Brightness(image).enhance(.5)
        canvas.paste(image,(x+(w-image.width)//2,y+(height-image.height)//2),image)
        if caption: text(x+2,y+2,w-4,height-4,label,color=gold if enabled else (115,106,86),align='center')

    def icon(name, index, x, y):
        art = frame(name,index)
        canvas.paste(art,(x,y),art)

    obj = frame('zartif1.def',0)
    obj.thumbnail((104,78))
    canvas.paste(obj,(20+(104-obj.width)//2,16+(78-obj.height)//2),obj)
    text(140,18,320,30,'Ремесленник',big,gold)
    text(140,50,320,22,'Посетитель: Кристиан',medium)
    text(140,76,552,24,'Заказов: 2. Работа стоит 100 очков передвижения.',color=gold)
    for i,(resource,value) in enumerate([('Золото','12 000'),('Мифрил','24')]):
        icon('resource.def',6+i,482+i*140,22)
        text(518+i*140,18,102,24,value,medium,gold)
        text(518+i*140,44,102,22,resource)
    for i,name in enumerate(['Улучшение','Обмен','Сборка','Продажа']):
        button(20+i*184,110,174,name,i==(3 if sale else 0),asset='ecptsml1.def',height=26)
    button(706,70,54,'?',asset='rmg_help.def',height=32,caption=False)
    text(22,148,416,24,'Предметов для продажи: 12' if sale else 'Доступно предложений: 7',color=gold)
    if sale:
        button(460,144,144,'Золото',True)
        button(614,144,146,'Мифрил')
    names_table=json.loads((GAME / 'Mods/WoG/lang/ru/WoG_Artifacts_Original.json').read_text(encoding='utf-8-sig'))['era']['artifacts']
    names=[names_table[str((7 if sale else 8)+i)]['name'] for i in range(5)]
    for i in range(5):
        y=177+i*60
        if i:
            draw.line((22,y-1,438,y-1),fill=(94,75,43))
        if i==0:
            draw.rectangle((20,y,439,y+57),outline=(224,184,80))
        icon('artifact.def',7+i,27,y+7)
        if not sale:
            icon('artifact.def',8+i,86,y+7)
            text(72,y+17,14,24,'>',color=gold)
        text(140,y+2,290,32,names[i])
        text(140,y+34,140,20,f'(рюкзак {i+1})' if sale else 'Правая рука')
        text(282,y+34,148,20,f'{250+i*125 if sale else 1000+i*500} монет',color=gold)
    text(470,183,276,22,'Вы продаёте' if sale else 'Вы получите',color=gold)
    icon('artifact.def',7 if sale else 8,480,219)
    text(538,208,208,62,names[0],medium,gold)
    text(470,274,276,38,'Продаются только предметы из рюкзака.\nПродажа не тратит движение и заказы.' if sale else 'Необходимые артефакты')
    if not sale:
        icon('artifact.def',7,492,316)
        text(468,361,92,30,names_table['7']['name'],align='center')
    if sale:
        text(478,326,266,106,'Продать всё: 12 предметов\n\nОбщая выручка:\n8 500 монет',medium,gold)
    button(20,482,40,'<',enabled=False,asset='scnrblf.def',caption=False)
    button(400,482,40,'>',asset='scnrbrt.def',caption=False)
    text(68,482,324,26,'1 / 3' if sale else '1 / 2',align='center')
    icon('resource.def',6,462,477)
    text(498,478,110,24,'250' if sale else '1000',medium,gold)
    button(338,512,102,'Уйти',asset='rmg_cncl.def',height=32,caption=False)
    button(460,512,55,'',asset='rmg_okay.def',height=32,caption=False)
    text(520,512,84,32,'Продать' if sale else 'Улучшить')
    if sale:
        button(614,512,146,'Продать всё (12)')
    text(18,542,744,14,'Выберите предложение. ПКМ по артефакту: описание. Enter: сделка. Esc: уйти.')
    target=OUT / ('artificer-sale.png' if sale else 'artificer-upgrade.png')
    canvas.save(target)
    print(target)


preview()
preview(True)
