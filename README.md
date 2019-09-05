## Build instructions

The _media library_ depends on _libvlc_, so we must build and install _libvlc_
first.

### Libvlc

To build _libvlc_ in some build directory (let's call it `x`), and install it in
a subdirectory `install/`:

```bash
cd ~/projects/vlc   # the VLC git repository
mkdir -p x/install
cd x
../configure --prefix="$PWD"/install
make
make install
```

If everything is ok then _libvlc_ is built and installed in
`~/projects/vlc/x/install`.


### Media library

First, we must set `PKG_CONFIG_PATH` to point to the _libvlc_ `pkgconfig`:

```bash
export PKG_CONFIG_PATH=~/projects/vlc/x/install/lib/pkgconfig
```

We can now build the _media library_:

```bash
cd ~/projects/medialibrary
mkdir x
cd x
../configure
make
```

#### Tests

To build with the tests:

```bash
cd ~/projects/medialibrary
mkdir x
cd x
../configure --enable-tests
make check
```

To run the tests:

```bash
./unittest
./samples
```
