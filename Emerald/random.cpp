
#include "emerald.h"


void __stdcall ReallocProhibitionTables(PEvent e)
{

	int base = *(int*)0x699538;

	//memcpy((void*)(save.used_artifacts),	 (void*)(base+0x4E224), 144);
	//memcpy((void*)(save.allowed_artifacts), (void*)(base+0x4E2B4), 144);

	int used_artifacts_references[] = 
	{0x4BCC98, 0x4BCCC8, 0x4BE1BB, 0x4C0776,
	 0x4C07E9, 0x4C91B5, 0x4C91ED, 0x4C9237,
	 0x4C9252, 0x4CE084};

	for (int i=0; i!=sizeof(used_artifacts_references)/4; i++)
	{
		emerald->WriteDword(used_artifacts_references[i], (int)(save.used_artifacts) - base);

		//*(int*)used_artifacts_references[i]+=(int)(save.used_artifacts) - base - 0x4E224;
	}


	
	int allowed_artifacts_references[] = 
	{0x4BCC8C, 0x4BCCBC, 0x4BE1A8, 0x4C0767,
	 0x4C21A2, 0x4C21EB, 0x4C2299, 0x4C24B1,
	 0x4C24F7, 0x4C4BB7, 0x4C91E4, 0x4CE093,
	 0x5701C4, 0x574AB2, 0x574FD4, 0x714EF5,
	 0x714F6F, 0x74E7BF};

	int allowed_artifacts_end_references[] = 
	{0x4C076D, 0x4C229F, 0x4C23DC};

	


	emerald->WriteDword(0x4C21B3, NEW_ARTS_AMOUNT*0x20);
	emerald->WriteDword(0x4C2306, NEW_ARTS_AMOUNT*0x20);

	
	emerald->WriteDword(0x4C21E0, NEW_ARTS_AMOUNT);
	emerald->WriteDword(0x4C2279, NEW_ARTS_AMOUNT);

	emerald->WriteDword(0x44D1AA, NEW_ARTS_AMOUNT);

	
	

	for (int i=0; i!=sizeof(allowed_artifacts_references)/4; i++)
	{
		emerald->WriteDword(allowed_artifacts_references[i], (int)(save.allowed_artifacts) - base);
	}

	for (int i=0; i!=sizeof(allowed_artifacts_end_references)/4; i++)
	{
		emerald->WriteDword(allowed_artifacts_end_references[i], (int)(save.allowed_artifacts) - base + NEW_ARTS_AMOUNT);
	}

	const int max_art = NEW_ARTS_AMOUNT;

	//emerald->WriteDword(0x4C076D, max_art + (int)(save.allowed_artifacts) - base);
	//emerald->WriteDword(0x4C229F, max_art + (int)(save.allowed_artifacts) - base);
	//emerald->WriteDword(0x4C23DC, max_art + (int)(save.allowed_artifacts) - base);
	
	emerald->WriteDword(0x4C9248, max_art);
	emerald->WriteDword(0x4C91C2, max_art);

	/**(int*)(0x4C076B+2) += 188 + (int)(save.allowed_artifacts) - base - 0x4E2B4;
	*(int*)(0x4C229D+2) += 188 + (int)(save.allowed_artifacts) - base - 0x4E2B4;
	*(int*)(0x4C23DB+1) += 188 + (int)(save.allowed_artifacts) - base - 0x4E2B4;*/


	
        //*(int*)0x4C9248 = (int)188;
        //*(int*)0x4C91C2 = (int)188;
}