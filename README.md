# Pooling System

Blueprint-first actor pooling for Unreal Engine.

Spawning and destroying actors is expensive. If your game creates bullets, impact effects, pickups or enemies every few frames, that cost shows up as frame hitches and garbage collection spikes. Pooling solves it by creating the actors once and reusing them: instead of destroying an actor, you put it to sleep and hand it back out when it is needed again.

**Supported engine versions:** 5.7, 5.8
**Cost:** free
**Networking:** not supported in this version (see [FAQ](Docs/FAQ.md))

---

## What makes this one different

- **No base class.** Any `AActor` subclass works, including Blueprints you already have. Nothing to reparent, nothing to migrate.
- **No setup.** There is no manager to place, no config asset to create, no registration step. Ask for an actor and you get one.
- **One node.** `Spawn Pooled Actor` and `Despawn Pooled Actor` are the whole API for most projects.
- **Pools are keyed by class.** No pool names to invent, remember or misspell.

---

## Installation

1. Copy the `PoolingSystem` folder into your project's `Plugins` directory. Create the directory if it does not exist:
   ```
   MyProject/
     Plugins/
       PoolingSystem/
         PoolingSystem.uplugin
   ```
2. Open the project. If Unreal asks to rebuild missing modules, accept.
3. Confirm it loaded: **Edit → Plugins**, search for "Pooling System".

C++ projects only: add `"PoolingSystem"` to the dependency list in your `.Build.cs` if you want to call the API from C++.

---

## Quick start

**1. Spawn from the pool instead of spawning normally.**

Replace your `Spawn Actor from Class` node with `Spawn Pooled Actor`. Set the class and the transform exactly as you did before.

The first call creates the pool automatically. You do not have to register anything first.

**2. Despawn instead of destroying.**

Replace `Destroy Actor` with `Despawn Pooled Actor`. The actor is hidden, its collision and tick are switched off, and it goes back into the pool.

**3. Reset your own state (optional, but usually needed).**

The plugin resets visibility, collision, tick and physics velocity. It cannot know about *your* variables. Add the **Poolable** interface to your actor:

- Class Settings → Interfaces → Add → **Poolable**
- Implement **On Pool Spawned** — set health, clear the target, start effects
- Implement **On Pool Despawned** — stop effects, clear references

Actors that do not implement the interface still pool correctly. They just do not reset their own variables.

**4. Prepare pools ahead of time (optional).**

Drag a **Pool Prewarmer** into your level and fill in its **Pools To Prewarm** list: one row per class, with a count. Those instances are created when the level starts, so the first shot of the game costs nothing extra.

Without a prewarmer everything still works — the pool simply grows the first time you use it.

Step-by-step scenarios are in **[Docs/Examples.md](Docs/Examples.md)**.

---

## API

All nodes are under the **Pooling** category and can be called from anywhere in Blueprint — no target pin, no subsystem to fetch.

### Spawn Pooled Actor

| Pin | Type | Meaning |
|---|---|---|
| Actor Class | Class | Which actor to take from the pool |
| Spawn Transform | Transform | Where to place it |
| *Return Value* | Actor | The actor, or nothing if the pool refused |
| *Success* | Boolean | Whether an actor was handed out |

Creates the pool on first use. The actor is moved into place, unhidden, re-collided and re-ticked, and then **On Pool Spawned** fires.

Check **Success** rather than the actor reference if you use the Reject policy.

### Despawn Pooled Actor

| Pin | Type | Meaning |
|---|---|---|
| Actor | Actor | The actor to return |
| *Return Value* | Boolean | False if the actor did not come from a pool, or was already returned |

Fires **On Pool Despawned**, then hides the actor, disables collision and tick, zeroes physics velocity, stops any movement component, and clears timers bound to that actor.

Calling it twice on the same actor is safe — the second call returns false and logs a warning.

### Prewarm Pool

| Pin | Type | Meaning |
|---|---|---|
| Actor Class | Class | Which pool |
| Count | Integer | Target number of instances |
| Overflow Policy | Enum | `Grow` or `Reject` (advanced pin) |
| Max Size | Integer | Upper bound, `0` = unlimited (advanced pin) |

Makes sure the pool holds **at least** `Count` instances. It is not additive: calling it twice with 50 gives you 50, not 100. Safe to call at any time — before a boss fight, at the start of a wave, or in `Begin Play`.

### Clear Pool

Destroys every idle instance of a pool. Instances still checked out are left alone and return to the pool as normal. Use it when leaving a heavy area and you want the memory back.

### Get Pool Stats

Returns **Total**, **Active** and **Available** for one pool. Useful for a debug overlay, and for showing the difference in a demo.

### Get Pooling Subsystem

Returns the underlying `Pooling Subsystem`. Rarely needed from Blueprint; it exists for C++ callers and for advanced use.

---

## Overflow: what happens when the pool runs out

Every pool has a policy, set per class on the Pool Prewarmer or through `Prewarm Pool`:

| Policy | Behaviour |
|---|---|
| **Grow** (default) | Creates another instance. The pool never fails, but it can keep growing. Set **Max Size** to cap it. |
| **Reject** | Hands out nothing and sets **Success** to false. Memory stays fixed; your Blueprint must handle the failure. |

Either way the plugin logs a warning **once per pool** the first time it happens, so you find out that your prewarm count is low without your Output Log being flooded.

If a pool keeps growing, the usual cause is a missing `Despawn Pooled Actor` somewhere — the actors are never coming back.

---

## C++ usage

```cpp
#include "PoolingSubsystem.h"

if (UPoolingSubsystem* Pool = GetWorld()->GetSubsystem<UPoolingSubsystem>())
{
    bool bSuccess = false;
    AActor* Bullet = Pool->SpawnPooledActor(BulletClass, SpawnTransform, bSuccess);

    // ...later
    Pool->DespawnPooledActor(Bullet);
}
```

Implement `IPoolable` on your actor to receive `OnPoolSpawned` / `OnPoolDespawned`.

---

## Documentation

- **[Usage examples](Docs/Examples.md)** — bullets, impact effects, enemy waves, step by step
- **[FAQ](Docs/FAQ.md)** — including "why isn't Begin Play called?" and "why doesn't my actor reset?"

---

## License

MIT — see [LICENSE](LICENSE). Free for any use, including commercial projects.
