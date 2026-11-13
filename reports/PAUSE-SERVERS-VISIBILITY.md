# Servers visibility in race-pause Settings

Original reference: Steam depot 351991/1687655 `Game.exe`, SHA-256
`2d1af8a01ec6b1e3477ff6dbe4a44895828e2ac71449344fec9d7d99e87a58e2`.
The local recovery project was consulted; its native recovery notes do not yet
cover this managed Settings initializer.

The original `gaOptionsMenu.initScreens`, metadata token `0x06001195`, has
CIL body RVA `0x2EF354` (2739 code bytes). At IL_00E6–IL_0106 it checks
`gaStateManager` fields +60 >= 0 and +64 < 1. This determines the entries:
IL_011B–IL_0145 includes row IDs 0 and 3 only for that front-end state;
rows 1 and 2 are retained. The row creation loop begins at IL_01A2 and
skips IDs equal to -1. ID 0's text pointer is stored at menu +272.

ReRiff now uses that existing optional Graphics row, after original init,
to decide whether to add Servers. When +272 is null, it creates no server
controls or root reel and leaves the page outside its server-page ownership.
This is an intentional ReRiff visibility change, not original reconstruction
or a claim of matching-byte recovery. The original executable is unchanged.

Regression coverage exercises main-menu insertion and a pause-style layout
with Graphics absent, checking no new controls/reel, original ticking and
cleanup. An actual in-race visual retest remains useful.
