#include <WoGObjectPotionDialog.cpp>
namespace wog { namespace util {
std::string Tr(const std::string &key, const std::vector<std::string> &parameters)
{ return Era::tr(key.c_str(), parameters); }
bool AddDialogBackground(H3Dlg &, int, int, bool) { return true; }
} }
using wog::handlers::PotionDialog;
static void Click(H3Dlg &dialog, int id) { H3Msg message; dialog.OnLeftClick(id, message); }
int main()
{
    H3Hero hero; unsigned checks = 0;
    for (int available = 0; available < 8; ++available)
        for (int chosen = 0; chosen < 8; ++chosen)
        {
            const std::array<int,3> stock{{available & 1 ? 3 : 0, available & 2 ? 2 : 0, available & 4 ? 1 : 0}};
            PotionDialog dialog(&hero, stock);
            for (int row = 0; row < 3; ++row) if (chosen & (1 << row)) Click(dialog, 10 + row);
            Click(dialog, 1);
            const auto result = dialog.Run();
            for (int row = 0; row < 3; ++row)
            { assert(result[row] == ((available & chosen & (1 << row)) != 0)); ++checks; }
            assert(window.resultItemID == 123); ++checks;
        }
    for (bool escape : {false, true})
    {
        PotionDialog dialog(&hero, {{3,3,3}});
        Click(dialog, 10); Click(dialog, 11); Click(dialog, 12);
        if (escape) dialog.OnKeyPress(eVKey::H3VK_ESCAPE, eMsgFlag::NONE); else Click(dialog, 2);
        assert((dialog.Run() == std::array<bool,3>{})); ++checks;
    }
    PotionDialog dialog(&hero, {{3,3,3}});
    Click(dialog, 10); Click(dialog, 10); Click(dialog, 12);
    dialog.OnKeyPress(eVKey::H3VK_ENTER, eMsgFlag::NONE);
    assert((dialog.Run() == std::array<bool,3>{{false,false,true}})); ++checks;
    std::printf("PASS: %u native potion dialog availability, selection, toggle, confirmation and cancellation checks\n", checks);
}
