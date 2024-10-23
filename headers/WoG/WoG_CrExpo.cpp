

#define CE_HERO  1
#define CE_MAP   2
#define CE_TOWN  3
#define CE_MINE  4
#define CE_GARR  5

#define CRLOC_HERO(hero_id, slot_ix)	(_ptr_)((hero_id)+(slot_ix)*0x10000)
#define CRLOC_MAP(x,y,z) 				(_ptr_)((x)+(y)*0x100+(z)*0x10000)
#define CRLOC_TOWN(x,y,z, slot_ix) 		(_ptr_)((x)+(y)*0x100+(z)*0x10000+(slot_ix)*0x20000)
#define CRLOC_GARRISON(x,y,z, slot_ix)	(_ptr_)((x)+(y)*0x100+(z)*0x10000+(slot_ix)*0x20000)



#define o_CrExpoSet_FindType(army, slot_ix, p_type, p_crloc) CALL_4(void, __cdecl, 0x71A1B7, army, slot_ix, p_type, p_crloc)
//#define o_CrExpoSet_FindType2(x,y,z, slot_ix, p_type, p_crloc, p_cr_type, p_cr_count) CALL_8(void, __cdecl, 0x719DE3, x,y,z, slot_ix, p_type, p_crloc, p_cr_type, p_cr_count)
#define o_CrExpoSet_HMove(type_s,type_d,crloc_s, crloc_d, cr_type_s, cr_type_d, cr_count_s, cr_count_d) CALL_8(void, __cdecl, 0x7198C1, type_s,type_d,crloc_s, crloc_d, cr_type_s, cr_type_d, cr_count_s, cr_count_d)
#define o_CrExpoSet_HComb(type_s,type_d,crloc_s, crloc_d, cr_type_s, cr_type_d, cr_count_s, cr_count_d) CALL_8(void, __cdecl, 0x719A95, type_s,type_d,crloc_s, crloc_d, cr_type_s, cr_type_d, cr_count_s, cr_count_d)
#define o_CrExpoSet_HCombReal(type_s,type_d,crloc_s, crloc_d, cr_type_s, cr_type_d, cr_count_s, cr_count_d) CALL_8(void, __cdecl, 0x719CBC, type_s,type_d,crloc_s, crloc_d, cr_type_s, cr_type_d, cr_count_s, cr_count_d)
#define o_CrExpoSet_GetExp(type, crloc) CALL_2(int, __cdecl, 0x718CCD, type, crloc)

#define o_CrExpoSet_Del(type, crloc) CALL_2(int, __cdecl, 0x71866A, type, crloc)

// эта ф-ия установки опыта обнуляет арты.
#define o_CrExpoSet_SetN(type, crloc, cr_type, cr_count, exp) CALL_5(void, __cdecl, 0x718AD0, type, crloc, cr_type, cr_count, exp)


// функция устанавливает опыт не обнуляя арты
inline void n_CrExpoSet_Set(int type, _ptr_ crloc, int cr_type, int cr_count, int exp)
{
	int crexpo_i = CALL_2(int, __cdecl, 0x718579, type, crloc); // CrExpoSet::FindIt
	if (crexpo_i != -1)	{
		_ptr_ crexpo = DwordAt(0x718B32 + 2) + 16 * crexpo_i;
		IntAt(crexpo + 0) = exp;
		IntAt(crexpo + 4) = cr_count;
		DwordAt(crexpo + 8) = ((((_dword_)(_byte_)cr_type) << 5) & 0x1FE0) | (DwordAt(crexpo + 8) & 0xFFFFE01F);
		CALL_1(void, __thiscall, 0x7176A1, 0x860550 + 16 * crexpo_i); // CrExpo::Check4Max
	} else o_CrExpoSet_SetN(type, crloc, cr_type, cr_count, exp);
}


inline void CrExpoSet_Exchange(_Army_* army_s, int i_s, _Army_* army_d, int i_d)
{
	int type_s, type_d;
	_ptr_ crloc_s, crloc_d;
	o_CrExpoSet_FindType(army_s, i_s, &type_s, &crloc_s);
	o_CrExpoSet_FindType(army_d, i_d, &type_d, &crloc_d);
	o_CrExpoSet_HMove(type_s, type_d, crloc_s, crloc_d, army_s->type[i_s], army_d->type[i_d], army_s->count[i_s], army_d->count[i_d]);
}

inline void CrExpoSet_Combine(_Army_* army_s, int i_s, _Army_* army_d, int i_d)
{
	int type_s, type_d;
	_ptr_ crloc_s, crloc_d;
	o_CrExpoSet_FindType(army_s, i_s, &type_s, &crloc_s);
	o_CrExpoSet_FindType(army_d, i_d, &type_d, &crloc_d);
	o_CrExpoSet_HCombReal(type_s, type_d, crloc_s, crloc_d, army_s->type[i_s], army_d->type[i_d], army_s->count[i_s], army_d->count[i_d]);
}



inline bool CrExpoSet_SplitAdd(_Army_* army_s, int i_s, _Army_* army_d, int i_d, int count, bool pass_arts = false)
{
	if ((army_d->type[i_d] != -1) && (army_d->type[i_d] != army_s->type[i_s])) return false;
	if (army_s->count[i_s] <= count) return false;

	int type_s, type_d;
	_ptr_ crloc_s, crloc_d;
	o_CrExpoSet_FindType(army_s, i_s, &type_s, &crloc_s);
	o_CrExpoSet_FindType(army_d, i_d, &type_d, &crloc_d);
	int exp_s = o_CrExpoSet_GetExp(type_s, crloc_s);
	int exp_d = army_d->type[i_d] < 0 ? 0 : o_CrExpoSet_GetExp(type_d, crloc_d);
	int count_d = army_d->type[i_d] < 0 ? 0 : army_d->count[i_d];
	int exp = (exp_d * army_d->count[i_d] + exp_s * count ) / (army_d->count[i_d] + count);

	if (pass_arts) {
		n_CrExpoSet_Set(type_s, crloc_s, army_s->type[i_s], 1, exp_s);
		n_CrExpoSet_Set(type_d, crloc_d, army_s->type[i_s], 1, exp_d);
		o_CrExpoSet_HComb(type_s, type_d, crloc_s, crloc_d, army_s->type[i_s], army_s->type[i_s], 1, 1);
	}

	n_CrExpoSet_Set(type_s, crloc_s, army_s->type[i_s], army_s->count[i_s] - count, exp_s);
	n_CrExpoSet_Set(type_d, crloc_d, army_s->type[i_s], count_d + count, exp);

	return true;
}


inline void CrExpoSet_Remove(_Army_* army, int i)
{
	int type_s;
	_ptr_ crloc_s;
	o_CrExpoSet_FindType(army, i, &type_s, &crloc_s);
	o_CrExpoSet_Del(type_s, crloc_s);
}

inline int CrExpoSet_GetExp(_Army_* army, int i)
{
	int type_s;
	_ptr_ crloc_s;
	o_CrExpoSet_FindType(army, i, &type_s, &crloc_s);
	return army->type[i] < 0 ? 0 : o_CrExpoSet_GetExp(type_s, crloc_s);
}

inline void CrExpoSet_SetExp(_Army_* army, int i, int exp)
{
	int type_s;
	_ptr_ crloc_s;
	o_CrExpoSet_FindType(army, i, &type_s, &crloc_s);
	n_CrExpoSet_Set(type_s, crloc_s, army->type[i], army->count[i], exp);
}