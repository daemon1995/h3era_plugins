#include "RuntimeState.h"
#include "ObjectRendering.h"
#include <cassert>
#include <cstdio>

using namespace erm_functions;

static void TestRegistrations()
{
    ObjectTypes types;
    assert(types.Register(112, -1, 1));
    assert(types.Find(112, 0) && *types.Find(112, 0));
    assert(types.Register(112, 3, 0));
    assert(types.Find(112, 3) && !*types.Find(112, 3));
    assert(*types.Find(112, 4));
    assert(types.Register(112, 3, 1));
    assert(*types.Find(112, 3));
    assert(!types.Register(-1, 0, 1));
    assert(!types.Register(232, 0, 1));
    assert(!types.Register(112, -2, 1));
    assert(!types.Register(112, 32768, 1));
    assert(!types.Register(112, 0, 2));
    assert(!types.Find(113, 0));
    types.Clear();
    assert(!types.Find(112, 0));
    for (int owner = -3; owner < 10; ++owner)
        assert(ValidOwner(owner) == (owner >= -1 && owner <= 7));
}

static void TestTimeline()
{
    ReplayTimeline timeline;
    ReplayEvent event;
    event.args[0] = 10;
    assert(timeline.Add(event, 0));
    event.args[0] = 20;
    assert(timeline.Add(event, 1));
    event.args[0] = 30;
    assert(timeline.Add(event, 1));
    event.args[0] = 40;
    assert(timeline.Add(event, 2));
    std::vector<int> calls;
    auto callback = [&calls](const ReplayEvent &ev, bool undo) { calls.push_back((undo ? -1 : 1) * ev.args[0]); };
    assert(timeline.Begin());
    const auto generation = timeline.Generation();
    assert(!timeline.Begin());
    assert(!timeline.Add(event, 2));
    for (int boundary = 2; boundary >= 0; --boundary)
        timeline.AtBoundary(boundary, true, callback);
    for (int boundary = 0; boundary <= 2; ++boundary)
        timeline.AtBoundary(boundary, false, callback);
    assert((calls == std::vector<int>{-40, -30, -20, -10, 10, 20, 30, 40}));
    timeline.ownerView[123] = 7;
    timeline.End(generation);
    assert(!timeline.IsReplaying() && timeline.ownerView.empty());
    // Repeated replay uses the same immutable payload and ordering.
    calls.clear();
    assert(timeline.Begin());
    timeline.AtBoundary(1, false, callback);
    assert((calls == std::vector<int>{20, 30}));
    timeline.End(generation);
    // The native list can shrink outside its ordinary clear function: discard stale events.
    event.args[0] = 50;
    assert(timeline.Add(event, 0));
    assert(timeline.Begin());
    calls.clear();
    timeline.AtBoundary(0, false, callback);
    assert((calls == std::vector<int>{50}));
    timeline.End(timeline.Generation());
}

static void TestResetDuringCallback()
{
    ReplayTimeline timeline;
    ReplayEvent event;
    assert(timeline.Add(event, 0));
    assert(timeline.Add(event, 0));
    timeline.Begin();
    const auto oldGeneration = timeline.Generation();
    int calls = 0;
    timeline.AtBoundary(0, false, [&](const ReplayEvent &, bool) {
        ++calls;
        timeline.Clear();
        // A callback can load a map and even start a new replay scope.
        timeline.Begin();
        timeline.ownerView[456] = 3;
    });
    assert(calls == 1);
    timeline.End(oldGeneration);
    assert(timeline.IsReplaying());
    assert(timeline.ownerView.at(456) == 3);
    timeline.End(timeline.Generation());
}

static void TestOwnershipView()
{
    ReplayTimeline timeline;
    ReplayEvent event;
    event.kind = ReplayEvent::OWNER;
    event.coords = 123;
    event.oldOwner = -1;
    event.newOwner = 2;
    assert(timeline.Add(event, 1));
    event.oldOwner = 2;
    event.newOwner = 7;
    assert(timeline.Add(event, 1));
    timeline.Begin();
    const auto callback = [&timeline](const ReplayEvent &ev, bool undo) {
        timeline.ownerView[ev.coords] = undo ? ev.oldOwner : ev.newOwner;
    };
    timeline.AtBoundary(1, true, callback);
    assert(timeline.ownerView.at(123) == -1);
    timeline.AtBoundary(1, false, callback);
    assert(timeline.ownerView.at(123) == 7);
    timeline.Clear();
    assert(!timeline.IsReplaying() && timeline.ownerView.empty());
}

static void TestMinimapFootprint()
{
    struct Cell { int actualType; unsigned char mirror; unsigned char access; };
    // A 3x3 DEF extends beyond its two blocked cells and its walkable entrance.
    // The old renderer painted all nine cells just because their draw list contained the DEF.
    const Cell cells[] = {
        {0, 0x40, 0}, {112, 0x40, 0}, {0, 0x40, 0},
        {112, 0, 0}, {112, 0, 0}, {5, 0, 0},
        {0, 0x40, 0}, {112, 0x40, 0x10}, {0, 0x40, 0},
    };
    const bool expected[] = {false, false, false, true, true, false, false, true, false};
    unsigned colored = 0;
    for (unsigned i = 0; i < sizeof(cells) / sizeof(cells[0]); ++i)
    {
        const auto &cell = cells[i];
        const bool shown = rendering::IsMinimapObjectCell(cell.actualType, 112, cell.mirror, cell.access);
        assert(shown == expected[i]);
        colored += shown ? 1 : 0;
    }
    assert(colored == 3);
    // Unrelated horizontal/vertical mirror bits must not change the occupied-cell rule.
    assert(rendering::IsMinimapObjectCell(112, 112, 0x03, 0));
    assert(!rendering::IsMinimapObjectCell(112, 112, 0x43, 0));
    assert(rendering::IsMinimapObjectCell(112, 112, 0x43, 0x10));
}

int main()
{
    TestRegistrations();
    TestTimeline();
    TestResetDuringCallback();
    TestOwnershipView();
    TestMinimapFootprint();
    std::puts("API_ErmFunctions runtime checks passed");
}
