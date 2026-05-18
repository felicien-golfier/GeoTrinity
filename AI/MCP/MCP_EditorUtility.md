# MCP Editor Utility Objects

Pattern for exposing C++ editor operations to Python/MCP when `get/set_editor_property` can't reach them (protected properties, template APIs, editor-only subsystems).

---

## When to Use

- A property is protected or not UPROPERTY-exposed (`EditorData` on `UStateTree`, internal arrays, etc.)
- You need to call a C++ template function (`AddTask<T>`, `AddEvaluator<T>`, etc.)
- You need to invoke an editor subsystem that has no Python binding

---

## Pattern

### 1. Header — `Source/GeoTrinity/Public/Tool/GeoXxxUtil.h`

```cpp
// Copyright 2024 GeoTrinity. All Rights Reserved.
#pragma once
#if WITH_EDITOR
#include "CoreMinimal.h"
#include "EditorUtilityObject.h"
#include "GeoXxxUtil.generated.h"

UCLASS()
class GEOTRINITY_API UGeoXxxUtil : public UEditorUtilityObject
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, CallInEditor, Category = "GeoTrinity|Editor")
    static void DoSomething(/* params */);
};
#endif
```

- One class per logical domain (StateTree, DataTable, AnimBlueprint, etc.)
- All methods `static` — no instance state needed
- Wrap `#if WITH_EDITOR` so the class is stripped from game builds

### 2. Implementation — `Source/GeoTrinity/Private/Tool/GeoXxxUtil.cpp`

```cpp
// Copyright 2024 GeoTrinity. All Rights Reserved.
#if WITH_EDITOR
#include "Tool/GeoXxxUtil.h"
#include "FileHelpers.h"   // UEditorLoadingAndSavingUtils::SavePackages

void UGeoXxxUtil::DoSomething(/* params */)
{
    // ... access internals, call editor APIs ...
    UEditorLoadingAndSavingUtils::SavePackages({ Asset->GetPackage() }, false);
}
#endif
```

**Saving assets:** always use `UEditorLoadingAndSavingUtils::SavePackages({ Asset->GetPackage() }, false)` from `FileHelpers.h`. Do not use `UEditorAssetLibrary::SaveLoadedAsset` — it requires the `EditorScriptingUtilities` module.

### 3. Build.cs — editor-only dependencies

```csharp
if (Target.bBuildEditor)
{
    PrivateDependencyModuleNames.AddRange(new string[]
    {
        "UnrealEd",    // UEditorLoadingAndSavingUtils, GEditor, etc.
        "Blutility",   // UEditorUtilityObject base class
        // add domain-specific modules here, e.g.:
        // "StateTreeEditorModule"
    });
}
```

**After any Build.cs change: close the editor and do a full build.** Live Coding cannot relink new DLL dependencies.

```bash
"C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" GeoTrinityEditor Win64 DebugGame "C:\GeoTrinity\GeoTrinity.uproject"
```

### 4. Calling from Python

```python
import unreal
util_class = unreal.load_class(None, "/Script/GeoTrinity.GeoXxxUtil")
util_class.get_default_object().do_something(arg1, arg2)
```

Python uses snake_case for UFUNCTION names automatically. Pass `UObject*` args as loaded assets, `FGameplayTag` via `import_text`.

---

## Existing Utilities

| Class | File | What it does |
|---|---|---|
| `UGeoStateTreeBuilderUtil` | `Public/Tool/GeoStateTreeBuilderUtil.h` | `AddFireAbilityState` — adds a state + `FSTTask_FireProjectileAbility` task to a StateTree |

---

## Notes

- `CallInEditor` in `UFUNCTION` makes the method appear as a button in the Details panel on instances — useful for manual one-shot operations
- Static UFunctions are callable on the CDO: `util_class.get_default_object().method()`
- If a method needs transactional undo support, call `Asset->Modify()` before mutating it
