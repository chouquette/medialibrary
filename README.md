## Dependencies

There are 2 modes for building the medialibrary. With libvlc dependency, and without.

Usually, unless you're building for integration with VLC desktop, you'll want to enable the libvlc integration. Please note that libvlc is required to build and run the tests.

If you want to disable libvlc, you can do so by passing `-Dlibvlc=disabled` to meson when configuring the build.

### Libraries

* libvlc
* libvlcpp (available as a git submodule)
* libsqlite3 (>= 3.33.0)
* libjpeg (only when using libvlc 3)
* rapidjson when building the functional tests

### Tools

* meson
* ninja

## Build instructions

If some of your dependencies are not installed in a default location, you'll need to point meson to that folder by using `PKG_CONFIG_PATH` and point it to the folder that contains your .pc files

The minimal command to setup the build is:
```bash
cd /path/to/medialibrary
PKG_CONFIG_PATH=/path/to/custom/lib/pkconfig meson build-folder
```
You can omit the `PKG_CONFIG_PATH` if all your dependencies are installed in the standard/default locations.

Once configured, building is just a matter of invoking ninja
```bash
cd build-folder
ninja
```

## Tests

If the libvlc dependency was found, then the tests will be build automatically when required.

You can launch them using either

```bash
ninja test
```
or
```bash
meson test
```

If you only want to run the unit tests, you can do so by running
```bash
meson test --suite unittest
```
and similarly for functional tests:
```bash
meson test --suite functional
```

You can find more options on how to tweak the test runs in the [meson tests documention](https://mesonbuild.com/Unit-tests.html#unit-tests)
