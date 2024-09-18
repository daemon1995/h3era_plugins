#pragma once
#include "ObjectsExtender.h"
namespace shrine
{

	class ShrineManager :
		public extender::ObjectsExtender
	{

		ShrineManager();

		virtual ~ShrineManager();

	private:
		virtual void CreatePatches()  override;
		virtual void AferLoadingObjectTxtProc(const INT16* maxSubtypes) override final;
	//	virtual void GetObjectPreperties() noexcept override final;
	private:

		void SetRmgObjectGenData(const int objectSubtype)  noexcept;

	private:

		static _LHF_(Game__AtShrineOfMagicIncantationSettingSpell);
		static _LHF_(Shrine__AtGetName);


	public:
		static ShrineManager& Get();

	};


}
