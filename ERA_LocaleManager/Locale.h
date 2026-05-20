#pragma once
class Locale
{
  public:
    std::string name;
    std::string displayedName;
    DWORD codePage = 1252; // default to Latin 1
    BOOL hasDescription = false;
    BOOL broken = false; // if locale is broken, it will be displayed with `??`

  public:
    Locale(const char *name, const char *_displayedName, DWORD codePage)
        : name(name), displayedName(_displayedName), codePage(codePage)
    {
        // displayedName += std::string(" (") + name + ")";
        broken = displayedName.find('?') != std::string::npos;
    }
    ~Locale() {};

  public:
    const bool operator==(const Locale &other) const
    {
        return !(*this != other);
    };
    const bool operator!=(const Locale &other) const
    {
        return strcmp(name.c_str(), other.name.c_str()) || strcmp(displayedName.c_str(), other.displayedName.c_str());
    }
};

struct DlgStyle
{
    enum STYLE_ID
    {
        BROWN_BACK = 0,
        BLUE_BACK
    };
    const UINT width;
    const UINT height;
    UINT maxRows;
    const BOOL createExportButton = true;
    const BOOL isBlueBack = false;
    LPCSTR pcxName = nullptr;
    H3String fontName;

  public:
    DlgStyle(const UINT width, const UINT height, const UINT maxRows, const BOOL createExportButton,
             const BOOL isBlueBack, LPCSTR fontName)
        : width(width), height(height), maxRows(maxRows), createExportButton(createExportButton),
          isBlueBack(isBlueBack), fontName(fontName)
    {
        this->maxRows = Clamp(0, maxRows + createExportButton, (600 / height));
        pcxName = isBlueBack ? NH3Dlg::HDassets::DLGBLUEBACK : NH3Dlg::Assets::DIBOXBACK;
    }
};
