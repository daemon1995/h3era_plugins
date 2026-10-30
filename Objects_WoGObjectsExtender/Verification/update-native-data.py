h3_NullString"Rebuild shipped WoG prototype rows from the installed WoG archive, preserving locale text.h3_NullString"
import json
import re
import struct
import zlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GAME = Path('D:/games/HoMM 3 ERA')
with (GAME / 'Mods/WoG/Data/hmm35wog.pac').open('rb') as f:
    count = struct.unpack_from('<I', f.read(92), 8)[0]
    directory = f.read(count * 32)
    for i in range(count):
        name, offset, size, _, packed = struct.unpack_from('<16s4I', directory, i * 32)
        if name.split(b'\0')[0].lower() == b'zobjcts.txt':
            f.seek(offset)
            content = f.read(packed or size)
            content = zlib.decompress(content) if packed else content
            break
    else:
        raise RuntimeError('WoG zobjcts.txt missing')
properties = {}
for line in content.decode('cp1252').splitlines():
    fields = line.split()
    if len(fields) == 9 and fields[5] == '63':
        fields[5] = '141'
        properties.setdefault(fields[6], []).append(' '.join(fields))

names = {
    '2': ('Venus Fountain', 'Фонтан Венеры', 2500, 30),
    '21': ('Summoning Stones', 'Камни призыва', 7000, 15),
    '48': ('Berserker Fountain', 'Фонтан берсерка', 3000, 30),
    '57': ('Adventure Cave', 'Пещера приключений', 10000, 15),
    '62': ('School of Wizardry', 'Школа волшебства', 5000, 25),
}
translations = {
 'common.unavailable': ('No service is available at present.', 'Сейчас нет доступных услуг.'),
 'academy.menu': ('{Battle Academy}\nChoose one available service. The price is charged when you select its picture. One service per hero per day.', '{Боевая академия}\nВыберите услугу. Указанная стоимость списывается при выборе картинки. Одна услуга на героя в день.'),
 'potions.choose': ('%hero: potions before battle', '%hero: зелья перед боем'),
 'potions.select': ('Mixing Hydromel and Berserker potion may permanently reduce Spell Power by 1.', 'Смешение медовухи и зелья берсерка может навсегда снизить силу магии на 1.'),
 'potions.effectHydromel': ('{Hydromel} — doses: %doses.\n+2 Attack and Defense for this battle.', '{Медовуха} — порций: %doses.\n+2 к атаке и защите на этот бой.'),
 'potions.effectBerserker': ('{Berserker potion} — doses: %doses.\n+10 Attack, maximum Luck and no mana for this battle.', '{Зелье берсерка} — порций: %doses.\n+10 к атаке, максимальная удача и отсутствие маны на этот бой.'),
 'potions.effectVenus': ('{Venus potion} — doses: %doses.\nMaximum Luck and Morale for this battle.', '{Зелье Венеры} — порций: %doses.\nМаксимальные удача и боевой дух на этот бой.'),
 'common.more': ('Show the next offers?', 'Показать следующие предложения?'),
 'potions.hydromel': ('Hydromel', 'медовуха'),
 'potions.berserker': ('Berserker potion', 'зелье берсерка'),
 'potions.venus': ('Venus potion', 'зелье Венеры'),
 'potions.refill': ('%hero receives %doses doses: %potion.', '%hero получает %doses порции: %potion.'),
 'potions.drinkHydromel': ('%hero: drink Hydromel? +2 Attack and Defense for this battle. Doses: %doses.', '%hero: выпить медовуху? +2 к атаке и защите на этот бой. Осталось порций: %doses.'),
 'potions.drinkBerserker': ('%hero: drink Berserker potion? +10 Attack and maximum Luck, but no mana during this battle. Doses: %doses.', '%hero: выпить зелье берсерка? +10 к атаке и максимальная удача, но без маны на этот бой. Осталось порций: %doses.'),
 'potions.drinkVenus': ('%hero: drink Venus potion? Maximum Luck and Morale for this battle. Doses: %doses.', '%hero: выпить зелье Венеры? Максимальные удача и боевой дух на этот бой. Осталось порций: %doses.'),
 'potions.mixed': ('Mixing potions costs %hero one point of Spell Power.', 'Смешение зелий отнимает у героя %hero одну единицу силы магии.'),
 'school.menu': ('{School of Wizardry}\nChoose a service: Spell Power, Knowledge, secondary skill, daily spell, or copying scrolls.', '{Школа волшебства}\nВыберите услугу: сила магии, знания, вторичный навык, заклинание дня или изучение свитков.'),
 'school.visited': ('This hero already used a school service today.', 'Сегодня этот герой уже пользовался услугами школы.'),
 'school.learned': ('Your studies are complete.', 'Обучение завершено.'),
 'school.unavailable': ('This service is unavailable.', 'Эта услуга недоступна.'),
 'school.confirm': ('Learn %name for %gold gold?', 'Изучить %name за %gold золота?'),
 'school.copy': ('Copy %spell from a scroll for %gold gold? The scroll is consumed.', 'Изучить %spell по свитку за %gold золота? Свиток будет израсходован.'),
 'school.changed': ('The offer is no longer available.', 'Предложение больше недоступно.'),
 'summon.unavailable': ('No affordable reinforcement can be summoned today. The stones may be used once per day.', 'Сегодня нет доступных подкреплений. Камнями можно пользоваться раз в день.'),
 'summon.choose': ('{Summoning Stones}\nChoose reinforcements from your towns. Cancel to see the next page.', '{Камни призыва}\nВыберите подкрепление из ваших городов. Отмена открывает следующую страницу.'),
 'summon.recruit': ('Recruit %count %creature from %town. The full price is shown below.', 'Нанять %count %creature из города %town. Полная стоимость указана ниже.'),
 'summon.transport': ('Transport %count %creature from the garrison of %town for %gold gold.', 'Перенести %count %creature из гарнизона города %town за %gold золота.'),
 'summon.confirm': ('Summon these reinforcements?', 'Призвать это подкрепление?'),
 'summon.changed': ('The town, troop supply or resources have changed. No recruitment was completed.', 'Изменились владелец города, запас войск или ресурсы. Призыв не выполнен.'),
 'cave.choose': ('{Adventure Cave}\nChoose a passage. Cancel to leave.', '{Пещера приключений}\nВыберите проход. Отмена — выйти из пещеры.'),
 'cave.fork': ('Two passages lead into the darkness.\n\nLeft: %left\nRight: %right\n\nWhich passage will you explore?', 'Два прохода ведут в темноту.\n\nНалево: %left\nНаправо: %right\n\nКакой путь вы выберете?'),
 'cave.left': ('Left', 'Налево'),
 'cave.right': ('Right', 'Направо'),
 'cave.unknown': ('unknown', 'неизвестно'),
 'cave.emptyLabel': ('cleared', 'пусто'),
 'cave.empty': ('This chamber has already been explored.', 'Эта часть пещеры уже исследована.'),
 'cave.entrance': ('Infernal Troglodytes guard the entrance. Fight them?', 'Вход охраняют адские троглодиты. Сразиться с ними?'),
 'cave.fight': ('Continue? %count %creature await you on a special battlefield.', 'Продолжить путь? Вас ждут %count %creature на особом поле боя.'),
 'cave.fightRogues': ('The Rogues are hostile. Attack their camp?', 'Разбойники настроены враждебно. Напасть на их лагерь?'),
 'cave.victory': ('You search the defeated monsters and collect %gold gold and their treasure.', 'Вы обыскиваете логово побеждённых существ и находите %gold золота и сокровища.'),
 'cave.roguesUsed': ('These Rogues already helped this hero today.', 'Сегодня эти разбойники уже помогали этому герою.'),
 'cave.service': ('%service\nPrice: %gold gold. Accept?', '%service\nСтоимость: %gold золота. Согласиться?'),
 'cave.noService': ('The Rogues cannot fulfill this order at present.', 'Сейчас разбойники не могут выполнить этот заказ.'),
 'cave.poor': ('You cannot afford their price.', 'У вас недостаточно золота.'),
 'cave.serviceDone': ('The Rogues fulfill your order.', 'Разбойники выполняют ваш заказ.'),
 'cave.heal': ('Help the sick Hermit? An Expert in First Aid can heal him; otherwise a suitable equipped artifact must be given.', 'Помочь больному отшельнику? Эксперт первой помощи вылечит его; иначе придётся отдать подходящий надетый артефакт.'),
 'cave.hermitBlocked': ('The Hermit cannot teach you yet. %reason', 'Отшельник пока не может вас обучить. %reason'),
 'cave.lessonUsed': ('No lesson is available: you already studied today or have mastered this discipline.', 'Обучение недоступно: вы уже занимались сегодня или полностью освоили этот предмет.'),
 'cave.lesson': ('Study %lesson, lesson %level of 3?', 'Изучить предмет «%lesson»? Урок %level из 3.'),
 'cave.poolUsed': ('You already have this pool blessing until next week.', 'Благословение этого источника уже действует до следующей недели.'),
 'cave.pool': ('Your army receives Expert %spell in every battle until next week.', 'До следующей недели ваша армия получает экспертное заклинание %spell в каждом бою.'),
 'cave.orb': ('The orb changes the shroud over the map.', 'Сфера меняет завесу тумана на карте.'),
 'cave.cache': ('You find %gold gold and a special item. Local merchants may buy the item.', 'Вы находите %gold золота и особый предмет. Его можно продать местным торговцам.'),
 'cave.altarUsed': ('You lack mana or already have this altar power until next week.', 'Не хватает маны или сила этого алтаря уже действует до следующей недели.'),
 'cave.altar': ('%effect\nSpend %mana mana? The power lasts until next week.', '%effect\nПотратить %mana маны? Сила действует до следующей недели.'),
 'cave.release': ('Release the surviving Rogue captain in exchange for the location of a hidden treasure?', 'Отпустить уцелевшего главаря в обмен на сведения о спрятанном сокровище?'),
 'cave.obelisk': ('Search the Obelisk at %x, %y.', 'Ищите у обелиска в точке %x, %y.'),
 'cave.obeliskReward': ('The Rogue spoke the truth. You find gold and two artifacts.', 'Разбойник сказал правду. Вы находите золото и два артефакта.'),
 'cave.obeliskLie': ('The Rogue deceived you. There is no treasure here.', 'Разбойник обманул вас. Здесь нет сокровища.'),
 'cave.sell': ('Sell %count %item for %gold gold?', 'Продать %item — %count шт. за %gold золота?'),
 'cave.stolen': ('%hero discovers that %artifact has been stolen!', '%hero обнаруживает пропажу артефакта «%artifact»!'),
 'cave.ambush': ('Rogues ambush your hero!', 'На героя напали разбойники!'),
 'cave.mineStolen': ('Rogues have stolen one of your Gold Mines!', 'Разбойники захватили одну из ваших золотых шахт!'),
 'cave.roguesSleeping': ('The Rogues are asleep. Left: wake them and talk. Right: slaughter half their camp and fight the survivors.', 'Разбойники спят. Слева: разбудить и поговорить. Справа: перебить половину лагеря и сразиться с остальными.'),
 'cave.roguesMenu': ('Left: talk to the Rogues and hear their offer. Right: fight them.', 'Слева: поговорить с разбойниками и выслушать предложение. Справа: сразиться с ними.'),
 'cave.roguesHostile': ('The Rogues threaten your party. Left: flee, losing 2 Morale. Right: fight.', 'Разбойники угрожают вашему отряду. Слева: бежать, потеряв 2 единицы боевого духа. Справа: сразиться.'),
 'cave.wake': ('Wake the sleeping Rogues?', 'Разбудить спящих разбойников?'),
}
groups = {
 'cave.room': [('Rogue camp','Лагерь разбойников'),('Hermit','Отшельник'),('Monster lair','Логово существ'),('Underground chamber','Подземный зал')],
 'cave.service': [('Recruit 15 Rogues','Нанять 15 разбойников'),('Steal an enemy Gold Mine','Захватить вражескую золотую шахту'),('Ambush an enemy hero','Устроить засаду вражескому герою'),('Steal an enemy artifact','Украсть вражеский артефакт')],
 'cave.lesson': [('Wealth: double treasure-chest gold','Богатство: вдвое больше золота в сундуках'),('Combat: unlimited retaliation','Бой: неограниченные ответные удары'),('Magic: an extra spell cast each battle','Магия: дополнительное заклинание в каждом бою'),('Voodoo: ground and disarm enemies in melee','Вуду: лишить врага полёта и стрельбы в ближнем бою')],
 'cave.hermitRequirement': [('', ''),('Heal his illness.','Вылечите его болезнь.'),('Remove the other inhabitants from the cave.','Избавьте пещеру от остальных обитателей.'),('Put away all artifacts except war machines and the spellbook.','Освободитесь от всех артефактов, кроме боевых машин и книги заклинаний.')],
 'cave.altar': [('Venom: attacks poison their target','Яд: атаки отравляют противника'),('Fork: Magic Arrow strikes all enemy stacks','Развилка: волшебная стрела поражает все вражеские отряды'),('Aegis: first-level creatures gain double health','Эгида: здоровье существ первого уровня удваивается'),('Ferocity: +2 Attack, Defense and Speed each round','Свирепость: +2 к атаке, защите и скорости каждый раунд')],
}
for prefix, values in groups.items():
    for i, value in enumerate(values):
        branch = {'cave.service':'cave.services','cave.lesson':'cave.lessons','cave.altar':'cave.altars'}.get(prefix,prefix)
        translations[branch + '.' + str(i)] = value
# Suffixes avoid JSON leaf/branch collisions (e.g. the cave.lesson prompt vs option names).
for i, pair in enumerate([('ancient books','древние книги'),('mandrake roots','корни мандрагоры'),('tulips','тюльпаны'),('toys','игрушки'),('wine','вино'),('dog armor','собачьи доспехи'),('three-handed swords','трёхручные мечи'),('useless wands','бесполезные жезлы')],69):
    variable = i if i < 73 else i+2
    translations['cave.item.'+str(variable)] = pair
    translations['cave.journal.'+str(variable)] = (f'Adventure Cave cargo: {pair[0]}. Sell it to interested merchants.', f'Находка в Пещере приключений: {pair[1]}. Продайте её заинтересованным торговцам.')
for i in range(4):
    translations['cave.journal.'+str(64+i)] = groups['cave.lesson'][i]
for i, pair in enumerate([
    ('Receive three doses of Venus potion. Before a battle, a dose grants maximum Morale and Luck.', 'Даёт три порции зелья Венеры. Порция обеспечивает максимальные боевой дух и удачу в одном бою.'),
    ('Summon recruits or garrison troops from owned towns. One service per day.', 'Призывает пополнение или гарнизон из ваших городов. Одна услуга в день.'),
    ('Receive three doses of Berserker potion: +10 Attack and Luck, but no mana for one battle.', 'Даёт три порции зелья берсерка: +10 атаки и удача, но без маны на один бой.'),
    ('Explore branching chambers: Rogues, Hermits, special battles and treasures. Cleared rooms remain cleared.', 'Исследуйте подземные проходы: разбойники, отшельники, особые битвы и сокровища. Зачищенные залы остаются пустыми.'),
    ('Offers daily primary and secondary training, a spell, or learning spells from scrolls.', 'Ежедневное обучение основным и вторичным навыкам, заклинанию дня или магии со свитков.')]):
    translations['hint.'+list(names)[i]] = pair

def put(target, key, value):
    parts = key.split('.')
    for part in parts[:-1]:
        target = target.setdefault(part, {})
    target[parts[-1]] = value

for locale, index in [('lang/wog_objects.json', 0), ('lang/ru/wog_objects.json', 1)]:
    path = ROOT / locale
    data = json.loads(path.read_text(encoding='utf-8-sig'))
    objects = data['RMG']['objectGeneration']['141']
    for subtype, (en, ru, value, density) in names.items():
        objects[subtype] = {'name': (en,ru)[index], 'enabled':1, 'value':value, 'density':density}
    for subtype, obj in objects.items():
        obj.setdefault('enabled',1)
        if subtype not in ('51','52'): obj['properties'] = properties[subtype]
        obj.setdefault('text',{})['hint'] = data['wog_objects'].get('native',{}).get('hint',{}).get(subtype,obj['name'])
    for key, value in translations.items():
        put(data['wog_objects'].setdefault('native',{}), key, re.sub(r'%([A-Za-z]+)', r'@\1@', value[index]))
    data['wog_objects']['artificer']['ui']['helpTitle'] = ('Artificer services','Услуги Ремесленника')[index]
    for subtype in objects:
        hint = data['wog_objects']['native']['hint'].get(subtype)
        if hint: objects[subtype]['text']['hint'] = hint
    path.write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n', encoding='utf-8', newline='')
print('Updated 23 RMG entries, native prototypes and two locales.')
