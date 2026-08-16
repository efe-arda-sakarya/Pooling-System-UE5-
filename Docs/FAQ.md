# FAQ

## Why isn't Begin Play called when I spawn from the pool?

Because the actor was already created — usually long before, during prewarm. `Begin Play` runs **once per instance**, when it is first created, not once per use.

Use **On Pool Spawned** instead. That is exactly what it is for. If you have setup that should genuinely happen only once, `Begin Play` is still the right place for it.

## Why doesn't my actor reset?

The plugin resets what it can see: visibility, collision, tick, physics velocity, movement components, and timers bound to the actor. It has no way to know about your health variable, your target reference or your state machine.

Add the **Poolable** interface to your actor and reset your own state in **On Pool Spawned**. This is the single most common source of confusion with any pooling system.

## Do I have to use a special base class?

No. Any `AActor` subclass works, including Blueprints you already shipped. The Poolable interface is optional and adds no parent class.

## Do I need to place a Pool Prewarmer?

No. Pools are created the first time you ask for a class. The prewarmer only decides *when* the cost is paid: at level start instead of mid-combat.

## Can I change the pool size at runtime?

Yes, upward. Call `Prewarm Pool` at any time — before a boss, at the start of a wave, whenever. It tops the pool up to the count you ask for and never shrinks it.

There is no partial shrink. `Clear Pool` destroys every idle instance at once; instances still in use are untouched.

## My particle effect keeps playing after despawn

Hiding an actor does not stop a Niagara or Cascade component, and it does not stop audio. Stop them in **On Pool Despawned**:

```
Event On Pool Despawned
  └─> Niagara Component → Deactivate
  └─> Audio Component  → Stop
```

## What happens when the level changes?

Pools are destroyed with the level, the same as any actor in it. Nothing to clean up and nothing to reset — the next level starts with empty pools.

This is deliberate. Actors belong to a world; a pool that outlived its world would be holding references to destroyed actors.

## What happens if the pool runs out?

It depends on the pool's **Overflow Policy**:

- **Grow** (default) — another instance is created. The pool never fails.
- **Reject** — nothing is handed out and **Success** is false.

Either way you get one warning in the Output Log per pool, the first time it happens. Take it as a hint that your prewarm count is low, or that something is not being despawned.

## The pool keeps growing and performance is getting worse

Almost always a missing `Despawn Pooled Actor`. The actors are handed out and never come back, so every request creates another one — which is worse than not pooling at all, because you also pay for the bookkeeping.

`Get Pool Stats` will show it immediately: **Active** climbing and never dropping. Set **Max Size** while you hunt for the cause.

## I can only despawn the last actor I spawned — how do I track several at once?

`Despawn Pooled Actor` needs a specific actor reference, the same way `Destroy Actor` does — the pool has no way to guess which instance you mean. If your Blueprint only keeps a single "last spawned actor" variable, calling Despawn twice in a row fails the second time: the reference still points at an actor you already returned.

In real gameplay code this rarely comes up, because actors usually despawn themselves (a bullet calls `Despawn Pooled Actor (Self)` on hit). If you are managing several pooled actors from one Blueprint — a spawner, a debug panel, a wave controller — track them yourself with an array used as a stack:

```
On spawn:
  Spawn Pooled Actor  ─>  Branch (Success)
                              └─> Array Add  (Target: SpawnedActors, New Item: the spawned actor)

On despawn:
  Branch (SpawnedActors → Length > 0)
    └─> Get Last Index  ─> Get (a copy)  ─> Despawn Pooled Actor
                        └─> Remove Index  (same index, after despawning)
```

This despawns the most recently spawned actor first and keeps working no matter how many times you call it in a row.

## How much is pooling actually saving me?

Measure it. Call **Set Pooling Bypassed** (or bind **Toggle Pooling Bypassed** to a debug key) and the plugin stops pooling entirely: spawning becomes a plain `SpawnActor`, despawning a plain `Destroy`. Your calling code does not change at all, so you are comparing the same scene against itself.

Put **Get All Pool Stats** and a frame-time readout on screen, run your heaviest moment, and flip the switch. If the two numbers are identical, pooling is not your bottleneck and you can spend your effort elsewhere — that is a useful answer too.

Toggling mid-game is safe: actors that were handed out before the switch still return to their pool correctly.

## Do I have to type my pool sizes into every level?

No. Make a **Pool Profile** asset (right-click → Miscellaneous → Data Asset → Pool Profile), fill in the rows once, and point every Pool Prewarmer at it with the **Profile** field.

If one level needs a different number for a single class, add just that row to the prewarmer's own **Pools To Prewarm** list. The profile is applied first and the inline rows run after it, so the inline value wins without you having to duplicate the whole asset.

## How do I despawn something after a few seconds?

Add a **Pool Lifetime** component to the actor and set **Lifetime**. That is the whole setup — no timer nodes, no Begin Play wiring.

The countdown starts when the pool hands the actor out and is cancelled if the actor is returned early, so an actor that is despawned on impact never fires a leftover timer on its next use.

If you want to control the timing yourself, set **Auto Start** to false and call **Start Lifetime** when it suits you.

## Does it support networking / replication?

Not in this version. The plugin runs locally on whichever instance calls it. Replicated actors have their own lifetime rules that pooling has to respect, and doing that properly is a larger job than this plugin takes on.

If you need pooled replicated actors today, this is not the right tool.

## Can I pool Characters and Pawns?

Yes, with care. Characters carry more state than a bullet: movement mode, controller possession, animation state. Reset all of it in **On Pool Spawned**, and unpossess or stop the AI in **On Pool Despawned**. Example 3 in [Examples.md](Examples.md) shows the shape of it.

## What if I destroy a pooled actor manually?

The pool detects the destroyed entry and skips it, so you get a valid actor rather than a crash. But you also lose an instance permanently — the pool does not notice it should create a replacement until it next runs out. Prefer `Despawn Pooled Actor`.

## Can I have two separate pools of the same class?

No. Pools are keyed by class, which is what removes the need to name or register anything. If you genuinely need two pools with different settings, make two Blueprint subclasses.

## Does it work in the editor viewport, outside Play mode?

No. The pooling subsystem only exists in game and PIE worlds. There is nothing for it to do in an editor world.

## Where do the log messages come from?

The `LogPoolingSystem` category. Filter the Output Log by it to see only pooling messages.
