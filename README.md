# libkorutin

Coroutines are a nice concept, but there are barely any C libraries that could be widely used in production at the moment. Libraries that available have random shortcomings here and there.

This is a small proof-of-concept library to try to avoid common shortcomings:

- It should work on all popular platforms.
- It doesn’t change stack locations (unlike pypy stacklet).
- It doesn’t allocate any memory.

The heavy lifting in libkorutin is done by stack switching code from pypy's stacklet, which seems to be the most complete asm routine library on a market.

TODO:

- Extensive built-in self check suite.
- Basic scheduler.
- Channels.
- Ability to run a coroutine from another coroutine.
- Stack switching for emscripten/js.
- More fallback stack switching solution in case if asm fails.
- Investigate using boost context asm backend.

### Backends

- PyPy stacklet switch, asm based, provides stack pointer switch. Fastest backend, but depends on ABI.
- Threads. Slow and doesn't scale (~50 coroutines is ok), but should work if other options fail.
- boost context, TODO.
- ucontext, TODO.
- setjmp, TODO.
- emscripten, TODO.

### Platform support

| OS         | ABI    | Switch | Threads |
| ---------- | ------ | ------ | ------- |
| Windows    | x86    | ✓      | ✓       |
| Windows    | x64    | ✓      | ✓       |
| Linux      | x86    | ?      | ?       |
| Linux      | x64    | ?      | ?       |
| Linux      | armv7  | ?      | ?       |
| Linux      | arm64  | ?      | ?       |
| FreeBSD    | x86    | ?      | ?       |
| FreeBSD    | x64    | ?      | ?       |
| MacOS      | x86    | ?      | ?       |
| MacOS      | x64    | ?      | ?       |
| iOS        | armv7  | ?      | ?       |
| iOS        | arm64  | ?      | ?       |
| Android    | x86    | ?      | ?       |
| Android    | x64    | ?      | ?       |
| Android    | armv7  | ?      | ?       |
| Android    | arm64  | ?      | ?       |
| Emscripten | asm.js | ✗      | ✗       |
| Emscripten | wasm   | ✗      | ✗       |

### Performance

- Value means switches per second.
- (X) means amount of coroutines yielded/scheduled.

| OS             | CPU   | ABI    | Switch (1024) | Switch(16384) | Threads (32) | Threads (64) | Threads (128) |
| -------------- | ----- | ------ | ------------- | ------------- | ------------ | ------------ | ------------- |
| Windows 10 x64 | i3770 | x86    | 12337349      | 10625162      | 176795       | 94395        | 45812         |
| Windows 10 x64 | i3770 | x64    | 20897959      | 9875828       | 205128       | 87551        | 47513         |
