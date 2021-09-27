# Windows specific

configure_file(
  input : 'gimp-plug-ins.rc.in',
  output: 'gimp-plug-ins.rc',
  configuration: versionconfig,
)

configure_file(
  input : 'gimp.rc.in',
  output: 'gimp.rc',
  configuration: versionconfig,
)

# Following rules add the build-time dependency on `git-version.h`
# for `gimp-plug-ins.rc` and `gimp.rc` to ensure proper order of
# creation of files.
# Otherwise meson build on Windows experience race condition when
# resource compiler tries to do its job before `git-version.h`
# is built.

# It's better to make one `fake` custom_target() that will ensure
# proper order of creation of files, rather than to patch 35+ rules
# to add an explicit dependency

# actual command does not matter as long as it exists on all build systems
gimp_plugins_rc = custom_target('git-version.h build-time-dependency for gimp_plugins_rc',
  build_by_default: true,
  build_always_stale: true,
  command: ['git', '--version'],
  depends: [gitversion_h],
  output:  ['gimp-plug-ins.rc']
).full_path()

gimp_app_rc = custom_target('git-version.h build-time-dependency for gimp.rc',
  build_by_default: true,
  build_always_stale: true,
  command: ['git', '--version'],
  depends: [gitversion_h],
  output:  ['gimp.rc']
).full_path()
