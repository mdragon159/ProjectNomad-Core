# ProjectNomad-Core
Public portion of core library for tentatively named Project Nomad game

In particular, this repo focuses on supporting a fully deterministic cross-platform game via the following goals:
- Supply a fixed-point type with associated math functions for "easy" float replacement
- Supply a 3D physics library using that fixed-point type for deterministic physics

However, take note that this library is in early development with various bugs, TODOs, and otherwise messy 
in-progress code.

## Requirements
1. Install Visual Studio (using 2019 Community edition).
- Make sure to install the "Desktop development with C++" workload/package set. In addition, after clicking on that workload, make sure "Test Adapter for Google Test" 
is selected in the sidebar

Recommended to also use [JetBrains Rider for Unreal IDE](https://www.jetbrains.com/lp/rider-unreal/) as Visual Studio is very... not good with C++

## Setup
1. Check out repo
   - Recommended to create a folder as high up in file directory as possible for repo. This is due to
     Google Test files being named farrr too long
2. Open the solution located in `Solution/ProjectNomadCore.sln`\
3. Create a copy for all files in `ProjectNomadCore/Secrets` and remove '.example.' from their names
    - Eg, copy `NetworkSecrets.example.h` and name it `NetworkSecrets.h`
    - For full functionality, you can fill in the secrets with your own values. However for now, 
        the project as a whole will compile without those secrets filled in
4. Build and run tests

## Physics Engine Details

The intention behind the physics engine is the following:
- "Simple" (as barebones as possible) real-time 3D physics engine which supports the needs of Project Nomad
  - This includes being able to possibly grapple and swing around a level, other arbitrary movement options 
  (eg, wall running), and any needs of a fast-paced 3D melee combat game while also being "decently" performant
    - Preferably as performant as possible  but opting to worry about performance bit by bit once there's more gameplay in the actual game
- The real reason I made this is that I was naïve enough to think that "creating simple physics" from scratch was super easy… Really should have just bugged Havok more for their engine or replaced all floats with fixed point in an existing engine, but hey it works! Or at least works well enough for the moment!

#### What does the physics engine include?
- Bugs
- Fixed point support (for full cross-platform determinism at the cost of speed)
- Simple colliders ("primitives"): OBB (box), Sphere, Capsule
  - No arbitrary shapes in current design as no need
- Raycasting/linetesting (ray or line vs the simple colliders above)
- "Simple" collision testing (checking if primitives collide with one another at all)

#### What does the physics engine not include yet but will include?
- "Complex" collision testing (check if primitives collide then calculate intersection point, axis, depth, etc for proper collision resolution)
- Some sort of actual Broadphase (one day)
  - Note that current physics engine does not scale well. Ie, right now it's doing very straightforward n^2 collision checks every frame
- Cone collision tests ("conecast" to check for any colliders within a cone)
  - Designed on paper for grapple point searching, but not yet implemented
- Separation of overarching collision checking and resolution code out of the main ECS code
  - Currently have a gray line for much should be in ECS-game side directly (eg, gravity which I like to easily turn off on an entity-state basis) vs separate physics engine side (broadphase collision detection and resolution plus data ownership)
- Trigger volumes (ie, walk into area specified by non-collision "collider"/primitive and trigger some arbitrary event)
Collision layers (eg, set camera to not collide against player and enemies)

#### What will the physics engine likely never include?
- Collision with arbitrary shapes/polygons/meshes
- Collision with concave shapes
- "Joints" (combining colliders in some fashion, like two boxes connected by a joint)
  - No anticipated need for this

#### Other notable elements:
- Trying to avoid pointers (including interfaces/virtual methods and overloads) as much as possible for performance reasons
- Likewise, trying to avoid object oriented programming as much as possible
- Also header-only as is a pain dealing with linker (esp with Unreal)

## Style Guide

After much deliberation... I'm not a fan of a single style guide, due to the mismatch of prior
experiences with styles in other languages (such as Java and C#) and dislike of Unreal style
vs my personal preference.

Thus, taking note of preferred style below:

### Inspirations
- [Google Style Guide](https://google.github.io/styleguide/cppguide.html) is decent but not perfect
- Recommended talk: [When C++ Style Guides Contradict](https://www.youtube.com/watch?v=WRQ1xqYBKgc)
- Also not necessarily style but good talk to add here nonetheless: [Breaking Dependencies: The SOLID Principles](https://www.youtube.com/watch?v=Ntraj80qN2k)

### General
- Use 4 space indenting
  - Use 2 spaces for `public`/`protected`/`private` declarations
- 120 line character limit. Try to stick to character limit whenever possible (ie, have good reason when breaking rule)

- Use const and constexpr wherever possible
- Do NOT capture everything by reference for lambdas (eg, `[&]`). Use `[this]` if need reference to self
- Follow class format from [Google Style Guide](https://google.github.io/styleguide/cppguide.html#Class_Format)
  - One `public`, `protected`, and `private` section each, in that order
  - Prefer functions before data members

### Naming
- Follow variable, constant, enumerator, and function names from 
  [Google Style Guide](https://google.github.io/styleguide/cppguide.html#Variable_Names) *EXCEPT* 
  - Variables should be lowercase with underscore (snake_case)
    - So `table_name` instead of `tableName`. Yes I normally like camelCase too
    - Struct member variables should be identical to above
  - Class data members (but NOT struct data members) should have a trailing underscore
    - Eg, `table_name_`
  - Constants should have a leading `k` followed by mixed case
    - Eg, `kDaysInAWeek = 7`
  - All function names should have "mixed" case (PascalCase)
    - Eg, `DoSomeStuff()`
- Variables should be in camelCase
  - So `tableName` instead of `table_name`
  - Struct member variables should be identical to above
- Class data members (but NOT struct data members) should have a leading `m`
  - Eg, `mTableTable` instead of `table_name` or `tableName_`
    - That trailing underscore looks terrible when calling member functions or such
- Constants should have a leading `k` followed by mixed case. Enums should also follow this
  - Eg, `kDaysInAWeek = 7`
  - This is taken straight from the [Google Style guide](https://google.github.io/styleguide/cppguide.html#Constant_Names)
    as it _seems_ to work pretty well with other rules

### Pointers
- Don't use raw pointers except when absolutely necessary 
  - Prefer passing around references when possible instead
- Unreal: Use `TUniquePtr` when owning an object and `TWeakPtr` when not owning an object
  - [Link to Unreal Smart Pointer Library documentation](https://docs.unrealengine.com/4.27/en-US/ProgrammingAndScripting/ProgrammingWithCPP/UnrealArchitecture/SmartPointerLibrary/)
  - Note that this means something pointed towards COULD be garbage collected. Thus we'll need to
    to be very careful with the lifetime of all objects pointed towards at all times
  - Preferably, this means "owned" pointers are only held by the `GameInstance` and the 
    `GameInstance` is the only object which creates or destroys other objects which it points to
- Likewise, for native C++ code use `std::unique_ptr` and `std::weak_ptr`
  - [Link to unique_ptr documentation](https://en.cppreference.com/w/cpp/memory/unique_ptr)