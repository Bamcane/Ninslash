cmake_minimum_required(VERSION 3.11)

file(STRINGS src/game/version.h VERSION_LINE
  LIMIT_COUNT 1
  REGEX GAME_RELEASE_VERSION
)

if(VERSION_LINE MATCHES "\"([0-9]+)\\.([0-9]+)\\.([0-9]+)\"")
  set(VERSION_MAJOR ${CMAKE_MATCH_1})
  set(VERSION_MINOR ${CMAKE_MATCH_2})
  set(VERSION_PATCH ${CMAKE_MATCH_3})
elseif(VERSION_LINE MATCHES "\"([0-9]+)\\.([0-9]+)\"")
  set(VERSION_MAJOR ${CMAKE_MATCH_1})
  set(VERSION_MINOR ${CMAKE_MATCH_2})
  set(VERSION_PATCH "0")
else()
  message(FATAL_ERROR "Couldn't parse version from src/game/version.h")
endif()

if(POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
  if(VERSION_PATCH STREQUAL "0")
    project(Ninslash VERSION ${VERSION_MAJOR}.${VERSION_MINOR})
  else()
    project(Ninslash VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
  endif()
else()
  project(Ninslash)
  set(PROJECT_VERSION_MAJOR ${VERSION_MAJOR})
  set(PROJECT_VERSION_MINOR ${VERSION_MINOR})
  set(PROJECT_VERSION_PATCH ${VERSION_PATCH})
  if(VERSION_PATCH STREQUAL "0")
    set(PROJECT_VERSION ${VERSION_MAJOR}.${VERSION_MINOR})
  else()
    set(PROJECT_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
  endif()
endif()

set(ORIGINAL_CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})
set(ORIGINAL_CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES})
set(ORIGINAL_CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
set(OWN_CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_MODULE_PATH ${OWN_CMAKE_MODULE_PATH})

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(TARGET_BITS "64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
  set(TARGET_BITS "32")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(TARGET_OS "windows")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(TARGET_OS "linux")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(TARGET_OS "mac")
endif()

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CheckSymbolExists)

check_symbol_exists(__i386 "" TARGET_ARCH_X86_i386)
if(TARGET_ARCH_X86_i386)
  set(TARGET_ARCH x86)
else()
  set(TARGET_ARCH)
endif()

set(AUTO_DEPENDENCIES_DEFAULT OFF)
if(TARGET_OS STREQUAL "windows")
  set(AUTO_DEPENDENCIES_DEFAULT ON)
endif()

option(CLIENT "Compile client" ON)
option(PREFER_BUNDLED_LIBS "Prefer bundled libraries over system libraries" ${AUTO_DEPENDENCIES_DEFAULT})
option(DEV "Don't generate stuff necessary for packaging" OFF)

# Set the default build type to Release
if(NOT(CMAKE_BUILD_TYPE))
  if(DEV)
    set(CMAKE_BUILD_TYPE Release)
  else()
    set(CMAKE_BUILD_TYPE Debug)
  endif()
endif()

set(DBG $<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  src/game/version.h
)

set(SERVER_EXECUTABLE ninslash_srv CACHE STRING "Name of the built server executable")
set(CLIENT_EXECUTABLE ninslash CACHE STRING "Name of the build client executable")

########################################################################
# Compiler flags
########################################################################

function(add_c_compiler_flag_if_supported VARIABLE FLAG)
  if(ARGC GREATER 2)
    set(CHECKED_FLAG "${ARGV2}")
  else()
    set(CHECKED_FLAG "${FLAG}")
  endif()
  string(REGEX REPLACE "[^A-Za-z0-9]" "_" CONFIG_VARIABLE "FLAG_SUPPORTED${CHECKED_FLAG}")
  check_c_compiler_flag("${CHECKED_FLAG}" ${CONFIG_VARIABLE})
  if(${CONFIG_VARIABLE})
    if(${VARIABLE})
      set("${VARIABLE}" "${${VARIABLE}};${FLAG}" PARENT_SCOPE)
    else()
      set("${VARIABLE}" "${FLAG}" PARENT_SCOPE)
    endif()
  endif()
endfunction()

if(NOT MSVC)
  # Protect the stack pointer.
  # -fstack-protector-all doesn't work on MinGW.
  add_c_compiler_flag_if_supported(OUR_FLAGS -fstack-protector-all)

  # Inaccurate floating point numbers cause problems on mingw-w64-gcc when
  # compiling for x86, might cause problems elsewhere. So don't store floats
  # in registers but keep them at higher accuracy.

  if(TARGET_ARCH STREQUAL "x86")
    add_c_compiler_flag_if_supported(OUR_FLAGS -ffloat-store)
  endif()

  if(TARGET_OS STREQUAL "mac")
    add_c_compiler_flag_if_supported(OUR_FLAGS -stdlib=libc++)
    add_c_compiler_flag_if_supported(OUR_FLAGS -mmacosx-version-min=10.7)
  endif()

  add_c_compiler_flag_if_supported(OUR_FLAGS_OWN -Wall)
  if(CMAKE_VERSION VERSION_GREATER 3.3 OR CMAKE_VERSION VERSION_EQUAL 3.3)
    add_c_compiler_flag_if_supported(OUR_FLAGS_OWN
      $<$<COMPILE_LANGUAGE:C>:-Wdeclaration-after-statement>
      -Wdeclaration-after-statement
    )
  endif()
  add_c_compiler_flag_if_supported(OUR_FLAGS_OWN -Wextra)
  add_c_compiler_flag_if_supported(OUR_FLAGS_OWN -Wno-unused-parameter)
  add_c_compiler_flag_if_supported(OUR_FLAGS_OWN -Wno-missing-field-initializers)
  add_c_compiler_flag_if_supported(OUR_FLAGS_OWN -Wformat=2) # Warn about format strings.
  add_c_compiler_flag_if_supported(OUR_FLAGS_DEP -Wno-implicit-function-declaration)
  add_c_compiler_flag_if_supported(OUR_FLAGS_OWN -Wno-deprecated-copy)
  add_c_compiler_flag_if_supported(OUR_FLAGS_OWN -Wno-ignored-qualifiers)
  add_c_compiler_flag_if_supported(OUR_FLAGS_OWN -Wno-format-nonliteral)
  add_c_compiler_flag_if_supported(OUR_FLAGS_OWN -Wno-return-local-addr)
  add_c_compiler_flag_if_supported(OUR_FLAGS_OWN -Wno-implicit-fallthrough)
  add_c_compiler_flag_if_supported(OUR_FLAGS_OWN -Wno-cast-function-type)
  # TODO: Remove this....
endif()

if(NOT MSVC)
  check_c_compiler_flag("-O2;-Wp,-Werror;-D_FORTIFY_SOURCE=2" DEFINE_FORTIFY_SOURCE) # Some distributions define _FORTIFY_SOURCE by themselves.
endif()

########################################################################
# COMMON FUNCTIONS
########################################################################

function(set_glob VAR GLOBBING EXTS DIRECTORY) # ...
  set(GLOBS)
  foreach(ext ${EXTS})
    list(APPEND GLOBS "${DIRECTORY}/*.${ext}")
  endforeach()
  file(${GLOBBING} GLOB_RESULT ${GLOBS})
  list(SORT GLOB_RESULT)
  set(${VAR} ${GLOB_RESULT} PARENT_SCOPE)
endfunction()

function(set_src VAR GLOBBING DIRECTORY) # ...
  set_glob(${VAR} ${GLOBBING} "c;cpp;h" ${DIRECTORY} ${ARGN})
  set(${VAR} ${${VAR}} PARENT_SCOPE)
endfunction()

########################################################################
# INITIALIZE TARGET LISTS
########################################################################

set(TARGETS_OWN)
set(TARGETS_DEP)

set(TARGETS_LINK) # Targets with a linking stage.

########################################################################
# DEPENDENCIES
########################################################################

function(set_extra_dirs_lib VARIABLE NAME)
  set("PATHS_${VARIABLE}_LIBDIR" PARENT_SCOPE)
  set("HINTS_${VARIABLE}_LIBDIR" PARENT_SCOPE)
  if(PREFER_BUNDLED_LIBS)
    set(TYPE HINTS)
  else()
    set(TYPE PATHS)
  endif()
  if(TARGET_BITS AND TARGET_OS)
    set(DIR "other/${NAME}/${TARGET_OS}/lib${TARGET_BITS}")
    set("${TYPE}_${VARIABLE}_LIBDIR" "${DIR}" PARENT_SCOPE)
    set("EXTRA_${VARIABLE}_LIBDIR" "${DIR}" PARENT_SCOPE)
  endif()
endfunction()

function(set_extra_dirs_include VARIABLE NAME LIBRARY)
  set("PATHS_${VARIABLE}_INCLUDEDIR" PARENT_SCOPE)
  set("HINTS_${VARIABLE}_INCLUDEDIR" PARENT_SCOPE)
  is_bundled(IS_BUNDLED "${LIBRARY}")
  if(IS_BUNDLED)
    set("HINTS_${VARIABLE}_INCLUDEDIR" "other/${NAME}/include" "other/${NAME}/include/${TARGET_OS}" PARENT_SCOPE)
  endif()
endfunction()

if(CMAKE_CROSSCOMPILING)
  set(CROSSCOMPILING_NO_CMAKE_SYSTEM_PATH NO_CMAKE_SYSTEM_PATH)
else()
  set(CROSSCOMPILING_NO_CMAKE_SYSTEM_PATH)
endif()

function(is_bundled VARIABLE PATH)
  if(PATH)
    string(FIND "${PATH}" "${PROJECT_SOURCE_DIR}" LOCAL_PATH_POS)
    if(LOCAL_PATH_POS EQUAL 0 AND TARGET_BITS AND TARGET_OS)
      set("${VARIABLE}" ON PARENT_SCOPE)
    else()
      set("${VARIABLE}" OFF PARENT_SCOPE)
    endif()
  else()
    set("${VARIABLE}" OFF PARENT_SCOPE)
  endif()
endfunction()

if(NOT CMAKE_CROSSCOMPILING)
  # Check for PkgConfig once so all the other `find_package` calls can do it
  # quietly.
  find_package(PkgConfig)
endif()
find_package(ZLIB)
find_package(Freetype)
find_package(Git)
find_package(Pnglite)
find_package(Python3)
find_package(SDL3)
find_package(Threads)
find_package(Wavpack)
find_package(GLEW)

if(TARGET_OS AND TARGET_OS STREQUAL "mac")
  find_program(DMG dmg)
  find_program(HFSPLUS hfsplus)
  find_program(NEWFS_HFS newfs_hfs)
  if(DMG AND HFSPLUS AND NEWFS_HFS)
    set(DMGTOOLS_FOUND ON)
  else()
    set(DMGTOOLS_FOUND OFF)
  endif()

  find_program(HDIUTIL hdiutil)
endif()

message(STATUS "******** Ninslash ********")
message(STATUS "Target OS: ${TARGET_OS} ${TARGET_BITS}bit")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

message(STATUS "Dependencies:")
function(show_dependency_status OUTPUT_NAME NAME)
  if(${NAME}_FOUND)
    if(${NAME}_BUNDLED)
      message(STATUS " * ${OUTPUT_NAME} not found (using bundled version)")
    else()
      message(STATUS " * ${OUTPUT_NAME} found")
    endif()
  else()
    message(STATUS " * ${OUTPUT_NAME} not found")
  endif()
endfunction()

if(TARGET_OS AND TARGET_OS STREQUAL "mac")
  show_dependency_status("Dmg tools" DMGTOOLS)
endif()

if(TARGET_OS AND TARGET_OS STREQUAL "mac")
  show_dependency_status("Hdiutil" HDIUTIL)
endif()

show_dependency_status("Freetype" FREETYPE)
show_dependency_status("Pnglite" PNGLITE)
show_dependency_status("Python3" PYTHON3)
show_dependency_status("SDL3" SDL3)
show_dependency_status("Wavpack" WAVPACK)
show_dependency_status("Zlib" ZLIB)
show_dependency_status("GLEW" GLEW)

if(NOT(PYTHON3_FOUND))
  message(SEND_ERROR "You must install Python3 to compile Ninslash")
endif()

if(CLIENT AND NOT(FREETYPE_FOUND))
  message(SEND_ERROR "You must install Freetype to compile the Ninslash client")
endif()

if(CLIENT AND NOT(SDL3_FOUND))
  message(SEND_ERROR "You must install SDL3 to compile the Ninslash client")
endif()

if(CLIENT AND NOT(GLEW_FOUND))
  message(SEND_ERROR "You must install GLEW to compile the Ninslash client")
endif()

if(TARGET_OS STREQUAL "windows")
  set(PLATFORM_CLIENT)
  set(PLATFORM_CLIENT_LIBS opengl32 glu32 winmm)
  set(PLATFORM_LIBS ws2_32) # Windows sockets
elseif(TARGET_OS STREQUAL "mac")
  find_library(CARBON Carbon)
  find_library(COCOA Cocoa)
  find_library(OPENGL OpenGL)
  set(PLATFORM_CLIENT
    src/osxlaunch/client.h
    src/osxlaunch/client.m
  )
  set(PLATFORM_CLIENT_LIBS ${COCOA} ${OPENGL})
  set(PLATFORM_LIBS ${CARBON})
else()
  set(PLATFORM_CLIENT)
  set(PLATFORM_CLIENT_LIBS GL GLU X11)
  if(TARGET_OS STREQUAL "linux")
    set(PLATFORM_LIBS rt) # clock_gettime for glibc < 2.17
  else()
    set(PLATFORM_LIBS)
  endif()
endif()

########################################################################
# DEPENDENCY COMPILATION
########################################################################

set_src(DEP_JSON_SRC GLOB src/engine/external/json-parser json.c json.h)
add_library(json EXCLUDE_FROM_ALL OBJECT ${DEP_JSON_SRC})

list(APPEND TARGETS_DEP json)
set(DEP_JSON $<TARGET_OBJECTS:json>)

########################################################################
# COPY DATA AND DLLS
########################################################################

file(COPY data DESTINATION .)
set(COPY_FILES
  ${FREETYPE_COPY_FILES}
  ${SDL3_COPY_FILES}
  autoexec.cfg
  storage.cfg
)
file(COPY ${COPY_FILES} DESTINATION .)

########################################################################
# CODE GENERATION
########################################################################

function(chash output_file)
  add_custom_command(OUTPUT ${output_file}
    COMMAND ${Python3_EXECUTABLE} scripts/cmd5.py ${ARGN}
      > "${PROJECT_BINARY_DIR}/${output_file}"
    DEPENDS scripts/cmd5.py ${ARGN}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )
endfunction()

function(generate_source output_file script_parameter)
  add_custom_command(OUTPUT ${output_file}
    COMMAND ${Python3_EXECUTABLE} datasrc/compile.py ${script_parameter}
      > "${PROJECT_BINARY_DIR}/${output_file}"
    DEPENDS
      datasrc/compile.py
      datasrc/content.py
      datasrc/datatypes.py
      datasrc/network.py
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )
endfunction()

file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/src/generated/")
chash("src/generated/nethash.cpp"
  "src/engine/shared/protocol.h"
  "${PROJECT_BINARY_DIR}/src/generated/protocol.h"
  "src/game/tuning.h"
  "src/game/gamecore.cpp"
)
generate_source("src/generated/game_data.cpp" "game_content_source")
generate_source("src/generated/game_data.h" "game_content_header")
generate_source("src/generated/protocol.cpp" "network_source")
generate_source("src/generated/protocol.h" "network_header")


########################################################################
# SHARED
########################################################################

# Sources
set_src(BASE GLOB_RECURSE src/base
  tl/algorithm.h
  tl/allocator.h
  tl/array.h
  tl/base.h
  tl/range.h
  tl/sorted_array.h
  tl/string.h
  tl/threading.h 
  detect.h
  math.h
  system.c
  system.h
  vmath.h
)

set_src(ENGINE_INTERFACE GLOB src/engine
  client.h
  config.h
  console.h
  demo.h
  editor.h
  engine.h
  friends.h
  gamepad.h
  graphics.h
  input.h
  kernel.h
  keys.h
  localization.h
  map.h
  masterserver.h
  message.h
  server.h
  serverbrowser.h
  sound.h
  storage.h
  textrender.h
)

set_src(ENGINE_SHARED GLOB src/engine/shared
  compression.cpp
  compression.h
  config_variables.h
  config.cpp
  config.h
  console.cpp
  console.h
  datafile.cpp
  datafile.h
  demo.cpp
  demo.h
  econ.cpp
  econ.h
  engine.cpp
  filecollection.cpp
  filecollection.h
  huffman.cpp
  huffman.h
  jobs.cpp
  jobs.h
  kernel.cpp
  linereader.cpp
  linereader.h
  map.cpp
  mapchecker.cpp
  mapchecker.h
  mapchunk.cpp
  mapchunk.h
  masterserver.cpp
  memheap.cpp
  memheap.h
  message.h
  netban.cpp
  netban.h
  network_client.cpp
  network_conn.cpp
  network_console_conn.cpp
  network_console.cpp
  network_server.cpp
  network.cpp
  network.h
  packer.cpp
  packer.h
  ringbuffer.cpp
  ringbuffer.h
  snapshot.cpp
  snapshot.h
  storage.cpp
  stream.cpp
  stream.h
)

set(ENGINE_GENERATED_SHARED src/generated/protocol.cpp src/generated/protocol.h)

set_src(GAME_SHARED GLOB src/game
  astar.cpp
  buildables.h
  collision.cpp
  collision.h
  gamecore.cpp
  gamecore.h
  layers.cpp
  layers.h
  localization.cpp
  localization.h
  mapitems.h
  pathfinding.cpp
  pathfinding.h
  questinfo.cpp
  questinfo.h
  tuning.h
  variables.h
  version.h
  voting.h
  weapons.cpp
  weapons.h
)

set(GAME_GENERATED_SHARED
  src/generated/nethash.cpp
  src/generated/protocol.h
  src/generated/game_data.cpp
  src/generated/game_data.h
)

set(DEPS ${DEP_JSON} ${ZLIB_DEP})

# Libraries
set(LIBS
  ${CMAKE_THREAD_LIBS_INIT}
  ${ZLIB_LIBRARIES}
  ${PLATFORM_LIBS}
)

# Targets
add_library(engine-shared EXCLUDE_FROM_ALL OBJECT ${ENGINE_INTERFACE} ${ENGINE_SHARED} ${ENGINE_GENERATED_SHARED} ${BASE})
add_library(game-shared EXCLUDE_FROM_ALL OBJECT ${GAME_SHARED} ${GAME_GENERATED_SHARED})
list(APPEND TARGETS_OWN engine-shared game-shared)

########################################################################
# CLIENT
########################################################################

if(CLIENT)
  # Sources
  set_src(ENGINE_CLIENT GLOB src/engine/client
    backend_sdl.cpp
    backend_sdl.h
    client.cpp
    client.h
    friends.cpp
    friends.h
    gamepad.cpp
    gamepad.h
    glshader.cpp
    glshader.h
    graphics_threaded.cpp
    graphics_threaded.h
    input.cpp
    input.h
    keynames.h
    serverbrowser.cpp
    serverbrowser.h
    sound.cpp
    sound.h
    text.cpp
  )
  set_src(GAME_CLIENT GLOB_RECURSE src/game/client 
    components/ball.cpp
    components/ball.h
    components/binds.cpp
    components/binds.h
    components/block.cpp
    components/block.h
    components/blood.cpp
    components/blood.h
    components/brains.cpp
    components/brains.h
    components/broadcast.cpp
    components/broadcast.h
    components/buildings.cpp
    components/buildings.h
    components/buildings2.cpp
    components/buildings2.h
    components/camera.cpp
    components/camera.h
    components/cbelt.cpp
    components/cbelt.h
    components/chat.cpp
    components/chat.h
    components/console.cpp
    components/console.h
    components/controls.cpp
    components/controls.h
    components/countryflags.cpp
    components/countryflags.h
    components/damageind.cpp
    components/damageind.h
    components/debughud.cpp
    components/debughud.h
    components/droids.cpp
    components/droids.h
    components/effects.cpp
    components/effects.h
    components/flow.cpp
    components/flow.h
    components/fluid.cpp
    components/fluid.h
    components/gamevote.cpp
    components/gamevote.h
    components/guts.cpp
    components/guts.h
    components/hud.cpp
    components/hud.h
    components/inventory.cpp
    components/inventory.h
    components/items.cpp
    components/items.h
    components/killmessages.cpp
    components/killmessages.h
    components/light.cpp
    components/light.h
    components/mapimages.cpp
    components/mapimages.h
    components/maplayers.cpp
    components/maplayers.h
    components/menus_browser.cpp
    components/menus_demo.cpp
    components/menus_ingame.cpp
    components/menus_settings.cpp
    components/menus.cpp
    components/menus.h
    components/motd.cpp
    components/motd.h
    components/nameplates.cpp
    components/nameplates.h
    components/particles.cpp
    components/particles.h
    components/picker.cpp
    components/picker.h
    components/players.cpp
    components/players.h
    components/radar.cpp
    components/radar.h
    components/scoreboard.cpp
    components/scoreboard.h
    components/skins.cpp
    components/skins.h
    components/sounds.cpp
    components/sounds.h
    components/spark.cpp
    components/spark.h
    components/spectator.cpp
    components/spectator.h
    components/splatter.cpp
    components/splatter.h
    components/tracer.cpp
    components/tracer.h
    components/voting.cpp
    components/voting.h
    components/weapons.cpp
    components/weapons.h
    customstuff/droidanim.cpp
    customstuff/droidanim.h
    customstuff/playerinfo.cpp
    customstuff/playerinfo.h
    animdata.cpp
    animdata.h
    component.h
    customstuff.cpp
    customstuff.h
    gameclient.cpp
    gameclient.h
    lineinput.cpp
    lineinput.h
    render_anim.inl
    render_map.cpp
    render.cpp
    render.h
    skelebank.cpp
    skelebank.h
    spine.cpp
    spine.h
    ui.cpp
    ui.h
  )
  set_src(GAME_EDITOR GLOB src/game/editor
    auto_map.cpp
    auto_map.h
    editor.cpp
    editor.h
    generator.cpp
    generator.h
    io.cpp
    layer_game.cpp
    layer_quads.cpp
    layer_tiles.cpp
    popups.cpp
    room.cpp
    room.h
  )

  set(CLIENT_SRC ${ENGINE_CLIENT} ${PLATFORM_CLIENT} ${GAME_CLIENT} ${GAME_EDITOR})

  set(DEPS_CLIENT ${DEPS} ${GLEW_DEP} ${PNGLITE_DEP} ${WAVPACK_DEP})

  # Libraries
  set(LIBS_CLIENT
    ${LIBS}
    ${FREETYPE_LIBRARIES}
    ${PNGLITE_LIBRARIES}
    ${SDL3_LIBRARIES}
    ${WAVPACK_LIBRARIES}
    ${PLATFORM_CLIENT_LIBS}
    ${GLEW_LIBRARIES}
  )

  if(TARGET_OS STREQUAL "windows")
    set(CLIENT_ICON "other/icons/${CLIENT_EXECUTABLE}.rc")
  else()
    set(CLIENT_ICON)
  endif()

  # Target
  set(TARGET_CLIENT ${CLIENT_EXECUTABLE})
  add_executable(${TARGET_CLIENT}
    ${CLIENT_SRC}
    ${CLIENT_ICON}
    ${CLIENT_MANIFEST}
    ${DEPS_CLIENT}
    $<TARGET_OBJECTS:engine-shared>
    $<TARGET_OBJECTS:game-shared>
  )
  target_link_libraries(${TARGET_CLIENT} ${LIBS_CLIENT})

  target_include_directories(${TARGET_CLIENT} PRIVATE
    ${FREETYPE_INCLUDE_DIRS}
    ${PNGLITE_INCLUDE_DIRS}
    ${SDL3_INCLUDE_DIRS}
    ${WAVPACK_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
  )

  set(PARAMS "${WAVPACK_INCLUDE_DIRS};${WAVPACK_INCLUDE_DIRS}")
  if(NOT(WAVPACK_OPEN_FILE_INPUT_EX_PARAMS STREQUAL PARAMS))
    unset(WAVPACK_OPEN_FILE_INPUT_EX CACHE)
  endif()
  set(WAVPACK_OPEN_FILE_INPUT_EX_PARAMS "${PARAMS}" CACHE INTERNAL "")

  set(CMAKE_REQUIRED_INCLUDES ${ORIGINAL_CMAKE_REQUIRED_INCLUDES} ${WAVPACK_INCLUDE_DIRS})
  set(CMAKE_REQUIRED_LIBRARIES ${ORIGINAL_CMAKE_REQUIRED_LIBRARIES} ${WAVPACK_LIBRARIES})
  check_symbol_exists(WavpackOpenFileInputEx wavpack.h WAVPACK_OPEN_FILE_INPUT_EX)
  set(CMAKE_REQUIRED_INCLUDES ${ORIGINAL_CMAKE_REQUIRED_INCLUDES})
  set(CMAKE_REQUIRED_LIBRARIES ${ORIGINAL_CMAKE_REQUIRED_LIBRARIES})

  if(WAVPACK_OPEN_FILE_INPUT_EX)
    target_compile_definitions(${TARGET_CLIENT} PRIVATE CONF_WAVPACK_OPEN_FILE_INPUT_EX)
  endif()

  list(APPEND TARGETS_OWN ${TARGET_CLIENT})
  list(APPEND TARGETS_LINK ${TARGET_CLIENT})
endif()

########################################################################
# SERVER
########################################################################

# Sources
set_src(ENGINE_SERVER GLOB src/engine/server
  localization.cpp
  localization.h
  register.cpp
  register.h
  server.cpp
  server.h
)

set_src(GAME_SERVER GLOB_RECURSE src/game/server
  ai/def/def_alien1_ai.cpp
  ai/def/def_alien1_ai.h
  ai/def/def_alien2_ai.cpp
  ai/def/def_alien2_ai.h
  ai/def/def_bunny1_ai.cpp
  ai/def/def_bunny1_ai.h
  ai/def/def_bunny2_ai.cpp
  ai/def/def_bunny2_ai.h
  ai/def/def_pyro1_ai.cpp
  ai/def/def_pyro1_ai.h
  ai/def/def_pyro2_ai.cpp
  ai/def/def_pyro2_ai.h
  ai/def/def_robot1_ai.cpp
  ai/def/def_robot1_ai.h
  ai/def/def_robot2_ai.cpp
  ai/def/def_robot2_ai.h
  ai/def/def_robot3_ai.cpp
  ai/def/def_robot3_ai.h
  ai/inv/alien1_ai.cpp
  ai/inv/alien1_ai.h
  ai/inv/alien2_ai.cpp
  ai/inv/alien2_ai.h
  ai/inv/bunny1_ai.cpp
  ai/inv/bunny1_ai.h
  ai/inv/bunny2_ai.cpp
  ai/inv/bunny2_ai.h
  ai/inv/pyro1_ai.cpp
  ai/inv/pyro1_ai.h
  ai/inv/pyro2_ai.cpp
  ai/inv/pyro2_ai.h
  ai/inv/robot1_ai.cpp
  ai/inv/robot1_ai.h
  ai/inv/robot2_ai.cpp
  ai/inv/robot2_ai.h
  ai/ball_ai.cpp
  ai/ball_ai.h
  ai/base_ai.cpp
  ai/base_ai.h
  ai/ctf_ai.cpp
  ai/ctf_ai.h
  ai/def_ai.cpp
  ai/def_ai.h
  ai/dm_ai.cpp
  ai/dm_ai.h
  ai/inv_ai.cpp
  ai/inv_ai.h
  ai/roam_ai.cpp
  ai/roam_ai.h
  ai/tdm_ai.cpp
  ai/tdm_ai.h
  ai/texas_ai.cpp
  ai/texas_ai.h
  entities/ball.cpp
  entities/ball.h
  entities/block.cpp
  entities/block.h
  entities/building.cpp
  entities/building.h
  entities/character.cpp
  entities/character.h
  entities/deathray.cpp
  entities/deathray.h
  entities/droid_bosscrawler.cpp
  entities/droid_bosscrawler.h
  entities/droid_crawler.cpp
  entities/droid_crawler.h
  entities/droid_star.cpp
  entities/droid_star.h
  entities/droid_walker.cpp
  entities/droid_walker.h
  entities/droid.cpp
  entities/droid.h
  entities/electro.cpp
  entities/electro.h
  entities/electrowall.cpp
  entities/electrowall.h
  entities/flag.cpp
  entities/flag.h
  entities/laser.cpp
  entities/laser.h
  entities/laserfail.cpp
  entities/laserfail.h
  entities/lightning.cpp
  entities/lightning.h
  entities/pickup.cpp
  entities/pickup.h
  entities/powerupper.cpp
  entities/powerupper.h
  entities/projectile.cpp
  entities/projectile.h
  entities/radar.cpp
  entities/radar.h
  entities/shop.cpp
  entities/shop.h
  entities/staticlaser.cpp
  entities/staticlaser.h
  entities/superexplosion.cpp
  entities/superexplosion.h
  entities/teslacoil.cpp
  entities/teslacoil.h
  entities/turret.cpp
  entities/turret.h
  entities/weapon.cpp
  entities/weapon.h
  gamemodes/ball.cpp
  gamemodes/ball.h
  gamemodes/base.cpp
  gamemodes/base.h
  gamemodes/cs.cpp
  gamemodes/cs.h
  gamemodes/ctf.cpp
  gamemodes/ctf.h
  gamemodes/dm.cpp
  gamemodes/dm.h
  gamemodes/invasion.cpp
  gamemodes/invasion.h
  gamemodes/roam.cpp
  gamemodes/roam.h
  gamemodes/tdm.cpp
  gamemodes/tdm.h
  gamemodes/texasrun.cpp
  gamemodes/texasrun.h
  mapgen/gen_layer.cpp
  mapgen/gen_layer.h
  mapgen/maze.cpp
  mapgen/maze.h
  mapgen/room.cpp
  mapgen/room.h
  ai_protocol.h
  ai.cpp
  ai.h
  aiskin.h
  blockentities.cpp
  blockentities.h  
  entity.cpp
  entity.h
  eventhandler.cpp
  eventhandler.h
  gamecontext.cpp
  gamecontext.h
  gamecontroller.cpp
  gamecontroller.h
  gamevote.h
  gameworld.cpp
  gameworld.h
  mapgen.cpp
  mapgen.h
  player.cpp
  player.h
  playerdata.cpp
  playerdata.h
)

set(SERVER_SRC ${ENGINE_SERVER} ${GAME_SERVER})
if(TARGET_OS STREQUAL "windows")
  set(SERVER_ICON "other/icons/${SERVER_EXECUTABLE}.rc")
else()
  set(SERVER_ICON)
endif()

# Libraries
set(LIBS_SERVER ${LIBS})

# Target
set(TARGET_SERVER ${SERVER_EXECUTABLE})
add_executable(${TARGET_SERVER}
  ${DEPS}
  ${SERVER_SRC}
  ${SERVER_ICON}
  $<TARGET_OBJECTS:engine-shared>
  $<TARGET_OBJECTS:game-shared>
)
target_link_libraries(${TARGET_SERVER} ${LIBS_SERVER})
list(APPEND TARGETS_OWN ${TARGET_SERVER})
list(APPEND TARGETS_LINK ${TARGET_SERVER})

if(TARGET_OS AND TARGET_OS STREQUAL "mac")
  set(SERVER_LAUNCHER_SRC src/osxlaunch/server.m)
  set(TARGET_SERVER_LAUNCHER ${TARGET_SERVER}-Launcher)
  add_executable(${TARGET_SERVER_LAUNCHER} ${SERVER_LAUNCHER_SRC})
  target_link_libraries(${TARGET_SERVER_LAUNCHER} ${COCOA})
  list(APPEND TARGETS_OWN ${TARGET_SERVER_LAUNCHER})
  list(APPEND TARGETS_LINK ${TARGET_SERVER_LAUNCHER})
endif()

########################################################################
# VARIOUS TARGETS
########################################################################

set_src(MASTERSRV_SRC GLOB src/mastersrv mastersrv.cpp mastersrv.h)
set_src(VERSIONSRV_SRC GLOB src/versionsrv mapversions.h versionsrv.cpp versionsrv.h)

set(TARGET_MASTERSRV mastersrv)
set(TARGET_VERSIONSRV versionsrv)

add_executable(${TARGET_MASTERSRV} EXCLUDE_FROM_ALL ${MASTERSRV_SRC} $<TARGET_OBJECTS:engine-shared> ${DEPS})
add_executable(${TARGET_VERSIONSRV} EXCLUDE_FROM_ALL ${VERSIONSRV_SRC} $<TARGET_OBJECTS:engine-shared> ${DEPS})

target_link_libraries(${TARGET_MASTERSRV} ${LIBS})
target_link_libraries(${TARGET_VERSIONSRV} ${LIBS})

list(APPEND TARGETS_OWN ${TARGET_MASTERSRV} ${TARGET_VERSIONSRV})
list(APPEND TARGETS_LINK ${TARGET_MASTERSRV} ${TARGET_VERSIONSRV})
########################################################################
# INSTALLATION
########################################################################

function(escape_regex VAR STRING)
  string(REGEX REPLACE "([][^$.+*?|()\\\\])" "\\\\\\1" ESCAPED "${STRING}")
  set(${VAR} ${ESCAPED} PARENT_SCOPE)
endfunction()

function(escape_backslashes VAR STRING)
  string(REGEX REPLACE "\\\\" "\\\\\\\\" ESCAPED "${STRING}")
  set(${VAR} ${ESCAPED} PARENT_SCOPE)
endfunction()

function(max_length VAR)
  set(MAX_LENGTH 0)
  foreach(str ${ARGN})
    string(LENGTH ${str} LENGTH)
    if(LENGTH GREATER MAX_LENGTH)
      set(MAX_LENGTH ${LENGTH})
    endif()
  endforeach()
  set(${VAR} ${MAX_LENGTH} PARENT_SCOPE)
endfunction()

# Tries to generate a list of regex that matches everything except the given
# parameters.
function(regex_inverted VAR)
  max_length(MAX_LENGTH ${ARGN})
  math(EXPR UPPER_BOUND "${MAX_LENGTH}-1")

  set(REMAINING ${ARGN})
  set(RESULT)

  foreach(i RANGE ${UPPER_BOUND})
    set(TEMP ${REMAINING})
    set(REMAINING)
    foreach(str ${TEMP})
      string(LENGTH ${str} LENGTH)
      if(i LESS LENGTH)
        list(APPEND REMAINING ${str})
      endif()
    endforeach()

    set(ADDITIONAL)
    foreach(outer ${REMAINING})
      string(SUBSTRING ${outer} 0 ${i} OUTER_PREFIX)
      set(CHARS "")
      foreach(inner ${REMAINING})
        string(SUBSTRING ${inner} 0 ${i} INNER_PREFIX)
        if(OUTER_PREFIX STREQUAL INNER_PREFIX)
          string(SUBSTRING ${inner} ${i} 1 INNER_NEXT)
          set(CHARS "${CHARS}${INNER_NEXT}")
        endif()
      endforeach()
      escape_regex(OUTER_PREFIX_ESCAPED "${OUTER_PREFIX}")

      list(APPEND ADDITIONAL "${OUTER_PREFIX_ESCAPED}([^${CHARS}]|$)")
    endforeach()
    list(REMOVE_DUPLICATES ADDITIONAL)
    list(APPEND RESULT ${ADDITIONAL})
  endforeach()
  set(${VAR} ${RESULT} PARENT_SCOPE)
endfunction()

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_GENERATOR TGZ TXZ)
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_STRIP_FILES TRUE)
set(CPACK_COMPONENTS_ALL portable)
set(CPACK_SOURCE_GENERATOR ZIP TGZ TBZ2 TXZ)
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
if(VERSION_PATCH STREQUAL "0")
  set(CPACK_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR})
else()
  set(CPACK_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH})
endif()
set(CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_NAME})

if(TARGET_OS AND TARGET_BITS)
  if(TARGET_OS STREQUAL "windows")
    set(CPACK_SYSTEM_NAME "win${TARGET_BITS}")
    set(CPACK_GENERATOR ZIP)
  elseif(TARGET_OS STREQUAL "linux")
    # Assuming Intel here.
    if(TARGET_BITS EQUAL 32)
      set(CPACK_SYSTEM_NAME "linux_x86")
    elseif(TARGET_BITS EQUAL 64)
      set(CPACK_SYSTEM_NAME "linux_x86_64")
    endif()
  elseif(TARGET_OS STREQUAL "mac")
    set(CPACK_SYSTEM_NAME "osx")
    set(CPACK_GENERATOR DMG)
  endif()
endif()

set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_SYSTEM_NAME})
set(CPACK_ARCHIVE_PORTABLE_FILE_NAME ${CPACK_PACKAGE_FILE_NAME})
set(CPACK_SOURCE_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-src)
set(CPACK_SOURCE_FILES
  CMakeLists.txt
  bam.lua
  cmake/
  configure.lua
  data/
  datasrc/
  docs/
  license.txt
  objs/
  other/
  readme.txt
  scripts/
  src/
  storage.cfg
)
set(CPACK_SOURCE_IGNORE_FILES
  "\\\\.o$"
  "\\\\.pyc$"
  "/\\\\.git"
  "/__pycache__/"
)

regex_inverted(CPACK_SOURCE_FILES_INVERTED ${CPACK_SOURCE_FILES})
escape_regex(PROJECT_SOURCE_DIR_ESCAPED ${PROJECT_SOURCE_DIR})

foreach(str ${CPACK_SOURCE_FILES_INVERTED})
  escape_backslashes(STR_ESCAPED "${PROJECT_SOURCE_DIR_ESCAPED}/${str}")
  list(APPEND CPACK_SOURCE_IGNORE_FILES "${STR_ESCAPED}")
endforeach()

set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME ${PROJECT_NAME})

set(CPACK_TARGETS
  ${TARGET_CLIENT}
  ${TARGET_SERVER}
)
set(CPACK_DIRS data)
set(CPACK_FILES
  license.txt
  storage.cfg
  ${COPY_FILES}
)
if(TARGET_OS STREQUAL "windows")
  list(APPEND CPACK_FILES other/config_directory.bat)
endif()

if(NOT DEV)
  install(DIRECTORY data DESTINATION share/${PROJECT_NAME} COMPONENT data)
  install(TARGETS ${TARGET_CLIENT} DESTINATION bin COMPONENT client)
  install(TARGETS ${TARGET_SERVER} DESTINATION bin COMPONENT server)
endif()

if(DEV)
  # Don't generate CPack targets.
elseif(CMAKE_VERSION VERSION_LESS 3.6 OR CMAKE_VERSION VERSION_EQUAL 3.6)
  message(WARNING "Cannot create CPack targets, CMake version too old. Use CMake 3.6 or newer.")
else()
  set(EXTRA_ARGS DESTINATION ${CPACK_PACKAGE_FILE_NAME} COMPONENT portable EXCLUDE_FROM_ALL)
  install(TARGETS ${CPACK_TARGETS} ${EXTRA_ARGS})
  install(DIRECTORY ${CPACK_DIRS} ${EXTRA_ARGS})
  install(FILES ${CPACK_FILES} ${EXTRA_ARGS})
endif()

set(PACKAGE_TARGETS)

foreach(ext zip tar.gz tar.xz)
  set(TAR_MODE c)
  set(TAR_EXTRA_ARGS)
  string(REPLACE . _ EXT_SLUG ${ext})

  set(TMPDIR pack_${CPACK_PACKAGE_FILE_NAME}_${EXT_SLUG}/${CPACK_PACKAGE_FILE_NAME})

  set(COPY_FILE_COMMANDS)
  set(COPY_DIR_COMMANDS)
  set(COPY_TARGET_COMMANDS)
  set(STRIP_TARGET_COMMANDS)
  foreach(file ${CPACK_FILES})
    list(APPEND COPY_FILE_COMMANDS COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/${file} ${TMPDIR}/)
  endforeach()
  foreach(dir ${CPACK_DIRS})
    list(APPEND COPY_DIR_COMMANDS COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/${dir} ${TMPDIR}/${dir})
  endforeach()
  foreach(target ${CPACK_TARGETS})
    list(APPEND COPY_TARGET_COMMANDS COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${target}> ${TMPDIR}/)
  endforeach()

  if(ext STREQUAL zip)
    set(TAR_EXTRA_ARGS --format=zip)
  elseif(ext STREQUAL tar.gz)
    set(TAR_MODE cz)
  elseif(ext STREQUAL tar.xz)
    set(TAR_MODE cJ)
  endif()
  add_custom_command(OUTPUT ${CPACK_PACKAGE_FILE_NAME}.${ext}
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${TMPDIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${TMPDIR}
    ${COPY_FILE_COMMANDS}
    ${COPY_DIR_COMMANDS}
    ${COPY_TARGET_COMMANDS}
    ${STRIP_TARGET_COMMANDS}
    COMMAND ${CMAKE_COMMAND} -E chdir pack_${CPACK_PACKAGE_FILE_NAME}_${EXT_SLUG} ${CMAKE_COMMAND} -E tar ${TAR_MODE} ../${CPACK_PACKAGE_FILE_NAME}.${ext} ${TAR_EXTRA_ARGS} -- ${CPACK_PACKAGE_FILE_NAME}/
    DEPENDS ${CPACK_TARGETS}
  )
  add_custom_target(package_${EXT_SLUG} DEPENDS ${CPACK_PACKAGE_FILE_NAME}.${ext})
  list(APPEND PACKAGE_TARGETS package_${EXT_SLUG})
endforeach()

set(PACKAGE_DEFAULT tar_xz)
if(TARGET_OS STREQUAL "windows")
  set(PACKAGE_DEFAULT zip)
elseif(TARGET_OS STREQUAL "mac")
  set(PACKAGE_DEFAULT dmg)
endif()
add_custom_target(package_default DEPENDS package_${PACKAGE_DEFAULT})

add_custom_target(package_all DEPENDS ${PACKAGE_TARGETS})

# Unset these variables, they might do something in the future of CPack.
unset(CPACK_SOURCE_FILES)
unset(CPACK_SOURCE_FILES_INVERTED)
unset(CPACK_TARGETS)
unset(CPACK_DIRS)
unset(CPACK_FILES)

include(CPack)

########################################################################
# COMPILER-SPECIFICS
########################################################################

# In the future (CMake 3.8.0+), use source_group(TREE ...)
macro(source_group_tree dir)
  file(GLOB ents RELATIVE ${PROJECT_SOURCE_DIR}/${dir} ${PROJECT_SOURCE_DIR}/${dir}/*)
  foreach(ent ${ents})
    if(IS_DIRECTORY ${PROJECT_SOURCE_DIR}/${dir}/${ent})
      source_group_tree(${dir}/${ent})
    else()
      string(REPLACE "/" "\\" group ${dir})
      source_group(${group} FILES ${PROJECT_SOURCE_DIR}/${dir}/${ent})
    endif()
  endforeach()
endmacro()
source_group_tree(src)

set(TARGETS ${TARGETS_OWN} ${TARGETS_DEP})

foreach(target ${TARGETS})
  if(MSVC)
    target_compile_options(${target} PRIVATE $<$<NOT:${DBG}>:/MT> $<${DBG}:/MTd>) # Use static CRT
    target_compile_options(${target} PRIVATE /MP) # Use multiple cores
    target_compile_options(${target} PRIVATE /EHsc) # Only catch C++ exceptions with catch.
    target_compile_options(${target} PRIVATE /GS) # Protect the stack pointer.
    target_compile_options(${target} PRIVATE /wd4996) # Use of non-_s functions.
  endif()
  if(OUR_FLAGS)
    target_compile_options(${target} PRIVATE ${OUR_FLAGS})
  endif()
  if(DEFINE_FORTIFY_SOURCE)
    target_compile_definitions(${target} PRIVATE $<$<NOT:$<CONFIG:Debug>>:_FORTIFY_SOURCE=2>) # Detect some buffer overflows.
  endif()
endforeach()

foreach(target ${TARGETS_LINK})
  if(MSVC)
    set_property(TARGET ${target} APPEND PROPERTY LINK_FLAGS /SAFESEH:NO) # Disable SafeSEH because the shipped libraries don't support it (would cause error LNK2026 otherwise).
  endif()
  if(TARGET_OS STREQUAL "mac")
    target_link_libraries(${target} -stdlib=libc++)
    target_link_libraries(${target} -mmacosx-version-min=10.7)
  endif()
  if((MINGW OR TARGET_OS STREQUAL "linux") AND PREFER_BUNDLED_LIBS)
    # Statically link the standard libraries with on MinGW/Linux so we don't
    # have to ship them as DLLs.
    target_link_libraries(${target} -static-libgcc)
    target_link_libraries(${target} -static-libstdc++)
    if(MINGW)
      # Link pthread library statically instead of dynamically.
      # Solution from https://stackoverflow.com/a/28001261.
      target_link_libraries(${target} -Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic)
    endif()
  endif()

  target_compile_definitions(${target} PRIVATE GLEW_STATIC)
endforeach()

foreach(target ${TARGETS_OWN})
  if(MSVC)
    target_compile_options(${target} PRIVATE /wd4244) # Possible loss of data (float -> int, int -> float, etc.).
    target_compile_options(${target} PRIVATE /wd4267) # Possible loss of data (size_t - int on win64).
    target_compile_options(${target} PRIVATE /wd4800) # Implicit conversion of int to bool.
  endif()
  if(OUR_FLAGS_OWN)
    target_compile_options(${target} PRIVATE ${OUR_FLAGS_OWN})
  endif()
  target_include_directories(${target} PRIVATE ${PROJECT_BINARY_DIR}/src)
  target_include_directories(${target} PRIVATE src)
  target_compile_definitions(${target} PRIVATE $<$<CONFIG:Debug>:CONF_DEBUG>)
  target_include_directories(${target} PRIVATE ${ZLIB_INCLUDE_DIRS})

  set_target_properties(${target} PROPERTIES CXX_STANDARD_REQUIRED ON)
  target_compile_features(${target} PUBLIC cxx_std_17)

endforeach()

foreach(target ${TARGETS_DEP})
  if(MSVC)
    target_compile_options(${target} PRIVATE /W0)
  endif()
  if(OUR_FLAGS_DEP)
    target_compile_options(${target} PRIVATE ${OUR_FLAGS_DEP})
  endif()
endforeach()
