# Usage Examples

Three complete scenarios, from the simplest to the one with the most moving parts. Each one is written for Blueprint; the C++ equivalent is a direct translation.

- [1. Bullets](#1-bullets) — the classic case
- [2. Impact effects](#2-impact-effects) — actors that return themselves
- [3. Enemy waves](#3-enemy-waves) — growing a pool between waves

---

## 1. Bullets

A weapon that fires several times per second. Without pooling, every shot spawns an actor and every hit destroys one.

### The bullet

Assume you already have `BP_Bullet`: a Projectile Movement component, a collision component, a mesh.

**Step 1 — Add the Poolable interface.**

Open `BP_Bullet` → **Class Settings** → **Details → Interfaces → Add** → **Poolable**.

**Step 2 — Reset on spawn.**

In the Event Graph, right-click → add **Event On Pool Spawned**:

```
Event On Pool Spawned
  └─> Set Damage        (your default, e.g. 10)
  └─> Set Has Hit       (false)
  └─> Projectile Movement → Set Velocity in Local Space  (Vector: 3000, 0, 0)
  └─> Projectile Movement → Set Component Tick Enabled   (true)
```

The velocity line matters: Projectile Movement only picks up its initial speed when it is first created. A pooled bullet has already been created, so you set the velocity yourself each time.

**Step 3 — Clean up on despawn.**

```
Event On Pool Despawned
  └─> Projectile Movement → Stop Movement Immediately
```

**Step 4 — Return the bullet on hit.**

Wherever you previously called `Destroy Actor`:

```
Event Hit  (or Event ActorBeginOverlap)
  └─> Branch (Has Hit == false)
        └─> Set Has Hit (true)
        └─> Apply Damage ...
        └─> Despawn Pooled Actor  (Actor: Self)
```

The `Has Hit` guard is worth keeping: a fast bullet can register two hits in one frame, and you do not want to despawn the same actor twice.

### The weapon

```
Fire
  └─> Spawn Pooled Actor
        Actor Class:     BP_Bullet
        Spawn Transform: Muzzle → Get World Transform
  └─> Branch (Success)
        └─> (optional) do something with the returned actor
```

### Prewarming

Drag a **Pool Prewarmer** into your level:

| Actor Class | Count | Overflow Policy | Max Size |
|---|---|---|---|
| BP_Bullet | 60 | Grow | 0 |

Pick a count that covers your worst realistic burst. If you see the "pool ran out of instances" warning in the Output Log, raise it.

---

## 2. Impact effects

Short-lived actors that clean up after themselves — no external system has to remember to return them.

### The effect actor

`BP_ImpactEffect`: a Niagara component, maybe an audio component.

**Step 1 — Add the Poolable interface** (as above).

**Step 2 — Play and schedule the return.**

```
Event On Pool Spawned
  └─> Niagara Component → Activate  (Reset: true)
  └─> Audio Component  → Play
  └─> Set Timer by Event  (Time: 2.0, Looping: false)
        └─> Despawn Pooled Actor  (Actor: Self)
```

**Step 3 — Stop everything on despawn.**

```
Event On Pool Despawned
  └─> Niagara Component → Deactivate
  └─> Audio Component  → Stop
```

This step is not optional for effects. Hiding an actor does not stop a Niagara system or an audio component — they keep running invisibly and cost you performance. Stop them explicitly.

You do not need to clear the timer yourself: the plugin clears timers bound to the actor when it goes back into the pool.

### Spawning the effect

```
On Hit
  └─> Spawn Pooled Actor
        Actor Class:     BP_ImpactEffect
        Spawn Transform: Make Transform (Location: Hit Location, Rotation: Hit Normal → Make Rot from Z)
```

Nothing else to do. The effect returns itself two seconds later.

---

## 3. Enemy waves

Enemies are heavier than bullets, and the number you need changes as the game goes on.

### The enemy

**Step 1 — Add the Poolable interface.**

**Step 2 — Full reset on spawn.** Enemies carry more state than bullets, so this event does more:

```
Event On Pool Spawned
  └─> Set Health         (Max Health)
  └─> Set Current Target (none)
  └─> Set Is Dead        (false)
  └─> Mesh → Set Visibility (true)
  └─> Character Movement → Set Movement Mode (Walking)
  └─> (if using AI) Spawn Default Controller / Run Behavior Tree
```

**Step 3 — Despawn instead of dying.**

```
On Death
  └─> Set Is Dead (true)
  └─> Play death montage / effect
  └─> Delay (2.0)
  └─> Despawn Pooled Actor (Actor: Self)
```

```
Event On Pool Despawned
  └─> Character Movement → Disable Movement
  └─> (if using AI) Unpossess / Stop Behavior Tree
```

### The wave manager

Start with a modest pool and grow it as the waves get bigger:

```
Start Wave (Wave Number)
  └─> Prewarm Pool
        Actor Class:     BP_Enemy
        Count:           Wave Number * 10
        Overflow Policy: Grow
        Max Size:        200
  └─> ForLoop (0 .. Wave Number * 10)
        └─> Spawn Pooled Actor
              Actor Class:     BP_Enemy
              Spawn Transform: (a random point from your spawn volume)
```

`Prewarm Pool` is not additive — asking for 30 when you already have 20 creates 10 more, not 30 more. That is what makes it safe to call at the start of every wave.

`Max Size: 200` is the safety net. If a bug stops enemies from being despawned, the pool stops at 200 instead of growing until the game runs out of memory.

### Cleaning up between levels

You do not need to. Pools live and die with the level. If you want the memory back *within* a level — say the arena is over and you are moving to a hub area — call:

```
Clear Pool  (Actor Class: BP_Enemy)
```

Instances still alive in the world are left alone; only the idle ones are destroyed.

---

## Debug overlay

Useful while tuning your prewarm counts, and it makes a good demo:

```
Event Tick  (on your HUD)
  └─> Get Pool Stats (Actor Class: BP_Bullet)
        └─> Break Pool Stats → Total / Active / Available
        └─> Draw Text  ("Bullets: {Active} / {Total}")
```
