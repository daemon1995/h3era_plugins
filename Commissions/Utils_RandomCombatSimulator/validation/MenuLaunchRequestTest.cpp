#include "../MenuLaunchRequest.h"
#include <cassert>
#include <iostream>
#include <vector>
struct Message { int itemId; int subtype; int flags; };
constexpr int WIDGET = 400, NEW_GAME = 101, CLOSE = 2;
void Prepare(Message &message) { message.itemId = NEW_GAME; message.subtype = 13; message.flags = 0; }
int main() {
    using randomcombat::MenuLaunchRequest;
    Message click{WIDGET, 13, 8};
    // API above our hook: it queues only after our original result is returned.
    MenuLaunchRequest outerApi;
    std::vector<int> outerEvents;
    auto native = [&](Message &message) {
        outerEvents.push_back(message.itemId);
        if (message.itemId == NEW_GAME) {
            assert(message.subtype == 13 && message.flags == 0);
            message.itemId = message.subtype = 10; // Native END_DIALOG output.
            return CLOSE;
        }
        return 0;
    };
    assert(outerApi.Process(click, native, Prepare) == 0);
    outerApi.Queue();
    Message escape{27, 0, 0};
    assert(outerApi.Process(escape, native, Prepare) == CLOSE);
    assert(escape.itemId == 10 && escape.subtype == 10);
    assert((outerEvents == std::vector<int>{WIDGET, NEW_GAME}));
    assert(click.itemId == WIDGET && click.flags == 8); // API's message remains valid.
    assert(outerApi.Process(escape, native, Prepare) == 0); // Request consumed once.
    // API below our hook: callback queues during invocation of the original chain.
    MenuLaunchRequest innerApi;
    std::vector<int> innerEvents;
    auto api = [&](Message &message) {
        innerEvents.push_back(message.itemId);
        if (message.itemId == WIDGET) { innerApi.Queue(); return 0; }
        assert(message.itemId == NEW_GAME && message.subtype == 13 && message.flags == 0);
        message.itemId = message.subtype = 10;
        return CLOSE;
    };
    assert(innerApi.Process(click, api, Prepare) == CLOSE);
    assert(click.itemId == 10 && click.subtype == 10);
    assert((innerEvents == std::vector<int>{WIDGET, NEW_GAME}));
    // Unrelated menu results must pass through intact; leave/enter clears a request.
    MenuLaunchRequest none;
    assert(none.Process(escape, [](Message &) { return 7; }, Prepare) == 7);
    none.Queue(); none.Clear();
    assert(none.Process(escape, [](Message &) { return 9; }, Prepare) == 9);
    std::cout << "PASS: both API/hook orders, native New Game close result and message, one-shot launch and cancellation\n";
}
