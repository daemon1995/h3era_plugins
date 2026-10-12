"""Execute campaign dispatch with real ERA Delphi string routines, not string mocks.

Only memory allocation, resource lookup, Win32 file I/O and the final engine
player are provided by the harness. The plugin, string headers, concatenation,
System.Move, cache and codec selection execute their actual x86 instructions.
"""
from verify import *


class CampaignMachine(Machine):
    def __init__(self, db=0x10000000, eb=0x30000000):
        super().__init__(db,eb)
        self.discovery()
        self.imports()
        self.allocations = {}
        self.lookups = []
        self.players = []
        self.put('CodeStart',0x401000)
        self.put('CodeSize',0x230000)
        self.put('L_0040315B',self.read32(0x44d974))
        self.put('L_0040315F',self.read32(0x5ff668))
        # Install the same jump as OnEraReady, retaining the game's real prologue.
        self.u.mem_write(0x597876,b'\xe9'+struct.pack('<i',self.addr('PlayVideoById')-0x59787b))
        self.write32(self.get('VideoTableAddress'),0x13579)
        self.stub(self.eb+era_sym['System.@GetMem'],self.getmem)
        self.stub(self.eb+era_sym['System.@FreeMem'],lambda:self.return_(0))
        self.stub(self.eb+era_sym['System.@ReallocMem'],self.reallocmem)
        self.stub(self.get('FindRedirectFunc'),self.redirect)
        self.stub(self.get('FindVideoFunc'),self.find_video)
        self.stub(0x5978f0,lambda:self.return_(0))
        self.stub(0x598df0,lambda:self.player('smk',24))
        self.stub(0x44d500,lambda:self.player('bik',20))
        self.item = HEAP+0x1f0000
        self.u.mem_write(self.item,struct.pack('<III',0x1234,0x5678,100))
        self.stub(STOP+0xd00,lambda:self.return_(0x5678,16))
        self.write32(0x63a1a0,STOP+0xd00)
        self.stub(STOP+0xd10,self.read_file)
        self.write32(0x63a0f4,STOP+0xd10)
        self.extension_snapshot = self.read_extensions()

    def allocate(self,size):
        if not 0<size<0x10000:
            raise AssertionError(f'Invalid Delphi string allocation: {size:#x}')
        addr=self.heap
        self.heap+=(size+15)&~15
        assert self.heap<HEAP+0x1e0000
        self.allocations[addr]=size
        return addr

    def getmem(self):
        self.return_(self.allocate(self.reg(UC_X86_REG_EAX)))

    def reallocmem(self):
        slot=self.reg(UC_X86_REG_EAX)
        size=self.reg(UC_X86_REG_EDX)
        old=self.read32(slot)
        new=self.allocate(size)
        if old:
            self.u.mem_write(new,bytes(self.u.mem_read(old,min(size,self.allocations[old]))))
        self.write32(slot,new)
        self.return_(0)

    def cstr(self,addr):
        return bytes(self.u.mem_read(addr,128)).split(b'\0')[0]

    def redirect(self):
        source=self.reg(UC_X86_REG_EAX)
        assert self.read32(self.reg(UC_X86_REG_EDX))==source
        name=self.cstr(source)
        assert self.read32(source-4)==len(name), 'real Delphi length header'
        assert name.endswith((b'.bik',b'.smk')), name
        self.return_(0)

    def find_video(self):
        name=self.cstr(self.reg(UC_X86_REG_EDX))
        self.lookups.append(name)
        found = ((self.mode in ('smk','bik','old_bik')) and
                 name.endswith(b'.smk' if self.mode=='smk' else b'.bik'))
        self.return_(self.item if found else 0)

    def read_file(self):
        sp=self.reg(UC_X86_REG_ESP)
        assert self.read32(sp+12)==4
        self.u.mem_write(self.read32(sp+8),b'BIKb' if self.mode=='old_bik' else b'BIKi')
        self.write32(self.read32(sp+16),4)
        self.return_(1,20)

    def player(self,codec,clean):
        self.players.append((codec,self.reg(UC_X86_REG_ECX),
            [self.read32(self.reg(UC_X86_REG_ESP)+4+i*4) for i in range(clean//4)]))
        self.return_(1,clean)

    def read_extensions(self):
        # The bug corrupts the globals and attempts to use their contents as a
        # Delphi string header. Compare headers as well as the text afterward.
        return (bytes(self.u.mem_read(self.addr('BikExtension'),8)),
                bytes(self.u.mem_read(self.get('BikExtension')-12,17)),
                bytes(self.u.mem_read(self.get('SmkExtension')-12,17)))

    def campaign(self,index,mode):
        self.mode=mode
        name=self.video_name(index)
        before=len(self.lookups)
        player_before=len(self.players)
        args=(132,0,0,1,0,1)  # Six stack arguments from the supplied crash log.
        saved={UC_X86_REG_EBX:0x123,UC_X86_REG_ESI:0x456,
               UC_X86_REG_EDI:0x789,UC_X86_REG_EBP:0xabc}
        result=self.run(0x597870,args,{**saved,UC_X86_REG_ECX:index,UC_X86_REG_EDX:190})
        assert self.reg(UC_X86_REG_ESP)==STACK+0xff00+28, 'fastcall stack imbalance'
        assert all(self.reg(r)==v for r,v in saved.items()), 'callee-saved register changed'
        if mode!='missing':
            expected='smk' if mode=='smk' else 'bik'
            assert self.players[player_before:]==[(expected,index,list(args if expected=='smk' else args[:5]))]
            assert result==1
        else:
            assert len(self.players)==player_before
        assert self.read_extensions()==self.extension_snapshot, 'extension memory corrupted'
        return name,self.lookups[before:]

    def name_pointer(self,index):
        pointer=self.read32(self.read32(0x44d563)+20*index)
        if pointer==self.read32(0x44d569):
            pointer=self.read32(self.read32(0x44d5c5)+20*index)
        return pointer

    def video_name(self,index):
        return self.cstr(self.name_pointer(index))


def test_campaigns(expect_broken=False):
    m=CampaignMachine()
    if expect_broken:
        # Reintroduce the first release's pointer indirection mistake only in
        # the emulated image; the DLL on disk remains untouched.
        m.put('SmkExtensionSlot',m.addr('SmkExtension'))
        try:
            m.campaign(113,'smk')
        except AssertionError as error:
            assert 'Invalid Delphi string allocation' in str(error), error
            print('REPRODUCED: H3x2_ELa SMK fallback passes a global variable as a Delphi string:',error)
            return
        raise AssertionError('Expected the original campaign crash')
    # Check all 300 IDs supported by the original plugin's cache. Many campaign
    # sequences share names, so reuse the actual game table instead of fixtures.
    indices=[]
    for i in range(300):
        pointer=m.name_pointer(i)
        if 0x400000<=pointer<0x400000+game.OPTIONAL_HEADER.SizeOfImage-128:
            if m.cstr(pointer).lower().startswith(b'h3'):
                indices.append(i)
    assert 113 in indices
    for db,eb in [(0x10000000,0x30000000),(0x24000000,0x55000000)]:
        for mode in ('smk','bik','old_bik','missing'):
            m=CampaignMachine(db,eb)
            for index in indices:
                name,lookups=m.campaign(index,mode)
                expected=[name+b'.bik']+([name+b'.smk'] if mode in ('smk','missing') else [])
                assert lookups==expected,(mode,index,lookups)
                # Repeat the same ID to execute the cached route too.
                _,lookups=m.campaign(index,mode)
                assert lookups==[],(mode,index,'cache did not suppress lookup')
    print(f'PASS: {len(indices)} campaign IDs, BIKi/BIKb/SMK/missing, first and cached playback, two ASLR bases; real ERA string code')


if __name__=='__main__':
    test_campaigns('--expect-broken' in sys.argv)
