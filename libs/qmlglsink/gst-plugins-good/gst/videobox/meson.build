orcsrc = 'gstvideoboxorc'
if have_orcc
  orc_h = custom_target(orcsrc + '.h',
    input : orcsrc + '.orc',
    output : orcsrc + '.h',
    command : orcc_args + ['--header', '-o', '@OUTPUT@', '@INPUT@'])
  orc_c = custom_target(orcsrc + '.c',
    input : orcsrc + '.orc',
    output : orcsrc + '.c',
    command : orcc_args + ['--implementation', '-o', '@OUTPUT@', '@INPUT@'])
else
  orc_h = configure_file(input : orcsrc + '-dist.h',
    output : orcsrc + '.h',
    copy : true)
  orc_c = configure_file(input : orcsrc + '-dist.c',
    output : orcsrc + '.c',
    copy : true)
endif

gstvideobox = library('gstvideobox',
  'gstvideobox.c', orc_c, orc_h,
  c_args : gst_plugins_good_args,
  include_directories : [configinc],
  dependencies : [orc_dep, gstbase_dep, gstvideo_dep],
  install : true,
  install_dir : plugins_install_dir,
)
pkgconfig.generate(gstvideobox, install_dir : plugins_pkgconfig_install_dir)
plugins += [gstvideobox]
