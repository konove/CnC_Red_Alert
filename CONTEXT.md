# C&C Red Alert / Tiberian Dawn port

Two 1990s Westwood games, Red Alert and Tiberian Dawn, built on one shared engine and run on SDL2.

## Language

### Code ownership

**Engine**: Code both games share, which knows nothing of either game. Lives under `src/engine/`.\
_Avoid_: tech, sdllib, port, library (as a name for the whole)

**Game**: One of the two programs, Red Alert (`ra`) or Tiberian Dawn (`td`), built on the engine.\
_Avoid_: client, app

**Engine folder**: One domain of the engine (`gfx`, `file`, `window`, ...) with a fixed place in the
dependency order. It may use only the folders below it.\
_Avoid_: layer, module, component

### Screen output

**Display**: The program's one window: what it shows and the palette it shows it through.\
_Avoid_: screen, window surface

**Page**: An 8-bit paletted pixel buffer that drawing targets. Most pages exist only in memory.\
_Avoid_: buffer, surface, GraphicBuffer

**Window page**: The one page whose pixels are the Display's, so that drawing on it shows up on
screen.\
_Avoid_: visible page, seen page, front buffer

**Text window**: A named rectangle of a page that text and stamps are clipped to (the legacy
`WindowList`).\
_Avoid_: window (unqualified), viewport
