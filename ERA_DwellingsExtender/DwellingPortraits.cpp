#include "pch.h"
#include "DwellingPortraits.h"
#include <algorithm>

namespace dwellings
{
namespace
{
void Log(const char *text)
{
    if (Era::WriteLog) Era::WriteLog("DwellingsExtender", "Kingdom portraits", text);
}
}
bool DwellingPortraits::IsResource(const char *name) noexcept
{
    return name && (!_stricmp(name, "zlagport.def") || !_stricmp(name, "flagport.def"));
}
bool DwellingPortraits::GraphicsReady()
{
    const auto *api = era_graphics::GetApi();
    if (!api && !reportedUnavailable)
    {
        Log("Graphics API v1 unavailable; update API_ErmFunctions.era. Using native portraits.");
        reportedUnavailable = true;
    }
    return api && api->isReady();
}
int DwellingPortraits::UseResource(H3LoadedDef *def, int minimumFrames)
{
    if (!def || !def->groups || def->groupsCount <= 0 || !def->groups[0] ||
        def->groups[0]->count <= 0 || !def->groups[0]->frames) return minimumFrames;
    resourceName = def->GetName();
    // Retain the atlas selected by the native constructor, including before Apply.
    if (std::find(resources.begin(), resources.end(), def) == resources.end())
    {
        def->IncreaseReferences();
        resources.push_back(def);
    }
    return (std::max)(minimumFrames, def->groups[0]->count);
}
bool DwellingPortraits::UsesResource(const char *name) const noexcept
{
    return name && !resourceName.empty() && !_stricmp(name, resourceName.c_str());
}
void DwellingPortraits::SetSpecialSource(int destination, int source)
{
    if (destination != source) specialSources.emplace_back(destination, source);
}
era_graphics::FrameHandle DwellingPortraits::Image(H3LoadedDef *target, const PortraitSource &source)
{
    const auto *api = era_graphics::GetApi();
    if (!api || !api->isReady()) return 0;
    era_graphics::FrameRequest request;
    request.targetDef = target;
    request.resource = source.image.c_str();
    request.sourceFrame = source.imageFrame;
    era_graphics::FrameHandle frame = 0;
    const int result = IsResource(request.resource) ? era_graphics::RecursiveSource : api->createFrame(&request, &frame);
    if (result != era_graphics::Ok)
        Log(H3String::Format("Cannot load %s (frame %d, graphics result %d); using placeholder.",
            request.resource, request.sourceFrame, result).String());
    return frame;
}
bool DwellingPortraits::Apply(H3LoadedDef *target)
{
    if (applied.count(target)) return true;
    if (!GraphicsReady() || !target || !target->groups || target->groupsCount <= 0 || !target->groups[0]) return false;
    const auto &group = *target->groups[0];
    const int originalCount = group.count;
    if (originalCount <= 0 || originalCount > layout.OriginalCount() || !group.frames) return false;
    const auto placeholder = Image(target, PortraitSource{});
    if (!placeholder) return false;
    std::vector<era_graphics::FrameBinding> frames(layout.Count());
    for (int i = 0; i < layout.Count(); ++i)
    {
        if (i < originalCount) frames[i].sourceFrame = i;
        else frames[i].frame = placeholder;
    }
    const auto borrow = [&](int index) {
        era_graphics::FrameBinding binding;
        if (index >= 0 && index < originalCount && group.frames[index]) binding.sourceFrame = index;
        else binding.frame = placeholder;
        return binding;
    };
    for (const auto &alias : specialSources)
        if (alias.first >= 0 && alias.first < layout.OriginalCount()) frames[alias.first] = borrow(alias.second);
    int index = layout.OriginalCount();
    for (const auto &source : layout.Appended())
    {
        if (source.image.empty()) frames[index] = borrow(source.frame);
        else
        {
            const auto image = Image(target, source);
            frames[index].frame = image ? image : placeholder;
        }
        ++index;
    }
    era_graphics::UpdateRequest request;
    request.targetDef = target;
    request.bindings = frames.data();
    request.count = static_cast<std::uint32_t>(frames.size());
    const int result = era_graphics::GetApi()->applyFrames(&request);
    if (result != era_graphics::Ok)
    {
        Log(H3String::Format("Cannot extend portrait table (graphics result %d).", result).String());
        return false;
    }
    applied.insert(target);
    Log(H3String::Format("%s: preserved %d source frames; %d total runtime frames.",
                        target->GetName(), originalCount, target->groups[0]->count).String());
    return true;
}
bool DwellingPortraits::Finish()
{
    ready = !resources.empty();
    for (auto *resource : resources) if (!Apply(resource)) ready = false;
    if (!ready) Log("Portrait extension unavailable; using native frame 0.");
    return ready;
}
} // namespace dwellings
