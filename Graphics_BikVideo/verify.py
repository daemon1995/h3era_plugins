"""Run actual rebuilt x86 DLL code against the installed ERA image in Unicorn."""
from pathlib import Path
import re, struct, sys
ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / 'build/python'))
sys.path.insert(0, str(ROOT / 'analysis'))
import pefile
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *
from inspect_binary import symbols

DLL = ROOT / 'build/Video playback in BIK format.dll'
ERA = Path('D:/games/HoMM 3 ERA/era.dll')
GAME = Path('D:/games/HoMM 3 ERA/h3era.exe')
pe = pefile.PE(str(DLL))
era = pefile.PE(str(ERA))
game = pefile.PE(str(GAME))
original = pefile.PE('C:/Users/dimon/Downloads/Telegram Desktop/Video playback in BIK format.dll')
sym = {name:int(addr,16)-pe.OPTIONAL_HEADER.ImageBase for name,addr in
       re.findall(r'^\s+[0-9A-Fa-f]+:[0-9A-Fa-f]+\s+(\S+)\s+([0-9A-Fa-f]{8})\s',
                  ROOT.joinpath('build/video.map').read_text(), re.M)}
era_sym = symbols()
STOP = 0x70000000
STACK = 0x71000000
HEAP = 0x72000000

class Machine:
    def __init__(self, dll_base=0x10000000, era_base=0x30000000):
        self.u = Uc(UC_ARCH_X86, UC_MODE_32)
        self.db, self.eb = dll_base, era_base
        self.map_pe(pe,dll_base)
        self.map_pe(era,era_base)
        self.map_pe(game,game.OPTIONAL_HEADER.ImageBase)
        for addr,size in [(STOP,0x1000),(STACK,0x10000),(HEAP,0x200000)]:
            self.u.mem_map(addr,size)
        self.heap = HEAP
        self.callbacks = {}
        self.codec_calls = []
        self.writes = []
        self.u.hook_add(UC_HOOK_CODE,self.hook,begin=STOP,end=STOP)
        self.put('EraBase',era_base)

    def map_pe(self,p,base):
        p = pefile.PE(data=p.__data__[:])
        p.relocate_image(base)
        self.u.mem_map(base,(p.OPTIONAL_HEADER.SizeOfImage+4095)&~4095)
        self.u.mem_write(base,p.get_memory_mapped_image())

    def addr(self,name): return self.db+sym[name]
    def read32(self,addr): return struct.unpack('<I',self.u.mem_read(addr,4))[0]
    def write32(self,addr,value): self.u.mem_write(addr,struct.pack('<I',value & 0xffffffff))
    def put(self,name,value): self.write32(self.addr(name),value)
    def get(self,name): return self.read32(self.addr(name))
    def reg(self,r): return self.u.reg_read(r)
    def hook(self,u,addr,size,_):
        if addr==STOP:
            u.emu_stop()
        elif addr in self.callbacks:
            self.callbacks[addr]()

    def return_(self,value=0,clean=0):
        sp = self.reg(UC_X86_REG_ESP)
        ret = self.read32(sp)
        self.u.reg_write(UC_X86_REG_EAX,value & 0xffffffff)
        self.u.reg_write(UC_X86_REG_ESP,sp+4+clean)
        self.u.reg_write(UC_X86_REG_EIP,ret)

    def run(self,name,args=(),regs=None):
        address = name if isinstance(name,int) else self.addr(name)
        sp = STACK+0xff00
        self.u.mem_write(sp,struct.pack('<'+'I'*(1+len(args)),STOP,*args))
        self.u.reg_write(UC_X86_REG_ESP,sp)
        for r,v in (regs or {}).items(): self.u.reg_write(r,v)
        try:
            self.u.emu_start(address,STOP+1,count=40000000)
        except Exception:
            print('CPU failure:',name,'EIP',hex(self.reg(UC_X86_REG_EIP)),'ESP',hex(self.reg(UC_X86_REG_ESP)),
                  'stack',bytes(self.u.mem_read(self.reg(UC_X86_REG_ESP),24)).hex())
            raise
        assert self.reg(UC_X86_REG_EIP)==STOP, (name,hex(self.reg(UC_X86_REG_EIP)))
        return self.reg(UC_X86_REG_EAX)

    def stub(self,addr,fn):
        self.callbacks[addr]=fn
        self.u.hook_add(UC_HOOK_CODE,self.hook,begin=addr,end=addr)
        self.u.mem_write(addr,b'\xc3')

    def imports(self):
        for descriptor in pe.DIRECTORY_ENTRY_IMPORT:
            for imp in descriptor.imports:
                addr = STOP+0x100+len(self.callbacks)*16
                name=imp.name.decode()
                if name=='VirtualProtect':
                    def protect():
                        sp=self.reg(UC_X86_REG_ESP)
                        self.write32(self.read32(sp+16),0x20)
                        self.return_(1,16)
                    fn=protect
                elif name=='GetCurrentProcess': fn=lambda:self.return_(0xffffffff)
                elif name=='FlushInstructionCache': fn=lambda:self.return_(1,12)
                elif name=='GetProcessHeap': fn=lambda:self.return_(1)
                elif name=='HeapAlloc':
                    def alloc():
                        sp=self.reg(UC_X86_REG_ESP)
                        size=self.read32(sp+12)
                        addr=self.heap; self.heap+=(size+15)&~15
                        self.return_(addr,12)
                    fn=alloc
                else: continue
                self.stub(addr,fn)
                self.write32(self.db+imp.address-pe.OPTIONAL_HEADER.ImageBase,addr)

    def discovery(self):
        assert self.run('DiscoverEraVideo')==1
        expected = {'FromPCharReal':'System.@LStrFromPChar','ConcatStringFunc':'System.@LStrCat3',
                    'FindRedirectFunc':'Lodman.FindRedirection','VideoTableAddress':'SndVid.VidFiles'}
        for dst,src in expected.items():
            assert self.get(dst)==self.eb+era_sym[src], (dst,hex(self.get(dst)))
        assert self.u.mem_read(self.get('BikExtension'),5)==b'.bik\0'
        assert self.u.mem_read(self.get('SmkExtension'),5)==b'.smk\0'
        assert self.get('BikExtensionSlot')==self.addr('BikExtension')
        assert self.get('SmkExtensionSlot')==self.get('SmkExtension'), 'SMK caller requires the string itself'

def test_discovery():
    for db,eb in [(0x10000000,0x30000000),(0x24000000,0x55000000)]:
        m=Machine(db,eb); m.discovery()
        before=bytes(m.u.mem_read(0x44d000,0x1000))
        # A real handler opcode mutation must cause a clean refusal before game patches.
        m.u.mem_write(eb+era_sym['SndVid.Splice_OpenBik']+0x36,b'\x90')
        assert m.run('DiscoverEraVideo')==0
        assert before==bytes(m.u.mem_read(0x44d000,0x1000))
    print('PASS: real ERA 3.9.31 handlers found at two ASLR bases; malformed handler rejected')

def test_old_failure():
    m=Machine(); ob=0x18000000; m.map_pe(original,ob)
    # The actual StdSplice fastcall(2) stub format generated by ApiJack.StdSplice.
    stub=STOP+0x800
    handler=m.eb+era_sym['SndVid.Splice_OpenBik']
    m.u.mem_write(stub,b'\x58\x52\x51\x68'+struct.pack('<I',0x44d270)+b'\x50\xe9'+struct.pack('<i',handler-(stub+14)))
    m.u.mem_write(0x44d270,b'\xe9'+struct.pack('<i',stub-0x44d275))
    # Let the legacy SMK decoder reach BIK, then stop on the original unsupported path.
    smkstub=STOP+0xa00
    smkhandler=STOP+0xb00
    m.u.mem_write(0x598a90,b'\xe9'+struct.pack('<i',smkstub-0x598a95))
    m.u.mem_write(smkstub+3,b'\xb8'+struct.pack('<I',smkhandler))
    m.u.mem_write(smkhandler+0x2b,b'\xba'+struct.pack('<I',0x12345678))
    failed=[]
    def stop_failure(): failed.append(True); m.u.emu_stop()
    m.u.hook_add(UC_HOOK_CODE,lambda u,a,s,d:stop_failure() if a==ob+0x1995 else None)
    m.u.reg_write(UC_X86_REG_ESP,STACK+0xff00)
    m.u.mem_write(STACK+0xff00,struct.pack('<I',STOP))
    m.u.emu_start(ob+0x19ab,STOP,count=10000)
    assert failed
    m.discovery()
    print('PASS: original bridge decoder fails on StdSplice; replacement succeeds with same ERA')

def test_switches():
    m=Machine(); m.discovery(); m.imports()
    m.put('CodeStart',0x401000); m.put('CodeSize',0x230000)
    m.put('L_0040315B',m.read32(0x44d974))
    m.put('L_0040315F',m.read32(0x5ff668))
    for i,name in enumerate(['BinkOpen','BinkDoFrame','BinkCopyToBuffer','BinkWait','BinkClose','BinkNextFrame',
                             'BinkGetRects','BinkPause','BinkDDSurfaceType','BinkGoto','BinkGetSummary',
                             'BinkOpenMiles','BinkOpenWaveOut','BinkSetSoundSystem']):
        # Production uses addresses of these pointer slots in engine call operands.
        m.put(name,STOP+0xc00+i*16)
    sites=[0x44d6cc,0x44d7bd,0x44d6ed,0x44d73c,0x599cfc,0x44d8d0,0x44d730,0x44d79b,
           0x44d974,0x599cf1,0x597e0b,0x44d719,0x44d7d7,0x44d768,0x44d546,0x597580,
           0x597ad8,0x597903,0x44d99b,0x597b8d,0x44d58d,0x44d5ef,0x44d8e1,0x44db4b,
           0x44db6c,0x45e400,0x45efaa,0x45f08a,0x45f0ad,0x59901f,0x44d9a2,0x44d59e,
           0x44d5fc,0x44d857,0x44d8db,0x44db58,0x44db78,0x5ff668]
    initial={a:bytes(m.u.mem_read(a,4)) for a in sites}
    regs={UC_X86_REG_EAX:0x1111,UC_X86_REG_EBX:0x2222,UC_X86_REG_ECX:0x3333,
          UC_X86_REG_EDX:0x4444,UC_X86_REG_ESI:0x5555,UC_X86_REG_EDI:0x6666,UC_X86_REG_EBP:0x7777}
    m.run('SwitchToModern',regs=regs)
    assert all(m.reg(r)==v for r,v in regs.items())
    assert m.u.mem_read(m.addr('ModernMode'),1)==b'\1'
    assert m.read32(0x44d6cc)==m.addr('BinkDoFrame')
    m.run('SwitchToLegacy',regs=regs)
    assert all(m.reg(r)==v for r,v in regs.items())
    assert m.u.mem_read(m.addr('ModernMode'),1)==b'\0'
    assert all(initial[a]==bytes(m.u.mem_read(a,4)) for a in sites)
    print('PASS: all 38 BIK call operands restored after modern/legacy codec switching; registers preserved')

def test_open_video():
    for modern,resource in [(False,True),(True,True),(True,False)]:
        m=Machine(); m.discovery(); m.imports()
        m.put('CodeStart',0x401000); m.put('CodeSize',0x230000)
        m.u.mem_write(m.addr('ModernMode'),bytes([modern]))
        filehandle=0x1234
        item=HEAP+0x1000
        m.u.mem_write(item,struct.pack('<III',filehandle,0x5678,100))
        m.write32(m.get('VideoTableAddress'),0x13579)
        def convert():
            assert m.reg(UC_X86_REG_ECX)==0, 'Delphi codepage argument must be initialized'
            m.return_(0)
        m.stub(m.get('FromPCharReal'),convert)
        m.stub(m.get('ConcatStringFunc'),lambda:m.return_(0))
        m.stub(m.get('FindRedirectFunc'),lambda:m.return_(0))
        m.stub(m.get('FindVideoFunc'),lambda:m.return_(item if resource else 0))
        def seek():
            sp=m.reg(UC_X86_REG_ESP)
            assert m.read32(sp+4)==filehandle and m.read32(sp+8)==0x5678
            m.return_(0x5678,16)
        seekaddr=STOP+0xd00; m.stub(seekaddr,seek); m.write32(0x63a1a0,seekaddr)
        m.stub(0x59aad0,lambda:m.return_(0))
        sound=HEAP+0x1800; m.write32(0x699414,sound); m.write32(sound+0x3c,0x99)
        calls=[]
        def codec():
            sp=m.reg(UC_X86_REG_ESP)
            calls.append((m.read32(sp+4),m.read32(sp+8)))
            m.return_(0x76543210,8)
        codecaddr=STOP+0xe00; m.stub(codecaddr,codec)
        m.put('BinkOpen',codecaddr); m.write32(0x63a390,codecaddr)
        setsound=STOP+0xe10; m.stub(setsound,lambda:m.return_(1,8)); m.put('BinkSetSoundSystem',setsound)
        result=m.run('L_00401827',regs={UC_X86_REG_ECX:HEAP+0x1900,UC_X86_REG_EDX:0x20})
        assert result==(0x76543210 if resource else 0)
        if resource:
            assert calls==[(filehandle,0x20|(0x800000 if modern else 0x8000000))],calls
        else: assert calls==[]
    print('PASS: archived BIK uses correct old/new handle flags; missing video returns NULL')

def test_initialization():
    m=Machine(); m.imports()
    def cstr(addr):
        data=bytes(m.u.mem_read(addr,128))
        return data.split(b'\0')[0].decode('ascii')
    errors=[]
    codec_funcs={}
    for descriptor in pe.DIRECTORY_ENTRY_IMPORT:
        for imp in descriptor.imports:
            name=imp.name.decode()
            addr=STOP+0x200+len(m.callbacks)*16
            if name=='GetModuleHandleA':
                fn=lambda:m.return_(0x400000)
                module_stub=addr
            elif name=='LoadLibraryA':
                fn=lambda:m.return_(0x80000000,4)
            elif name=='GetProcAddress':
                def getproc():
                    sp=m.reg(UC_X86_REG_ESP)
                    procname=cstr(m.read32(sp+8))
                    fnaddr=STOP+0x600+len(codec_funcs)*16
                    codec_funcs[procname]=fnaddr
                    m.return_(fnaddr,8)
                fn=getproc
            elif name=='MessageBoxA':
                def error():
                    errors.append(True); m.return_(0,16)
                fn=error
            else: continue
            if name=='GetModuleHandleA':
                fn=lambda:m.return_(0x400000,4)
            m.stub(addr,fn)
            m.write32(m.db+imp.address-pe.OPTIONAL_HEADER.ImageBase,addr)
    # Skip optional INI lookup through game IAT; it is independent of hook setup.
    m.write32(0x63a230,0)
    m.run('OnEraReady',args=(0,))
    assert not errors
    assert m.get('PluginReady')==1
    for operand,target,nextip in [(0x44d575,'L_0040177C',0x44d579),
                                  (0x44d5d4,'L_00401781',0x44d5d8),
                                  (0x44d6b1,'L_004010C4',0x44d6b5),
                                  (0x597877,'PlayVideoById',0x59787b),
                                  (0x44d539,'L_00401524',0x44d53d)]:
        assert (m.read32(operand)+nextip)&0xffffffff==m.addr(target),(hex(operand),target)
    assert m.get('BinkOpen')==codec_funcs['_BinkOpen@8']
    snapshot=bytes(m.u.mem_read(0x44d000,0x1000))
    m.run('OnEraReady',args=(0,))
    assert snapshot==bytes(m.u.mem_read(0x44d000,0x1000))
    print('PASS: ERA ready event installs all five hooks with correct relocated jump targets; repeated event is harmless')

if __name__ == '__main__':
    test_discovery()
    test_old_failure()
    test_switches()
    test_open_video()
    test_initialization()
    assert pe.FILE_HEADER.Machine==0x14c
    assert not any(d.dll.lower() in [b'msvcrt.dll',b'vcruntime140.dll'] for d in pe.DIRECTORY_ENTRY_IMPORT)
    print(f'PASS: x86 DLL, no CRT dependency, {DLL.stat().st_size} bytes')

