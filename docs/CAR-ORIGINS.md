# Car donor metadata and automatic slots

This is local experimental work. Restart after changing car mods. Live tests of
allocation, removal, reinstall and save migration are still required before release.

Place `origin.txt` at the mod root, beside `assets/` or its packed VFS pair:

```ini
[mod]
id=my-unique-car-package
overwrite=0

[origins]
alf=alf

[fallbacks]
alf=fab
```

The mapping is **authored car ID = stock donor ID**. `alf` is Pulse. Use the
package's actual authored ID on the left; do not guess a donor from its geometry.
Multiple mappings may share one package. IDs in `[origins]` are three lowercase
ASCII letters/digits. Keep the package `id` stable across updates and renames.
If `id` is omitted, the folder name becomes its identity, so renaming that folder
creates a different car.

`[fallbacks]` is optional and uses the same authored IDs. It names the **stock
car shown when the custom car's slot is unavailable**, such as after disabling or
removing the mod. It is independent of the donor: the donor still supplies
copied assets, configuration, save identity and ghost identity. Without a
fallback entry, or if the declared car is not a valid loaded stock car, the
donor is used. Conflicting fallbacks for one authored ID are refused.

- `overwrite=1` replaces the donor in its existing stock slot. It retains that
  stock car's save identity. Two enabled replacements for the same donor are
  rejected rather than choosing one silently.
- `overwrite=0` allocates a custom slot, copying missing assets/configuration from
  the donor. The native roster has seven spare slots (15–21). ReRiff combines
  car configurations and assigns runtime IDs `r00`–`r06`; those IDs are not
  permanent package identities.

Customs defaults new exports to `overwrite=1`, matching its replacement workflow,
with a generated stable package ID. Change it to `0` to add the car. Re-exporting
an existing package preserves its `origin.txt`, including its policy and ID.
Legacy files without `overwrite` retain their authored layout; they do not opt
into automatic allocation.

## Registry, saves and removal

ReRiff maintains `mods/car-slots.ini` with each identity, authored ID, donor,
current slot, runtime ID and active status. Treat it as generated data rather
than an editable slot assignment file. Removed records remain as history, but
**their slots are reused**. A checked cache is built in `mods/` and mounted by the
existing native loader; source mod assets remain unchanged. Disabled and hidden
folders, including `mods/.disabled`, are ignored.

Each isolated save/checkpoint carries its own checked car-identity metadata.
Moving a slot moves that car's selected paint, ownership, unlocked packs and
usage count. Removed cars' state is retained by identity for reinstalling them.
A new occupant starts with clean state and receives the game's native unlock
rules. A missing selected car falls back to its donor, including on another
ReRiff client without that mod. Stock progression is preserved.

A machine-wide registry cannot prove the layout of an old unlabelled save from
another client. Existing custom progress without portable identity metadata is
therefore refused for automatic migration; it needs an explicit, verified
identity mapping. Do not delete the save or registry to bypass that check.

Shared ghost recordings use the stock donor and its default paint, preserving
compatibility with older/vanilla clients. Custom-model sharing remains deferred.

## Validation limits

The loader uses native configuration, localization and save formats; it does not
expand the game's fixed car or paint capacities. Cars are limited to nine paint
packs. Malformed metadata, conflicting replacements, unsupported configuration
layouts and exhausted slots stop preparation before save access. Changes to a
package's number/order of paints can still require a separate paint migration;
keep paint indices stable across updates.

Native save reading of trailing metadata is supported by static inspection of
its fixed chunk reads. Live round-trip, removal/reinstall and cross-client checks
remain necessary; this document does not claim those tests have passed.
