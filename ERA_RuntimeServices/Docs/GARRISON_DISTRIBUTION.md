# Garrison distribution

The old script stopped after 200 new garrisons while scanning the surface from one edge, followed by the underground. Its counter was reset after generation. This favored the first rows and could leave the underground without new garrisons. The cap is retained. The SDK stores garrisons in a dynamic vector; this change does not assert that all engine subsystems support arbitrary counts.

`Erm_OrderGarrisonSites(xyz, count, mapSize, output, capacity, seed)` is a pure stdcall export. Input is count coordinate triples; output is zero-based input indices. Capacity must be at least count, count is at most 131072, mapSize is 8–256. Invalid coordinates are skipped; invalid arguments or allocation failure return -1. The caller must supply valid buffers. The function never accesses game memory, map functions or the game RNG.

The script first collects candidate cells using its existing corridor checks and density setting, without creating objects. The API partitions each level into up to 7x7 sectors, shuffles candidates within each sector, then takes one candidate from each nonempty sector per round. Sector order is shuffled each round. The script rechecks candidates before creation, until 200 successful creations or exhaustion. The cap counter is initialized before collection. A minimum gap of mapSize/36+3 cells is enforced between new garrisons on the same level. Appearance selection, native placement, PO marking and day-one shooter setup remain in ERM.

This distributes opportunities across sectors with eligible corridors, without guaranteeing equal player counts. No terrain modifications or relaxed corridor checks were introduced. Global connectivity of the legacy templates still needs game validation; the ordering API itself cannot change passability. The script requires the rebuilt plugin and reports a missing export before generation. The DLL is not installed automatically.

Offline C++ checks outside the repository cover 100 seeds: unique indices, deterministic output, all populated sectors visited in the first round, shared-limit fairness between equally populated levels, sparse sectors and invalid coordinates.
