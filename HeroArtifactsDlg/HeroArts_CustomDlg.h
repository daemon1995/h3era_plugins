#pragma once
#include "..\..\headers\header.h"
#include "map"
using namespace h3;
constexpr int MAX_BP_ARTIFACTS = 64;


class HeroArts_CustomDlg :public H3Dlg
{
    H3Hero* hero;
    int slot = eArtifactSlots::NONE;
    int selectedArt = eArtifact::NONE;

public:
    HeroArts_CustomDlg(int a, int b, int x, int y, H3Hero* heroA, int slot) :H3Dlg(a, b, x, y), hero(heroA), slot(slot) {} // delegate size to H3Dlg, clear map
    virtual  ~HeroArts_CustomDlg();
    BOOL DialogProc(H3Msg& msg) override;

    int GetSelectedArtd() { return selectedArt; }

    std::map<int, H3Artifact> displayedArts = {};
private:
    void ShowArtifactDescription(const H3Artifact* art);
    void SwitchHeroArtifact(H3Artifact* art, int itId);

};