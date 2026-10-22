#pragma once
#include <string>
#include <vector>
#include <array>

namespace dwellings
{
struct PortraitObject
{
    int type = -1;
    int subtype = -1;
};

struct PortraitSource
{
    int frame = 0;
    std::string image;
    int imageFrame = 0;
};

inline int VanillaSpecialHint(int frame, const std::array<int, 5> &nativeFrames) noexcept
{
    const int vanilla[] = {82, 83, 84, 85, 87};
    for (size_t i = 0; i < nativeFrames.size(); ++i)
        if (frame == nativeFrames[i]) return vanilla[i];
    return 86; // The native switch intentionally has no hint for the unused frame.
}

// The original DEF, including unused/special frames, is an immutable prefix.
// Equal pictures may share frame pointers, but different objects never share IDs.
class DwellingPortraitLayout
{
    int originalCount = 0;
    std::vector<PortraitObject> objects;
    std::vector<PortraitSource> appended;

  public:
    void Init(int count)
    {
        originalCount = count;
        objects.assign(count, PortraitObject{});
        appended.clear();
    }
    int Add(int type, int subtype, int nativeFrame, const PortraitSource &source)
    {
        int frame = nativeFrame;
        if (frame < 0 || frame >= originalCount || source.frame != frame || !source.image.empty())
        {
            frame = static_cast<int>(objects.size());
            objects.push_back(PortraitObject{});
            appended.push_back(source);
        }
        objects[frame].type = type;
        objects[frame].subtype = subtype;
        return frame;
    }
    const PortraitObject *Find(int frame) const noexcept
    {
        if (frame < 0 || static_cast<size_t>(frame) >= objects.size() || objects[frame].type < 0)
            return nullptr;
        return &objects[frame];
    }
    int OriginalCount() const noexcept { return originalCount; }
    int Count() const noexcept { return static_cast<int>(objects.size()); }
    const std::vector<PortraitSource> &Appended() const noexcept { return appended; }
};

} // namespace dwellings
