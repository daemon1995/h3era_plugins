#pragma once
#include "pch.h"
#include <array>

namespace graphics
{

class GraphicsEnhancements : public IGamePatch
{
  public:
    static constexpr UINT MAX_UNIQUE_HEROES = 1024;
    static constexpr UINT MAX_UNIQUE_CLASSES = 128;

  private:
    static GraphicsEnhancements *instance;

    std::array<H3LoadedDef *, MAX_UNIQUE_HEROES> uniqueHeroDefs;
    std::array<H3LoadedDef *, MAX_UNIQUE_CLASSES> heroClassDefs[2]; // 0 - male, 1 - female

  protected:
    GraphicsEnhancements();
    virtual void CreatePatches() noexcept override;

  public:
    static GraphicsEnhancements *Get() noexcept;
    static H3LoadedDef *Hero_GetMapItemDef(const H3Hero *hero) noexcept;
    H3LoadedDef *InitHeroData(const H3Hero *hero) noexcept;
    void InitHeroClassData(const UINT classId) noexcept;
    void CleanUpData() noexcept;
};

} // namespace graphics
