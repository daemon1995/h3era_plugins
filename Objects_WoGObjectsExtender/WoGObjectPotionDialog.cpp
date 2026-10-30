#include "WoGObjectPotionDialog.h"
#include "WoGObjectUtils.h"
namespace wog { namespace handlers { namespace {
class PotionDialog final : public H3Dlg
{
    std::array<bool, 3> selected{};
    H3DlgDef *checks[3]{};
    bool accepted = false;
  public:
    PotionDialog(H3Hero *hero, const std::array<int, 3> &stock) : H3Dlg(540, 370, -1, -1, FALSE, FALSE)
    {
        util::AddDialogBackground(*this, 540, 370);
        CreateText(24, 16, 492, 32, util::Tr("potions.choose", {"hero", hero->name}).c_str(),
                   NH3Dlg::Text::BIG, eTextColor::GOLD, -1, eTextAlignment::MIDDLE_CENTER);
        const char *effects[] = {"effectHydromel", "effectBerserker", "effectVenus"};
        for (int row = 0; row < 3; ++row)
        {
            const int doses = stock[row], y = 54 + row * 80;
            const auto message = util::Tr(std::string("potions.") + effects[row], {"doses", std::to_string(doses)});
            CreateText(66, y, 444, 68, message.c_str(), NH3Dlg::Text::MEDIUM,
                       doses > 0 ? eTextColor::REGULAR : eTextColor::SILVER, 10 + row, eTextAlignment::MIDDLE_LEFT);
            if (doses > 0)
            {
                checks[row] = H3DlgDef::Create(28, y + 24, 10 + row, "RMG_chbx.def", 0, 1);
                if (checks[row]) AddItem(checks[row]);
            }
        }
        CreateText(24, 282, 492, 20, util::Tr("potions.select").c_str(), NH3Dlg::Text::SMALL,
                   eTextColor::GOLD, -1, eTextAlignment::MIDDLE_CENTER);
        CreateOKButton(188, 310, 1, FALSE);
        CreateCancelButton(286, 310, 2, FALSE);
    }
    BOOL DialogProc(H3Msg &msg) override
    {
        if (!msg.IsLeftDown()) return TRUE;
        const int id = msg.itemId;
        if (id >= 10 && id < 13 && checks[id - 10])
        {
            const int row = id - 10;
            selected[row] = !selected[row];
            checks[row]->SetFrame(selected[row]);
            Redraw();
            return FALSE;
        }
        return TRUE;
    }
    BOOL OnLeftClick(INT id, H3Msg &) override
    {
        if (id == 1 || id == 2)
        { accepted = id == 1; Stop(); return FALSE; }
        return TRUE;
    }
    BOOL OnKeyPress(eVKey key, eMsgFlag) override
    {
        if (key == eVKey::H3VK_ENTER || key == eVKey::H3VK_ESCAPE)
        { accepted = key == eVKey::H3VK_ENTER; Stop(); return FALSE; }
        return TRUE;
    }
    std::array<bool, 3> Run()
    {
        const int previous = P_WindowManager->resultItemID;
        Start(); P_WindowManager->resultItemID = previous;
        return accepted ? selected : std::array<bool, 3>{};
    }
};
} // namespace
std::array<bool, 3> ChoosePotions(H3Hero *hero, const std::array<int, 3> &doses)
{
    return PotionDialog(hero, doses).Run();
}
} }
