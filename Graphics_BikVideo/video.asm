; Video playback in BIK format, x86 MASM, without C/C++ runtime.
; ERA 3.9.31 / Delphi ANSI-string Splice_OpenBik and Splice_OpenSmack.
.686
.model flat
option casemap:none

EXTERN __imp__GetModuleHandleA@4:DWORD
EXTERN __imp__GetProcAddress@8:DWORD
EXTERN __imp__LoadLibraryA@4:DWORD
EXTERN __imp__DisableThreadLibraryCalls@4:DWORD
EXTERN __imp__VirtualProtect@16:DWORD
EXTERN __imp__FlushInstructionCache@12:DWORD
EXTERN __imp__GetCurrentProcess@0:DWORD
EXTERN __imp__GetProcessHeap@0:DWORD
EXTERN __imp__HeapAlloc@12:DWORD
EXTERN __imp__MessageBoxA@16:DWORD
EXTERN __imp__MessageBoxW@16:DWORD

PUBLIC _DllEntry@12, OnEraReady, DiscoverEraVideo, ConvertPChar, SwitchToModern, SwitchToLegacy
PUBLIC EraBase, EraEnd, BinkOpen, FromPCharReal, BikExtension, SmkExtension
PUBLIC PluginReady, ProtectCode, RestoreCode

_DATA SEGMENT PARA PUBLIC USE32 'DATA'
EraName db 'era.dll',0
RegisterName db 'RegisterHandler',0
EraReadyName db 'OnAfterStructRelocations',0
CodecName db 'binkw32a.dll',0
IniApiName db 'ReadStrFromIni',0
IniFile db 'heroes3.ini',0
IniSection db 'Settings',0
IniKey db 'BinkSoundType',0
PluginTitle dw 'B','I','K',' ','v','i','d','e','o',0
ErrorTitle db 'BIK video plugin',0
UnsupportedText db 'Unsupported ERA video handlers. BIK plugin was not activated.',0
MissingCodecText db 'Cannot load all required functions from binkw32a.dll. BIK plugin was not activated.',0
EraBase dd 0
EraEnd dd 0
FromPCharReal dd 0
BikExtension dd 0
SmkExtension dd 0
PluginReady dd 0
CodeStart dd 0
CodeSize dd 0
OldProtection dd 0
UnusedProtection dd 0
CodecFunctions LABEL DWORD
BinkOpen dd 0
BinkDoFrame dd 0
BinkCopyToBuffer dd 0
BinkWait dd 0
BinkClose dd 0
BinkNextFrame dd 0
BinkGetRects dd 0
BinkPause dd 0
BinkDDSurfaceType dd 0
BinkGoto dd 0
BinkGetSummary dd 0
BinkOpenMiles dd 0
BinkOpenWaveOut dd 0
BinkSetSoundSystem dd 0
CodecNames dd OFFSET NameOpen, OFFSET NameDoFrame, OFFSET NameCopy, OFFSET NameWait
           dd OFFSET NameClose, OFFSET NameNext, OFFSET NameRects, OFFSET NamePause
           dd OFFSET NameSurface, OFFSET NameGoto, OFFSET NameSummary, OFFSET NameMiles
           dd OFFSET NameWave, OFFSET NameSound
NameOpen db '_BinkOpen@8',0
NameDoFrame db '_BinkDoFrame@4',0
NameCopy db '_BinkCopyToBuffer@28',0
NameWait db '_BinkWait@4',0
NameClose db '_BinkClose@4',0
NameNext db '_BinkNextFrame@4',0
NameRects db '_BinkGetRects@8',0
NamePause db '_BinkPause@8',0
NameSurface db '_BinkDDSurfaceType@4',0
NameGoto db '_BinkGoto@12',0
NameSummary db '_BinkGetSummary@8',0
NameMiles db '_BinkOpenMiles@4',0
NameWave db '_BinkOpenWaveOut@4',0
NameSound db '_BinkSetSoundSystem@8',0
L_004010BC dd 0044D6B1h
L_004010C0 dd 0044D6B5h
AllocationSlot dd OFFSET PluginAlloc
_DATA ENDS

_TEXT SEGMENT PARA PUBLIC USE32 'CODE'
_DllEntry@12 PROC
    cmp dword ptr [esp+8],1
    jne entry_ok
    push dword ptr [esp+4]
    call dword ptr [__imp__DisableThreadLibraryCalls@4]
    push OFFSET EraName
    call dword ptr [__imp__GetModuleHandleA@4]
    test eax,eax
    jz entry_ok
    mov [EraBase],eax
    push OFFSET RegisterName
    push eax
    call dword ptr [__imp__GetProcAddress@8]
    test eax,eax
    jz entry_ok
    push OFFSET EraReadyName
    push OFFSET OnEraReady
    call eax
entry_ok:
    mov eax,1
    ret 12
_DllEntry@12 ENDP

; Fires after ERA's snd/vid hooks and all AfterWoG binary patches are installed.
OnEraReady PROC
    pushad
    cmp [PluginReady],0
    jne after_done
    ; Verify the fixed SoD address layout used by the original plugin.
    push 0
    call dword ptr [__imp__GetModuleHandleA@4]
    cmp eax,00400000h
    jne unsupported
    cmp dword ptr [00497873h],0677660BFh
    jne unsupported
    call DiscoverEraVideo
    test eax,eax
    jz unsupported
    call LoadCodec
    test eax,eax
    jz missing_codec
    ; First executable section of the fixed-address game image.
    mov eax,dword ptr [0040003Ch]
    add eax,00400000h
    movzx ecx,word ptr [eax+20]
    lea eax,[eax+ecx+24]
    mov edx,[eax+12]
    add edx,00400000h
    mov [CodeStart],edx
    mov ecx,[eax+8]
    mov [CodeSize],ecx
    call ProtectCode
    test eax,eax
    jz unsupported
    ; Original initialization body, without its legacy loader-hook return shim.
    call L_004010E4
    mov [PluginReady],1
    call RestoreCode
    jmp after_done
unsupported:
    mov eax,OFFSET UnsupportedText
    jmp after_error
missing_codec:
    mov eax,OFFSET MissingCodecText
after_error:
    push 10h
    push OFFSET ErrorTitle
    push eax
    push 0
    call dword ptr [__imp__MessageBoxA@16]
after_done:
    popad
    ret 4
OnEraReady ENDP

; Input EAX = an ERA address, output CF clear if at least 16 bytes are in image.
InEra PROC
    cmp eax,[EraBase]
    jb range_bad
    push edx
    lea edx,[eax+16]
    cmp edx,eax
    jb range_bad_pop
    cmp edx,[EraEnd]
    ja range_bad_pop
    pop edx
    clc
    ret
range_bad_pop:
    pop edx
range_bad:
    stc
    ret
InEra ENDP

; Decode a checked E8 instruction. Returns target in EAX, CF from InEra.
CallTarget PROC
    mov eax,[eax+1]
    lea eax,[eax+edx+5]
    jmp InEra
CallTarget ENDP

DiscoverEraVideo PROC
    push ebx
    push esi
    push edi
    mov byte ptr [VideoLookupReady],0
    mov [BikExtension],0
    mov [SmkExtension],0
    mov ebx,[EraBase]
    test ebx,ebx
    jz discover_fail
    cmp word ptr [ebx],5A4Dh
    jne discover_fail
    mov eax,[ebx+3Ch]
    cmp eax,1000h
    ja discover_fail
    add eax,ebx
    cmp dword ptr [eax],4550h
    jne discover_fail
    mov edx,[eax+50h]
    cmp edx,1000h
    jb discover_fail
    add edx,ebx
    jc discover_fail
    mov [EraEnd],edx
    ; Scan only SizeOfCode starting at BaseOfCode, not arbitrary hook bridges.
    mov esi,[eax+2Ch]
    add esi,ebx
    mov edi,[eax+1Ch]
    add edi,esi
    sub edi,64
    cmp edi,[EraEnd]
    ja discover_fail
scan_next:
    cmp esi,edi
    jae scan_done
    inc esi
    cmp byte ptr [esi],0B9h ; mov ecx, Delphi extension string
    jne scan_next
    cmp byte ptr [esi+5],0E8h ; call @LStrCat3
    jne scan_next
    cmp dword ptr [esi-6],08DF8558Bh ; mov edx,[ebp-8] / lea eax,[ebp-4]
    jne scan_next
    cmp word ptr [esi-2],0FC45h
    jne scan_next
    mov eax,[esi+1]
    call InEra
    jc scan_next
    cmp dword ptr [eax],06B6D732Eh ; .smk
    jne maybe_bik
    cmp byte ptr [eax+4],0
    jne scan_next
    mov [SmkExtension],eax
    jmp scan_next
maybe_bik:
    cmp dword ptr [eax],06B69622Eh ; .bik
    jne scan_next
    cmp byte ptr [eax+4],0
    jne scan_next
    mov [BikExtension],eax
    ; Validate the complete post-concatenation lookup sequence before extracting.
    cmp dword ptr [esi+10],00FFC558Dh ; lea edx,[ebp-4] / movzx ...
    jne discover_fail
    cmp word ptr [esi+14],00DB6h
    jne discover_fail
    cmp dword ptr [esi+20],0E8FC458Bh ; mov eax,[ebp-4] / call FindRedirection
    jne discover_fail
    cmp dword ptr [esi+28],0A1FC558Bh ; mov edx,[ebp-4] / mov eax,[VidFiles]
    jne discover_fail
    cmp byte ptr [esi+36],0E8h
    jne discover_fail
    cmp byte ptr [esi-11],0E8h
    jne discover_fail
    cmp byte ptr [esi-16],0B9h
    jne discover_fail
    cmp dword ptr [esi-15],0 ; modern @LStrFromPChar codepage parameter
    jne discover_fail
    lea edx,[esi-11]
    mov eax,edx
    call CallTarget
    jc discover_fail
    mov [FromPCharReal],eax
    mov dword ptr [ConvertPCharFunc],OFFSET ConvertPChar
    lea edx,[esi+5]
    mov eax,edx
    call CallTarget
    jc discover_fail
    mov dword ptr [ConcatStringFunc],eax
    mov eax,[esi+16]
    call InEra
    jc discover_fail
    mov dword ptr [RedirectModeAddress],eax
    lea edx,[esi+23]
    mov eax,edx
    call CallTarget
    jc discover_fail
    mov dword ptr [FindRedirectFunc],eax
    mov eax,[esi+32]
    call InEra
    jc discover_fail
    mov dword ptr [VideoTableAddress],eax
    lea edx,[esi+36]
    mov eax,edx
    call CallTarget
    jc discover_fail
    mov dword ptr [FindVideoFunc],eax
    mov byte ptr [Cat3Convention],1
    mov byte ptr [HasRedirectMode],1
    mov dword ptr [BikExtensionSlot],OFFSET BikExtension
    mov byte ptr [VideoLookupReady],1
    jmp scan_next
scan_done:
    cmp [BikExtension],0
    je discover_fail
    cmp [SmkExtension],0
    je discover_fail
    ; Recovered BIK callers dereference their slot; the SMK fallback does not.
    ; SMK therefore needs the actual Delphi string, not &SmkExtension. Assign
    ; after scanning both handlers, irrespective of their order in the image.
    mov eax,[SmkExtension]
    mov dword ptr [SmkExtensionSlot],eax
    movzx eax,byte ptr [VideoLookupReady]
    jmp discover_return
discover_fail:
    mov byte ptr [VideoLookupReady],0
    xor eax,eax
discover_return:
    pop edi
    pop esi
    pop ebx
    ret
DiscoverEraVideo ENDP

; Old recovered code uses EAX=destination, EDX=PChar. Delphi now also needs ECX.
ConvertPChar PROC
    xor ecx,ecx
    jmp [FromPCharReal]
ConvertPChar ENDP

LoadCodec PROC
    push ebx
    push esi
    push OFFSET CodecName
    call dword ptr [__imp__LoadLibraryA@4]
    test eax,eax
    jz codec_return
    mov ebx,eax
    xor esi,esi
codec_next:
    push dword ptr [CodecNames+esi*4]
    push ebx
    call dword ptr [__imp__GetProcAddress@8]
    test eax,eax
    jz codec_return
    mov [CodecFunctions+esi*4],eax
    inc esi
    cmp esi,14
    jb codec_next
    mov eax,1
codec_return:
    pop esi
    pop ebx
    ret
LoadCodec ENDP

; Compatible with original cdecl malloc calls; zero-fill both cache allocations.
PluginAlloc PROC
    push dword ptr [esp+4]
    push 8 ; HEAP_ZERO_MEMORY
    call dword ptr [__imp__GetProcessHeap@0]
    push eax
    call dword ptr [__imp__HeapAlloc@12]
    ret
PluginAlloc ENDP

ProtectCode PROC
    push OFFSET OldProtection
    push 40h
    push [CodeSize]
    push [CodeStart]
    call dword ptr [__imp__VirtualProtect@16]
    ret
ProtectCode ENDP

RestoreCode PROC
    push OFFSET UnusedProtection
    push [OldProtection]
    push [CodeSize]
    push [CodeStart]
    call dword ptr [__imp__VirtualProtect@16]
    push [CodeSize]
    push [CodeStart]
    call dword ptr [__imp__GetCurrentProcess@0]
    push eax
    call dword ptr [__imp__FlushInstructionCache@12]
    ret
RestoreCode ENDP

SwitchToModern PROC
    pushfd
    pushad
    call ProtectCode
    test eax,eax
    jz modern_done
    call L_004020AA
    call RestoreCode
modern_done:
    popad
    popfd
    ret
SwitchToModern ENDP

SwitchToLegacy PROC
    pushfd
    pushad
    call ProtectCode
    test eax,eax
    jz legacy_done
    call L_00401CD2
    call RestoreCode
legacy_done:
    popad
    popfd
    ret
SwitchToLegacy ENDP
_TEXT ENDS

include recovered.inc
END _DllEntry@12
