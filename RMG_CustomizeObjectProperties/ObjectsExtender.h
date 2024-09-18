#pragma once
#include <set>


namespace extender
{
	// temp struct to allow add line into protected field "H3Vector<LPCSTR> text;" at 0x1C
	struct EditableH3TextFile : public H3TextFile
	{
		void AddLine(LPCSTR txt);
	};
	struct RMGSetObject
	{
		int type;
		int subtype;
		bool operator<(const RMGSetObject& other) const;
	};

	class ObjectsExtender : public IGamePatch
	{

		static std::set<ObjectsExtender*> extenders;

	protected:

		static std::vector<std::string> additionalProperties;
		static std::vector<RMGObjectInfo> additionalRmgObjects;

		ObjectsExtender(PatcherInstance* pi);

		virtual ~ObjectsExtender();

		//virtual void GetObjectPreperties() noexcept = 0;

		virtual void CreatePatches() override;
		virtual void AferLoadingObjectTxtProc(const INT16* maxSubtypes) = 0;

	private:
		// static methods to use them as General Objects Extending hooks
		static void __stdcall H3GameMainSetup__LoadObjects(HiHook* h, const  H3MainSetup* setup);
		static _LHF_(LoadObjectsTxt);
		static _LHF_(H3AdventureManager__ObjectVisit_SoundPlay);
	public:
		static void AddObjectsToObjectGenList(H3Vector<H3RmgObjectGenerator*>* rmgObjecsList);

		//static void __stdcall H3AdventureManager__ObjectVisit_SoundPlay(HiHook* h, const int objType, const int objSetup);
	};
}
