#pragma once
#include "pch.h"

class MenuItem
{
    // protected:
    static int item_id;
    int id = -1;

  public:
    UINT8 index = -1;
    // H3DlgItem* it = nullptr;
    MenuItem *parent = nullptr;
    H3String key_ptr;

    LPCSTR name = nullptr;
    LPCSTR hint = nullptr;

    LPCSTR popup = nullptr;
    bool enabled = false;
    UINT8 state = 0;
    MenuItem();

    MenuItem(LPCSTR name, MenuItem *parent = nullptr, LPCSTR hint = h3_NullString, LPCSTR popup = h3_NullString);

    void SendCommand(h3::H3DlgItem &it, h3::H3Msg &msg);
    MenuItem *GetParent() const noexcept;
    void SetEnabled(bool state) noexcept;
    // virtual MenuItem& GetParent() =0;
    int GetId() const noexcept
    {
        return id;
    }
    virtual void GetKey();
    virtual void SetupText();

    LPCSTR GetName();
    LPCSTR GetHint();
    LPCSTR GetPopup();

    virtual ~MenuItem();

    //	virtual MenuItem& GetParent() = 0;
    //	virtual MenuItem& GetChild() = 0;
};
//} /*end namespace em*/
