#pragma once

// Include after header.h. Game-thread-only coordination between menu hooks.
// The patcher variable borrows a stack pointer only for this message's scope.
namespace mainmenu
{
constexpr const char *CURSOR_REQUEST_KEY = "ERA.MainMenu.CursorRequest.v1";

struct CursorRequest
{
    h3::H3Msg *message;
    bool hand = false;
    CursorRequest *previous;
    CursorRequest *active;

    explicit CursorRequest(h3::H3Msg *msg) : message(msg)
    {
        auto &slot = GetPatcher()->VarValue<CursorRequest *>(CURSOR_REQUEST_KEY);
        previous = slot;
        active = previous && previous->message == msg ? previous : this;
        if (active == this)
            slot = this;
    }
    ~CursorRequest()
    {
        if (active == this)
            GetPatcher()->VarValue<CursorRequest *>(CURSOR_REQUEST_KEY) = previous;
    }
    void RequestHand(bool value) { active->hand |= value; }
    bool WantsHand() const { return active->hand; }
    bool IsOwner() const { return active == this; }
    CursorRequest(const CursorRequest &) = delete;
    CursorRequest &operator=(const CursorRequest &) = delete;
};

inline void ApplyHandCursor(bool hand)
{
    auto *mouse = h3::H3MouseManager::Get();
    if (!mouse)
        return;
    auto &owned = GetPatcher()->VarValue<int>("ERA.MainMenu.CursorOwned.v1");
    const bool isHand = mouse->GetType() == 0 && mouse->GetFrame() == 3;
    if (hand)
    {
        if (!isHand)
            mouse->SetCursor(3, 0);
        owned = true;
    }
    else
    {
        if (owned && isHand)
            mouse->DefaultCursor();
        owned = false;
    }
}
} // namespace mainmenu
