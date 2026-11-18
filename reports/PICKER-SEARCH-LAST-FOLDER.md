# Search selection and post-race picker folder

Original reference: Steam depot 351991/build 1687655 Game.exe, SHA-256
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
Consulted the RiffRacerDecomp recovery tree; no gaFileSelectInterface source
was found there. Evidence below comes from the existing original picker CIL
export in build/picker-alphabet/original-picker.il and original module export.
These are behavioral findings, not a matching-source reconstruction.

- gaFileSelectInterface.enter (token 0x06001049, RVA 0x2d6948) tests state 37
  and reads gaSaveGame.getLastFolderAccessed before directory setup.
- gaFileSelectInterface.handleScreenInput (token 0x0600104e, RVA 0x2d5e60)
  builds the current folder, converts it with getSystemShortFilename, removes
  its trailing separator and calls gaSaveGame.setLastFolderAccessed at IL_0703
  before invoking onFileSelected at IL_076b.
- gaSaveGame.setLastFolderAccessed (RVA 0x2f85ac, 45 CIL code bytes) assigns
  the narrow string into save object +360, calls saveUserLibraryInfo, then
  destroys the passed by-value string. Its fault handler also destroys it.
- onFileSelected (token 0x0600104f, RVA 0x2cebd4) selects music and changes
  overlay/state; it does not save the browsing folder itself.

ReRiff search directly invoked onFileSelected and omitted the folder setter.
The user reports a blank picker after choosing a search result, racing, and
pressing New Track on the results screen. The proposed ReRiff correction uses
original short-path conversion and the original consuming folder setter with
the selected result's parent, before the original selection callback.
A bridge regression checks the remembered parent and string-allocation balance.
Full post-race live validation remains pending; this evidence does not establish
that the omission explains every possible blank-picker case.
