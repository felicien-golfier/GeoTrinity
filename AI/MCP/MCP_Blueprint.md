# MCP Blueprint Creation

Creating and configuring Blueprint assets through `execute_script`. See `AI/Python/` for the call patterns:
creating an asset via a factory, setting CDO properties, setting a `FGameplayTag` by `import_text`, setting a
`TSubclassOf`, mutating components through `SubobjectDataSubsystem`, copying a `TInstancedStruct`, moving,
deleting and searching assets.

## Property access

- The CDO accessor is `unreal.get_default_object(bp.generated_class())`; calling `get_default_object()` on the
  class object fails.
- Property names are **PascalCase**, matching the C++ `UPROPERTY`. A boolean answers to its C++ name with the
  `b` kept and to its snake_case name with the `b` dropped, but never to the `b`-less PascalCase form.
- `EditDefaultsOnly` private properties need `meta=(AllowPrivateAccess="true")`. When the property is project
  C++, add the specifier — do not work around it.
- Resolve a class by its script path with the class loader; there is no find-by-name helper, and that lookup is
  also the only answer to whether a class is in the running build, since a reflected class is not always
  mirrored as a module attribute. The object factory takes such a class as readily as a module attribute, so a
  class the module omits can still be instanced, and its CDO is reachable by loading the script path with the
  `Default__` prefix — its reflected functions are then callable with the by-name method caller, which also
  works for static function libraries.
- A soft-object-pointer property takes the loaded asset object; a soft-object-path value fails type conversion.
- A byte property backed by an unexposed enum can be neither read nor written from Python, and the property API
  refuses private properties — changing one needs a C++ shim.
- A gameplay-tag container is read as exported text with `export_text` (there is no `to_string`) and written by
  round-tripping the desired tags through a fresh container's `import_text`.
- An instanced subobject property reaches spawned instances only when it holds the default subobject the owning
  C++ class creates under that name; one assigned afterwards stays on the class defaults and every instance gets
  none. Declare the subobject's class in C++ and let script author only its properties.

## Editing a struct container

Elements read from a struct array or map are by-value copies, so mutating one in place does not write back, and
`EditDefaultsOnly` struct fields reject the setter even on the copy. Clone the element by round-tripping its
exported text into a fresh struct, then merge overrides — a single-field text import sets only that field and
preserves the rest — reassign the whole rebuilt container and save. An object reference merges in as its
exported text, never as the asset itself. A container edit can leave the package clean, so save unconditionally.

See `AI/Python/Asset/struct_container_edit.py` for both container kinds and `AI/Python/Ability/ability_info_icons.py` for a
worked array case.

## Authoring a curve asset

A curve asset holds its channels in a fixed-size array the reflection system does not expose, so keys go in
through the CSV importer rather than the property setter. The importer reads one row per key — a time followed
by one value per channel, no header — and the number of values per row picks the curve class. Mark the import
task automated, or the options dialog stalls an unattended run. See `AI/Python/Asset/curve_asset_authoring.py`.

## Naming conventions

| Asset type | Prefix |
|---|---|
| Gameplay Ability Blueprint | `GA_` |
| Pattern Blueprint | `BP_` |
| Effect Data Asset | `DA_` |
| Widget Blueprint | `WBP_` |

Enemy abilities live under `/Game/AbilitySystem/Abilities/Enemy/`, one subfolder per ability; HUD widgets under
`/Game/HUD/`, grouped by widget type.
