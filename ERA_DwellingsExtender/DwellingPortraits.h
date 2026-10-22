#pragma once
#include "DwellingPortraitLayout.h"
#include "../headers/EraPluginsAPI/GraphicsAPI.h"
#include <set>

namespace dwellings
{
class DwellingPortraits
{
    // Pin the resource selected by the installed native/WoG constructor.
    // Frames and pointer tables remain valid across overview/map reloads.
    std::vector<h3::H3LoadedDef *> resources;
    std::string resourceName;
    std::set<h3::H3LoadedDef *> applied;
    std::vector<std::pair<int, int>> specialSources;
    bool ready = false;
    bool reportedUnavailable = false;

    era_graphics::FrameHandle Image(h3::H3LoadedDef *target, const PortraitSource &source);

  public:
    DwellingPortraitLayout layout;
    static bool IsResource(const char *name) noexcept;
    bool UsesResource(const char *name) const noexcept;
    int UseResource(h3::H3LoadedDef *resource, int minimumFrames);
    void SetSpecialSource(int destination, int source);
    bool Apply(h3::H3LoadedDef *target);
    bool Finish();
    bool GraphicsReady();
    bool Ready() const noexcept { return ready; }
};
} // namespace dwellings
