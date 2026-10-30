#pragma once
struct eObject { enum {FOUNTAIN_OF_FORTUNE=30}; };
struct H3TextFile
{
    struct Lines : std::vector<const char *>
    {
        size_t Size() const {return size();}
        void Add(const char *line){push_back(line);}
    } text;
    const char *operator[](size_t index) const {return text[index];}
};
static int IntAt(size_t){return 3;}
