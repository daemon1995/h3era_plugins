#include "pch.h"

std::vector<DlgStyle> DlgStyle::styles;

DlgStyle::DlgStyle(const UINT width, const UINT height, const UINT16 maxRows, LPCSTR pcxName, const LPCSTR f,
                   const BOOL createExport)
    : width(width), height(height), maxRows(maxRows), fontName(f), pcxName(pcxName), createExportButton(createExport)
{
}

bool DlgStyle::CreateAssets(bool forceRecreate)
{
    if (forceRecreate)
        styles.clear();

    if (styles.empty())
    {

        styles.emplace_back(DlgStyle(200, 23, 7, NH3Dlg::Assets::DIBOXBACK, h3::NH3Dlg::Text::MEDIUM, false ));
        styles.emplace_back(DlgStyle(123, 40, 25, NH3Dlg::HDassets::DLGBLUEBACK, h3::NH3Dlg::Text::BIG, true));
        text.Load();
    }
    return !styles.empty();
}

DlgStyle::~DlgStyle()
{
}

DlgStyle::StyleText DlgStyle::text;

void DlgStyle::StyleText::Load()
{
    displayNameFormat = EraJS::read("era.locale.dlg.buttonName");
}
