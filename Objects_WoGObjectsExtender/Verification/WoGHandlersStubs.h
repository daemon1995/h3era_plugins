#pragma once
#include <WoGObjectHandlers.h>
#include <WoGObjectIds.cpp>
namespace wog { namespace handlers {
inline BOOL Init(H3MapItem *) noexcept { return TRUE; }
inline void SetObjectAsVisited(H3MapItem *, int, H3Hero *, H3Position) noexcept {}
inline BOOL Visit(H3Hero *,H3MapItem *,H3Position,BOOL) noexcept { return TRUE; }
inline BOOL Hint(H3MapItem *,const H3Hero *,int,BOOL) noexcept { return TRUE; }
inline BOOL AiWeight(H3MapItem *,H3Hero *,const H3Player *,int &weight,int *,H3Position) noexcept {weight=100;return TRUE;}
inline void RegisterEvents() noexcept {}
inline BOOL VisitPotion(H3Hero *,int,BOOL){return TRUE;}
inline int PotionAiWeight(H3Hero *,int){return 600;}
} }
