#include "pch.h"
#include "ArtifactImages.h"
#include "../headers/EraPluginsAPI/GraphicsAPI.h"
#include <algorithm>

namespace artifacts
{
bool ArtifactImages::Ready()
{
    const auto *api = era_graphics::GetApi();
    if (!api && !reportedUnavailable)
    {
        Log("Graphics API v1 unavailable; update ERA_RuntimeServices.era. Artifact images were not replaced.");
        reportedUnavailable = true;
    }
    return api && api->isReady();
}
void ArtifactImages::Apply(H3LoadedDef *target, int count, const std::array<ArtifactData, CAPACITY> &definitions)
{
    using namespace era_graphics;
    const auto *api = GetApi();
    if (!Ready() || !api || !target || !target->groups || target->groupsCount <= 0 || !target->groups[0]) return;
    const auto &group = *target->groups[0];
    if (group.count < 0 || (group.count && !group.frames)) return;
    FrameRequest request;
    request.targetDef = target;
    FrameHandle placeholder = 0;
    if (api->createFrame(&request, &placeholder) != Ok) return;
    std::vector<FrameBinding> frames((std::max)(group.count, count));
    for (int i = 0; i < int(frames.size()); ++i)
    {
        if (i < group.count) frames[i].sourceFrame = i;
        else frames[i].frame = placeholder;
    }
    for (int id = 0; id < count; ++id)
    {
        const auto &definition = definitions[id];
        if (definition.image.empty()) continue;
        request.resource = definition.image.c_str();
        request.sourceFrame = definition.imageFrame;
        FrameHandle frame = 0;
        const bool recursive = !_stricmp(request.resource, "artifact.def") || !_stricmp(request.resource, "artifacts.def");
        const int result = recursive ? RecursiveSource : api->createFrame(&request, &frame);
        if (result != Ok) Log("Cannot load artifact image: %s (frame %d, Graphics API result %d)",
                              request.resource, request.sourceFrame, result);
        frames[id].frame = result == Ok ? frame : placeholder;
    }
    UpdateRequest update;
    update.targetDef = target;
    update.bindings = frames.data();
    update.count = static_cast<unsigned>(frames.size());
    const int result = api->applyFrames(&update);
    if (result != Ok) Log("Cannot update artifact DEF frames (Graphics API result %d)", result);
}
} // namespace artifacts
