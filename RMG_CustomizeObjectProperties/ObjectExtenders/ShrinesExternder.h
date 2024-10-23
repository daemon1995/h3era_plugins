#pragma once
namespace shrines
{

	class ShrinesExternder :
		public extender::ObjectsExtender
	{

		ShrinesExternder();

		virtual ~ShrinesExternder();

	private:
		virtual void CreatePatches()  override;
	//	virtual void AfterLoadingObjectTxtProc(const INT16* maxSubtypes) override final;
	//	virtual void GetObjectPreperties() noexcept override final;
	private:

	//	void SetRmgObjectGenData(const int objectSubtype)  noexcept;

	private:

		static _LHF_(Game__AtShrineOfMagicIncantationSettingSpell);
		static _LHF_(Shrine__AtGetName);


	public:
		static ShrinesExternder& Get();

	};


}
