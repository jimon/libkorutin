# libkorutin

Coroutines are a nice concept, but at the moment there are barely any production-ready C libraries as they suffer from various shortcomings. This is a small proof-of-concept library to avoid the more common problems:

- Extensive multi-platform functionality
- To not change stack location (unlike for example pypy stacklet)
- To not allocate any memory

The heavy lifting in libkorutin is done by different backends, mainly [PyPy stacklet](https://github.com/mozillazg/pypy/tree/master/rpython/translator/c/src/stacklet) and [Boost Context](https://github.com/boostorg/context/tree/develop/src/asm).
However the libraries are not used as is, but instead their assembly-coded stack switch is implemented. Thanks to their devoted developers this project can be realised (supporting all ABI's/compilers is difficult).

TODO:

- Extensive built-in self check suite
- Basic scheduler
- Channels
- Ability to run a coroutine from another coroutine
- More fallback stack switching solution in case if asm fails

### Backend description

- PyPy is used as an ASM based stack pointer switch. This backend is very fast, but do not support arm64
- Boost Context is used as an ASM based stack pointer switch. This backend is also very fast, and supports arm64
- Threads offer low scalability (~50 coroutines is ok), is quite slow, and uses malloc. However if no other work, this is a last resort.
- Emscripten based coroutines, the only way to run it in the browsers, has two flavours `ASYNCIFY` and `EMTERPRETIFY`.
- ucontext, TODO.
- setjmp, TODO.

### Platform support

| OS         | ABI    | Switch | Context | Threads | Emscripten |
| ---------- | ------ | ------ | ------- | ------- | ---------- |
| Windows    | x86    | ✓      | ✓       | ✓       | ✗          |
| Windows    | x64    | ✓      | ✓       | ✓       | ✗          |
| Linux      | x32    | ?      | ?       | ?       | ✗          |
| Linux      | x86    | ?      | ?       | ?       | ✗          |
| Linux      | x64    | ✓      | ✓       | ✓       | ✗          |
| Linux      | armv7  | ✓      | ✓       | ✓       | ✗          |
| Linux      | arm64  | ?      | ?       | ?       | ✗          |
| FreeBSD    | x86    | ?      | ?       | ?       | ✗          |
| FreeBSD    | x64    | ✓      | ✓       | ✓       | ✗          |
| MacOS      | x86    | ✓      | ✓       | ✓       | ✗          |
| MacOS      | x64    | ✓      | ✓       | ✓       | ✗          |
| iOS        | sim64  | ✓      | ✓       | ✓       | ✗          |
| iOS        | armv7  | ✗      | ✓       | ✓       | ✗          |
| iOS        | arm64  | ✗      | ✓       | ✓       | ✗          |
| Android    | x86    | ?      | ?       | ?       | ✗          |
| Android    | x64    | ?      | ?       | ?       | ✗          |
| Android    | armv7  | ?      | ✓       | ?       | ✗          |
| Android    | arm64  | ?      | ?       | ?       | ✗          |
| Emscripten | asm.js | ✗      | ✗       | ✗       | ✓          |
| Emscripten | wasm   | ✗      | ✗       | ✗       | ?          |

### Performance

- Value means switches per second.
- (X) means amount of coroutines yielded/scheduled.
- iOS test app apparently was in debug.

| OS               | CPU         | ABI    | Switch(1024) | Switch(16384) | Context(1024) | Context(16384) | Threads(32) | Threads(64) | Threads(128) |
| ---------------- | ----------- | ------ | ------------ | ------------- | ------------- | -------------- | ----------- | ------------| ------------ |
| Windows 10 x64   | i3770       | x86    | 12337349     | 10625162      | 18056174      | 12268213       | 176795      | 94395       | 45812        |
| Windows 10 x64   | i3770       | x64    | 20897959     | 9875828       | 17231499      | 8445864        | 205128      | 87551       | 47513        |
| MacOS 10.13.2    | i4980hq     | x86    | 11292405     | 8162099       | 16765164      | 12252028       | 46093       | 16470       | 7181         |
| MacOS 10.13.2    | i4980hq     | x64    | 22218395     | 15899470      | 17349371      | 12899821       | 46947       | 16641       | 7334         |
| iOS 9.2 (deb)    | A7          | armv7  | -            | -             | 3497542       | 2543724        | 9604        | 2890        | 806          |
| iOS 11.2.2 (deb) | A9          | arm64  | -            | -             | 6145022       | 4814864        | 17764       | 6353        | 1930         |
| Android 6.0.1    | MSM8974-AC  | armv7  | -            | -             | 2633119       | 2018506        | ?           | ?           | ?            |
| FreeBSD 11.1     | E5-2630L v2 | x64    | 16254737     | 9077585       | 11294780      | 6821553        | 29798       | 9772        | 4436         |
| Ubuntu 17.10     | E5-2650L v3 | x64    | 14368457     | 11775094      | 13073079      | 10392292       | 49764       | 15540       | 5341         |
| Raspbian         | BCM2836     | armv7  | 1019024      | ?             | 428089        | ?              | 31461       | ?           | ?            |

And some results for Emscripten:

| Browser          | Flavour     | Emscripten(1024) | Emscripten(16384) |
| ---------------- | ----------- | ---------------- | ----------------- |
| Chrome 63        | ASYNCIFY    | 9170502          | 6879678           |
