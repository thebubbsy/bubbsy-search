# Bundled Everything payload

This directory is where a redistributable copy of voidtools **Everything** is staged at packaging
time so the installer can offer to install it for users who don't already have it.

**Nothing here is committed to git** (see the repo `.gitignore`), and Everything is **not** covered by
this project's GPL license — it is proprietary software under voidtools' own terms. Obtain it from
https://www.voidtools.com/ and comply with its redistribution terms before shipping an installer that
includes it.

Expected contents when staged:
```
Everything.exe        (or Everything64.exe)
<supporting DLLs, if any>
```
