#pragma once
#include "Artificer.h"

namespace wog { namespace artificer {
// Runs one native market window; the host remains the single authority for quotes and mutations.
void ShowDialog(Host &host, H3Hero *hero, const H3MapItem *object);
} }
