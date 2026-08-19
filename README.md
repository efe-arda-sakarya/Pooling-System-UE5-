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

**5. Let short-lived actors return themselves (optional).**

Add a **Pool Lifetime** component to anything that should disappear on a timer — impact flashes, tracers, expiring pickups — and set its **Lifetime**. The countdown starts when the pool hands the actor out and is cancelled if the actor comes back early. No Blueprint wiring at all.

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
| Per Frame | Integer | Spread the work over frames, `0` = all at once (advanced pin) |

Makes sure the pool holds **at least** `Count` instances. It is not additive: calling it twice with 50 gives you 50, not 100. Safe to call at any time — before a boss fight, at the start of a wave, or in `Begin Play`.

See [Prewarming without stalling](#prewarming-without-stalling) if your counts run into the thousands.

### Configure Pool

Sets a pool's **Overflow Policy** and **Max Size** without creating anything. Use it when you are happy for a pool to grow on demand but still want a ceiling, or want it to refuse rather than grow.

### Prewarm From Profile

Applies every row of a **Pool Profile** asset in one call. See [Pool profiles](#pool-profiles) below.

### Clear Pool

Destroys every idle instance of a pool. Instances still checked out are left alone and return to the pool as normal. Use it when leaving a heavy area and you want the memory back.

### Get Pool Stats

Returns **Total**, **Active** and **Available** for one pool. Useful for a debug overlay, and for showing the difference in a demo.

### Get All Pool Stats

The same three numbers summed across every pool in the world. **Get Pool Count** tells you how many distinct pools exist.

### Get Pooling Subsystem

Returns the underlying `Pooling Subsystem`. Rarely needed from Blueprint; it exists for C++ callers and for advanced use.

---

## Pool profiles

Typing the same pool sizes into every level gets old, and keeping them in sync gets worse. A **Pool Profile** is an asset that holds the list once.

Create one with **right-click → Miscellaneous → Data Asset → Pool Profile**, then fill in the rows — the same four fields you would type on a prewarmer: actor class, count, overflow policy, max size.

Point a **Pool Prewarmer** at it with its **Profile** field, or call **Prewarm From Profile** yourself.

A prewarmer can use both at once. The profile is applied first, then the prewarmer's own **Pools To Prewarm** rows, so a single level can override one class without forking the shared asset:

| Source | Row | Result |
|---|---|---|
| `DA_Pools` (profile) | `BP_Bullet` → 120 | |
| Prewarmer (inline) | `BP_Bullet` → 200 | **200** — the inline row runs last |

Leave the inline list empty if you only want the profile.

One asymmetry worth knowing: prewarming only ever **tops a pool up**, it never tears one down. An inline row asking for 200 when the profile said 120 creates 80 more; an inline row asking for 50 leaves all 120 in place. Overflow policy and max size are overwritten either way, so a smaller inline row still changes the rules even though it changes no instances.

---

## Prewarming without stalling

Every prewarmed instance is a real `SpawnActor` with a real `BeginPlay`. A few hundred is nothing; a few thousand is a frozen frame at level start.

Set **Per Frame** on the pool row — on the profile or on the prewarmer — and the pool fills a slice at a time while the level keeps running:

| Per Frame | Behaviour |
|---|---|
| `0` (default) | Everything is created immediately. Right for small pools |
| `200` | 200 instances per frame until the pool is full |

**Get Pending Prewarm Count** returns how many are still queued, so a loading screen can wait for it to reach zero.

You do not have to guess where the line is. The plugin times the work and tells you when it hurt:

```
Warning: Prewarming 8000 instances of 'BP_Bullet' took 3400 ms in one frame,
which is a visible hitch. Set Per Frame on that pool row to spread the work
across several frames instead.
```

That number is measured, not estimated — an actor's real cost depends on its components, and the only honest way to know it is to build one and look at the clock.

---

## Pool Lifetime component

Add it to any actor that should return itself after a set time.

| Property | Default | Meaning |
|---|---|---|
| Lifetime | `2.0` | Seconds before the actor returns itself. `0` disables the countdown |
| Auto Start | `true` | Start counting as soon as the pool hands the actor out |

It also exposes **Start Lifetime**, **Cancel Lifetime** and **Get Remaining Lifetime** if you want to drive it yourself — set **Auto Start** to false and call **Start Lifetime** when it suits you.

The countdown is cancelled automatically when the actor goes back into the pool, so an actor that is despawned early never fires a stale timer.

---

## Measuring what pooling saves you

Pooling is supposed to be an optimisation, so the plugin lets you prove it.

**Set Pooling Bypassed** turns the whole system off without touching a single line of calling code: **Spawn Pooled Actor** becomes a plain `SpawnActor`, **Despawn Pooled Actor** becomes a plain `Destroy`. Everything that spawns through the plugin follows automatically — there is nothing to branch and nothing to duplicate.

| Function | Meaning |
|---|---|
| Set Pooling Bypassed | Turn the bypass on or off |
| Toggle Pooling Bypassed | Flip it and return the new state — one node for a debug key |
| Is Pooling Bypassed | For an on-screen readout |

Toggling mid-game is safe: actors handed out before the switch still return to their pool correctly.

A useful overlay is three numbers — frames per second, **Get All Pool Stats**, and **Is Pooling Bypassed** — with one key bound to **Toggle Pooling Bypassed**. Flip it under load and watch the frame time move.

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

Add `"PoolingSystem"` to `PublicDependencyModuleNames` in your `.Build.cs`.

---

## What ships in the box

| Folder | Contents |
|---|---|
| `Source/PoolingSystem` | The whole plugin — one module, nothing optional |
| `Content/PoolingSystemDemo` | The demo level and its assets |

There is no separate demo module: the demo is built from the same classes you get, so anything you see it doing you can do too. Deleting `Content/PoolingSystemDemo` removes the demo and leaves the plugin working.

---

## Documentation

- **[Usage examples](Docs/Examples.md)** — bullets, impact effects, enemy waves, step by step
- **[FAQ](Docs/FAQ.md)** — including "why isn't Begin Play called?" and "why doesn't my actor reset?"

---

## License

MIT — see [LICENSE](LICENSE). Free for any use, including commercial projects.
