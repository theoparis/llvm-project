add_startup_object(
  start
  SRC
    start.cpp
  DEPENDS
    libc.config.app_h
  COMPILE_OPTIONS
    -fno-omit-frame-pointer
    -ffreestanding
)

add_startup_object(
  start_standalone
  SRC
    start_standalone.S
  COMPILE_OPTIONS
    -ffreestanding
)
