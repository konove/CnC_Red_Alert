# Plan: enable `cppcoreguidelines-pro-type-cstyle-cast`

Written 2026-09-14 against `.clang-tidy` and clang-tidy 23.1.2. Companion to the row in
[CLANG_TIDY_PRIORITIES.md](CLANG_TIDY_PRIORITIES.md).

**Status: complete (2026-09-14).** The check is enabled; the results, including two defects this
plan did not list (the RA team-editor overflow and the signed-char PCX palette read), are in the
"C-style cast enablement" review in CLANG_TIDY_PRIORITIES.md.

## Context

`cppcoreguidelines-pro-type-cstyle-cast` reports the C-style casts that a named cast would spell as
`reinterpret_cast`, `const_cast` or a `static_cast` downcast — the type-unsafe subset of all C-style
casts. It is the direct continuation of `clang-diagnostic-cast-qual` (enabled 2026-09-14, eleven
commits, four real bugs): that work removed the `const` subset, and what is left is pointer punning
and unchecked downcasts. `CLAUDE.md` already says "Do not use C-style casts".

The row stayed skipped because `docs/TYPE_MIGRATION.md` rules out a codebase-wide cast hunt. That
rule is about the _numeric_ casts (`(int)x`) inside type-migration commits and still stands:
`modernize-avoid-c-style-cast` and `google-readability-casting` (1,393 sites, mostly numeric) remain
excluded and are out of scope here. `clang-diagnostic-old-style-cast` also stays as it is (the clang
flag set passes `-Wno-old-style-cast`; GCC's strict set already has `-Wold-style-cast`).

A fresh isolated sweep on 2026-09-14 (all 931 compile-database entries, 475 unique translation
units) reports **415 unique sites**, down from the 866 recorded on 2026-09-12:

| Message                                                            | Sites |
| ------------------------------------------------------------------ | ----- |
| do not use C-style cast to convert between unrelated types         | 327   |
| do not use C-style cast to downcast from a base to a derived class | 88    |

| Module | Sites |
| ------ | ----- |
| ra     | 302   |
| td     | 84    |
| tech   | 25    |
| sdllib | 4     |

Largest files: `ra/wolapiob.cc` 57, `ra/rawolapi.cc` 49, `ra/inline.h` 31, `ra/wol_gsup.cc` 21,
`ra/queue.cc` 21, `tech/mp.cc` 15, `ra/target.h` 14, `td/inline.h` 12, `ra/compat.h` 12.

## How the check classifies (read before fixing)

- clang's AST marks every pointer-to-pointer C-style cast `CK_BitCast`, **including `void*` →
  `T*`**, so `(User*)Get_Item_ExtraDataPtr(i)` is reported as "unrelated types" although
  `static_cast` is the honest spelling. About 25 sites are of this kind.
- A downcast through a _forward-declared_ type (`ra/target.h`, `ra/radio.h`, `td/radio.h`) is also
  reported as "unrelated" because the relationship is invisible at that point.
- `WarningsAsErrors: '*'` applies to the isolated sweep too: findings print as `error:` lines, not
  `warning:`. Grep for `error:` or the count reads as zero.
- The check has **no fix-its**. Every edit is by hand.
- Three sibling checks are already enforced and shape every fix:
  `cppcoreguidelines-pro-type-const-cast`, `cppcoreguidelines-pro-type-static-cast-downcast` (no
  suppression anywhere in the tree) and `cppcoreguidelines-pro-type-reinterpret-cast` (ten reasoned
  `NOLINTNEXTLINE`s, e.g. `port/socket_bytes.h`, `tech/mp.cc`). Consequences: downcasts become
  `dynamic_cast` (the cast-qual precedent, 119 sites); byte views go through one helper that carries
  the single documented `reinterpret_cast`; no call site gets a bare `reinterpret_cast`.

## Defects visible in the sweep list (verified)

- `ra/aircraft.cc:1711,2024,3738` — `building = (BuildingClass*)ship;` stores a `VesselClass*` (a
  carrier) in a `BuildingClass*`. It works only because the code after it uses `TechnoClass` members
  (`Transmit_Message`, `What_Am_I`, `As_Target`). The local becomes a `TechnoClass* dock`.
- `td/map.cc:1370-1383` — `(uintptr_t)obj & 0xff000000` on the occupier, its `Next` and its
  `Trigger`: DOS-era pointer-range heuristics in the map validator. On a 64-bit build every heap
  pointer trips them. Delete the three pointer tests in each block; keep `IsInLimbo` and the
  cell-range test. Related: commit `2b627a20` (Fail TD object validation loudly).
- `td/monoc.cc:76` — `MonoSegment = (void*)0x000b0000`, and `td/monoc.h:162` `memcpy`s into it.
  `MonoClass::Enable()` is reachable from `td/init.cc:2665` and `td/debug.cc:335`. RA moved its mono
  pages into memory in the `FixedAddressDereference` commit; TD gets the same in-memory pages.
- `ra/cargo.cc:155,159` — `Attached_Object()` already returns `FootClass*`; the local is typed
  `TechnoClass*` and cast back. Type the local correctly; no cast.
- `ra/sendfile.cc:160,397` — `(int*)&packet_len` aliases an `unsigned int` as `int`. Make it `int`.

## Fix strategy by group

Site counts are from the sweep; a line with two casts counts twice.

| Group                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           | Sites | Fix                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               |
| --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ----- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **A. `void*` → `T*`**: `Get_Item_ExtraDataPtr` (14, WOL lists), `Get_Buffer`/`Get_Offset`/`Get_Image_Data` (6), `(IUnknown*)(*ppv)` (3), `(Channel*)`, `(WOL_GAMETYPEINFO*)`                                                                                                                                                                                                                                                                                                                                    | ~25   | `static_cast<T*>`. Where an accessor always yields one type (the WOL player and channel lists hold `User*`/`Channel*`), a typed wrapper beats casting at every use.                                                                                                                                                                                                                                                                                                                                                               |
| **B. WOL generated structs with `unsigned char[]` text fields**: `User::name`, `Channel::name/key`, `Server::login/password/connlabel/conndata`, `Update::localpath/server/login/password/patchfile`, `Ladder::login_name` in `rawolapi.cc`, `wolapiob.cc`, `wol_gsup.cc`, `wol_chat.cc`                                                                                                                                                                                                                        | ~110  | One helper pair `WolText(unsigned char*)` / `WolText(const unsigned char*)` in `ra/rawolapi.h`, each with a single documented `NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)` (the `port::SocketBytes` pattern). `iChannelLobbyNumber` takes `const char*`; its six callers cast today. `rawolapi.cc:180,195,210` also `strtok` the const struct in place — record as a follow-up, do not widen scope.                                                                                                              |
| **C. Byte views of objects for I/O**: `(char*)&ackpacket` into `Send` (4), `Write_To_Serial_Port((unsigned char*)…)` (4), `Set_Pal((char*)&CCPalette)` (3) and the radar fading tables (5), `Read_PCX_File(…, (char*)palette)` (2), `IPX_*_Packet95` (2), `DEREncode`, `compat.h` `(unsigned char*)this + Offset` (12), `palette.cc` (2), `lcw.cc`/`lcwuncmp.cc` `(unsigned char*)dest` (4), `2keyfbuf.cc:63`, `sha.cc:99`, `misc.cc` `&RandNumb`, `mp.cc:407,456`, `writepcx.cc`/`pcx_file.cc` `(RGB*)palcopy` | ~50   | Add `port::BytesOf(T&)` / `BytesOf(const T&)` in `port/bytes_of.h` returning `unsigned char*` / `const unsigned char*`: constrained to trivially copyable `T`, one NOLINT `reinterpret_cast`, `ABSL_ATTRIBUTE_LIFETIME_BOUND`, tests in `port/bytes_of_test.cc`. Where the callee is a genuine byte sink or source (`Send`, `Write_To_Serial_Port`, `Set_Pal`, the `Read_PCX_File` palette, `IPX_*_Packet95`) change its parameter to `const void*` / `void*` so the call needs no cast at all — the file-I/O refactor precedent. |
| **D. Unaligned typed access into byte buffers**: `*(uint16_t*)(Buf + N)` CRC and length words in `td/conquer.cc` (4), `td/netdlg.cc` (8), `td/nulldlg.cc` (8); `sha.cc:237`; `ipxgconn.cc` `*(IPXAddressClass*)ExtraBuffer`; `ipxmgr.cc` `(IPXHEADER*)temp_receive_buffer` and `temp_address` (4); `mapedtm.cc` carving `SysMemPage` (2); `jshell.h` bit helpers (3); `search.h` comparator (4); `nulldlg.cc` `(EventClass*)&ReceivePacket` (2); `wspudp.cc:223` `(uint32_t**)h_addr_list`                      | ~40   | The existing `port::ReadUnaligned<T>` / `WriteUnaligned` (`port/unaligned.h`) for words; `port::AlignedObject<T>` (`port/aligned_buffer.h`, already used by `connect.cc` and `ipxgconn.cc`) for packet structs; the `jshell.h` helpers take `const uint32_t*`; `search.h` uses `static_cast<const int*>` from its `const void*` parameters; `mapedtm.cc` gets two typed locals.                                                                                                                                                   |
| **E. Packed coordinate, cell and target words punned through `*_COMPOSITE` union references or byte pointers**: `ra/inline.h` (31), `ra/face.h` (4), `ra/target.h:62`, `td/inline.h` (12)                                                                                                                                                                                                                                                                                                                       | 48    | RA: `std::bit_cast<COORD_COMPOSITE>(coord)` and friends for reads; the writers (`ra/inline.h:467-512`) build a composite, set the field and `bit_cast` back (eight `bit_cast` uses already in the tree). TD: shifts and masks on the `uint32_t`, as its neighbouring `Cell_X`/`Cell_Y` already do; `Dir_Diff` becomes `static_cast<int8_t>(dir2) - static_cast<int8_t>(dir1)`. Both smoke scripts (object positions, game states) are the regression net.                                                                         |
| **F. Object-model downcasts after an RTTI check**: `mapedit`/`mapedsel`/`mapedplc` (RA 29, TD 24), `ra/queue.cc` CRC dump (22), `techno.cc` type-class casts (RA 7, TD 4), `ra/cell.cc` (3), `debug.cc` (RA 3, TD 2), `house.cc`, `reinf.cc`, `mapedit.cc:1147` `(ListClass*)Extract_Gadget`, and the incomplete-type downcasts `ra/target.h` `As_*` (10) and `radio.h` `Contact_With_Whom` (RA, TD)                                                                                                            | ~100  | `dynamic_cast`, matching the cast-qual precedent, hoisted into one local per block so each block casts once. `target.h` and `radio.h` bodies move out of line into `target.cc` / `radio.cc`, where the types are complete. `queue.cc` uses `dynamic_cast<AircraftClass&>(*objp)` and so on. Do **not** weaken to `static_cast`: `pro-type-static-cast-downcast` is enforced.                                                                                                                                                      |
| **G. Platform idioms**: sockets `(sockaddr*)&addr` (6), registry `(LPBYTE)` (9), COM `(void**)&p` for `QueryInterface`/`CoCreateInstance` (8), `2keyfbuf.cc:173` `(uintptr_t)ptr`                                                                                                                                                                                                                                                                                                                               | ~24   | `port::SocketAddress(sockaddr_in&)` next to `SocketBytes` in `port/socket_bytes.h` (one NOLINT, test); registry values through `port::BytesOf(dwValue)`; a `ComOut(T**) -> void**` helper in the WOL header (one NOLINT); `std::bit_cast<uintptr_t>`.                                                                                                                                                                                                                                                                             |
| **H. `tech/mp.cc` half-word views over `digit` arrays** in the hybrid multiply                                                                                                                                                                                                                                                                                                                                                                                                                                  | 15    | One file-local `XMP_Halves(digit*)` with a const overload, carrying the same documented NOLINT as the file's two existing `reinterpret_cast`s; 15 sites become one. The strict-aliasing status is unchanged (it already is what it is); record as a follow-up.                                                                                                                                                                                                                                                                    |

## Steps

Each step is one or more commits; each commit passes the strict build, CTest and both smoke scripts.

1. **Helpers.** `port/bytes_of.h` + `port/bytes_of_test.cc`; `port::SocketAddress` in
   `port/socket_bytes.h` + test; `WolText` and `ComOut` in `ra/rawolapi.h`. No call-site change.
2. **Bugs first.** The aircraft dock local; the `td/map.cc` pointer heuristics; TD mono pages in
   memory; the cargo local type; `sendfile.cc` `packet_len`.
3. **Coordinates (E).** RA via `bit_cast`, TD via shifts. The smoke scripts must report identical
   positions and states.
4. **Byte views and byte-sink signatures (C, D).** Change the `Send`, `Write_To_Serial_Port`,
   `Set_Pal`, `Read_PCX_File` and `IPX_*` parameters one class family per commit; then `BytesOf`,
   `ReadUnaligned`/`WriteUnaligned`, `AlignedObject` at the remaining sites.
5. **WOL strings, COM and registry (B, A, G).**
6. **Downcasts (F).** The map-editor family; `queue.cc`; `techno.cc`; `target.h` and `radio.h` out
   of line; the rest.
7. **`mp.cc` halves (H).**
8. **Enable.** Remove `-cppcoreguidelines-pro-type-cstyle-cast` from `.clang-tidy`; full strict
   rebuild; set the priorities row to **Enabled** and add a "C-style cast enablement (date)" review
   section with the groups, counts and bugs found; state that the numeric-cast checks stay skipped.

## Verification

- **Isolated sweep** (~2 min), per the tree-wide fix-it recipe: the unique `src/` files from
  `cmake-build-strict-ra-clang/compile_commands.json`, then

  ```sh
  xargs -P 10 -I{} sh -c 'clang-tidy -p cmake-build-strict-ra-clang --quiet \
      --checks="-*,cppcoreguidelines-pro-type-cstyle-cast" "{}" 2>/dev/null' < tus.txt > report.txt
  grep -c 'error:.*cppcoreguidelines-pro-type-cstyle-cast' report.txt   # target: 0
  ```

  Dedupe on `file:line:col` before counting; header findings repeat per including unit.

- **Knock-on sweeps** before the strict build, since the fixes cascade into other enforced checks:
  `cppcoreguidelines-pro-type-reinterpret-cast`, `cppcoreguidelines-pro-type-static-cast-downcast`,
  `misc-const-correctness`, `readability-make-member-function-const`, `modernize-use-nodiscard`,
  `misc-include-cleaner` (new `<bit>` and `port/...` includes).
- **Full strict rebuild** in foreground chunks, without reconfiguring the strict directory:
  `timeout 590 cmake --build cmake-build-strict-ra-clang --parallel 10 -- -k 0`, repeated until
  `ninja: no work to do`.
- **Tests:** CTest over the strict (or debug) build directory, then `tools/ra_saveload_smoke.sh` and
  `tools/td_saveload_smoke.sh` including the `--team` and `--building` fixtures. Step 3
  (coordinates) and step 4 (packet CRC and length words) are what these catch.
- **Manual:** TD's mono display after `MonoClass::Enable()`; the network and modem dialogs stay a
  manual check, as they were for cast-qual.
- **Probe:** a one-line C-style downcast in a scratch file must still be reported once the check is
  on, so zero findings are not mistaken for a disabled check.

## Risks

- `dynamic_cast` in hot paths (`As_Unit`, the `techno.cc` target evaluation). The cast-qual work
  already accepted it; hoisting to one cast per block keeps it to one RTTI walk. Profile if a frame
  budget shows it.
- Byte-sink signature changes: `Send` has 15 call sites across both games and virtual overrides in
  the connection classes. One class family per commit.
- TD coordinate rewrites are bit-exact or wrong; the smoke diff is the gate, and a unit test for
  `Coord_Snap`, `Coord_Mid`, `Coord_XCell`/`YCell` and `Dir_Diff` against the old byte-poke results
  is cheap insurance.
- Follow-ups deliberately left behind: the `mp.cc` half-word aliasing, and the WOL `strtok` calls
  that write into a struct received as `const`.
