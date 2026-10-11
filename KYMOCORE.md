# KymoCore dependency

Library development lives in the separate `../KymoCore` repository.
This project consumes official KymoCore **6.3.1**, tag `v6.3.1`, through the
Git submodule `lib/KymoCore`. PlatformIO links that pinned consumer checkout;
it does not link the sibling development directory.

Fresh checkout:

```sh
git submodule update --init --recursive
```

Update to a later published release:

```sh
git -C lib/KymoCore fetch origin --tags
git -C lib/KymoCore switch --detach vX.Y.Z
git add lib/KymoCore
git commit -m "Update KymoCore to X.Y.Z"
```

Do not make library edits inside the submodule. Make and test them in
`../KymoCore`, publish the release, and then update this pin.
Standalone PlatformIO projects can instead install the exact registry version:

```ini
lib_deps = codename666/KymoCore @ 6.3.1
```
