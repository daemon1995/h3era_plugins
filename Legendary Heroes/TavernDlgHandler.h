#pragma once
class TavernDlgHandler : public IGamePatch
{
	static _LHF_(BeforeCreate);
	static _LHF_(BeforeGoldTextCreate);
	static _LHF_(AfterCreate);
	H3LoadedPcx16* m_drawBuffer[2];

	TavernDlgHandler( PatcherInstance *pi);
public:
	static TavernDlgHandler* instance;

	H3DlgPcx16* CreatePortraitPcx(H3DlgPcx* sourceItem, int heroId, H3LoadedPcx16* buffer);
	//void Init() override;
	static void Init(PatcherInstance* _pi, H3LoadedPcx16* drawBuffer[2]) noexcept;
	void  CreatePatches() noexcept override ;

};

