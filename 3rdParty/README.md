# Third-party dependencies

## QWindowKit

QWindowKit is distributed as two prebuilt archives managed by Git LFS:

- `artifacts/qwindowkit-1.5.1-qt6.8.3-msvc-x64-debug.zip`
- `artifacts/qwindowkit-1.5.1-qt6.8.3-msvc-x64-release.zip`

The exact upstream commit, build environment and SHA-256 checksums are recorded in
`qwindowkit-manifest.json`. The Apache-2.0 license is included in each archive and in
`QWindowKit-LICENSE.txt`.

### Prepare a checkout

From the repository root:

```powershell
git lfs pull
pwsh -File .\scripts\Prepare-QWindowKit.ps1
```

Use `-Force` to replace an existing local installation. The script verifies each archive before
extracting it to `3rdParty/debug/qwindowkit` and `3rdParty/release/qwindowkit`. These extracted
directories are local generated dependencies and are intentionally ignored by Git.

### Rebuild the archives

Open an x64 Visual Studio Developer PowerShell, ensure CMake and Ninja are available on `PATH`,
and set `QT_ROOT` to the Qt 6.8.3 MSVC x64 kit. Then run:

```powershell
pwsh -File .\scripts\Build-QWindowKit.ps1 -QtRoot $env:QT_ROOT
```

If CMake or Ninja is not on `PATH`, pass their executable paths with `-CMakeCommand` and
`-NinjaCommand`.

The build script checks out the pinned upstream commit, builds Debug and Release shared libraries,
includes Debug PDB files, recreates both archives and updates their checksums in the manifest.
