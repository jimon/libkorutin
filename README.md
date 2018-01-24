# libkorutin

Coroutines are a nice concept, but there are barely any C libraries that could be widely used in production at the moment. Libraries that available have random shortcomings here and there.

This is a small proof-of-concept library to try to avoid common shortcomings:

- It should work on all popular platforms.
- It doesn’t change stack locations (unlike pypy stacklet).
- It doesn’t allocate any memory.

The heavy lifting in libkorutin is done by stack switching code from pypy's stacklet, which seems to be the most complete asm routine library on a market.

TODO:

- Basic scheduler.
- Channels.
- Ability to run a coroutine from another coroutine.
- Stack switching for emscripten/js.
- Fallback stack switching solution in case if asm fails.
