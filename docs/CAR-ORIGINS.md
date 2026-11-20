# Car donors and missing-car fallback

Place `origin.txt` beside `assets/` in a loose car mod folder:

```ini
[mod]
id=my-unique-car-package
overwrite=0

[origins]
dod=alf

[fallbacks]
dod=fab
```

Each entry in `[origins]` is **authored car ID = stock donor ID**. `alf` is
Pulse and `fab` is Photon. Use the package's authored ID on the left. IDs are
three lowercase ASCII letters or digits. Keep the package `id` stable across
updates and renames; when omitted, the folder name becomes the identity.

`[fallbacks]` is optional and uses the same authored IDs. It names the stock
car shown when the custom car's slot is unavailable, for example after its mod
is disabled or removed. It is independent of the donor: the donor still
supplies copied assets, configuration, save identity and shared-ghost identity.
Without an entry, or if the declared car is not a valid loaded stock car, the
donor is used. Conflicting fallbacks for one authored ID are refused.

ReRiff reads these files when the main menu initializes and records the result
in `car-origins.ini` beside `client.ini`:

```ini
[slot_15]
base=alf
fallback=fab
```

Treat that file as generated data. It is retained when the mod is removed so
the original setter can select the fallback; reusing the slot for another
active package replaces the record. Unknown or malformed mappings are refused
rather than guessed.
