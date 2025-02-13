#pragma once
class Locale
{
  public:
    std::string name;
    std::string displayedName;
    BOOL hasDescription = false;

  public:
    Locale(const char *name, const char *displayedName);
    ~Locale();

  public:
    const bool operator==(const Locale &other) const;
    const bool operator!=(const Locale &other) const;
};
