#pragma once
class Locale
{
  public:
    std::string name;
    std::string displayedName;
    BOOL hasDescription = false;
    BOOL broken = false; // if locale is broken, it will be displayed with `??`

  public:
    Locale(const char *name, const char *displayedName);
    ~Locale();

  public:
    const bool operator==(const Locale &other) const;
    const bool operator!=(const Locale &other) const;
};
