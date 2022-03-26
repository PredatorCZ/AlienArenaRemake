set(AA_CLIENT_SOURCES
    client/cl_ents.c
    client/cl_fx.c
    client/cl_http.c
    client/cl_input.c
    client/cl_inv.c
    client/cl_irc.c
    client/cl_main.c
    client/cl_parse.c
    client/cl_pred.c
    client/cl_scrn.c
    client/cl_stats.c
    client/cl_tent.c
    client/cl_updates.c
    client/cl_view.c
    client/console.c
    client/keys.c
    client/menu.c
    client/qal.c
    client/qmenu.c
    client/snd_file.c
    client/snd_openal.c
)

set(AA_COMMON_SOURCES
    qcommon/binheap.c
    qcommon/cmd.c
    qcommon/cmodel.c
    qcommon/common.c
    qcommon/crc.c
    qcommon/cryptohash.c
    qcommon/cvar.c
    qcommon/files.c
    qcommon/hmac_sha2.c
    qcommon/htable.c
    qcommon/image.c
    qcommon/libgarland.c
    qcommon/lrucache.c
    qcommon/md5.c
    qcommon/mdfour.c
    qcommon/net_chan.c
    qcommon/pmove.c
    qcommon/sha2.c
    qcommon/terrain.c

    qcommon/mm_hash.cpp
)

set(AA_SERVER_SOURCES
    server/sv_ccmds.c
    server/sv_ents.c
    server/sv_game.c
    server/sv_init.c
    server/sv_main.c
    server/sv_send.c
    server/sv_user.c
    server/sv_world.c
)

set(AA_RENDER_SOURCES
    ref_gl/r_bloom.c
    ref_gl/r_image.cpp
    ref_gl/r_image.c
    ref_gl/r_misc.c
    ref_gl/r_particle.c
    ref_gl/r_ragdoll.c
    ref_gl/r_terrain.c
    ref_gl/r_ttf.c
    ref_gl/r_vbo.c
    ref_gl/r_decals.c
    ref_gl/r_light.c
    ref_gl/r_main.c
    ref_gl/r_md2.c
    ref_gl/r_model.c
    ref_gl/r_postprocess.c
    ref_gl/r_shadowmaps.c
    ref_gl/r_text.c
    ref_gl/r_warp.c
    ref_gl/r_draw.c
    ref_gl/r_iqm.c
    ref_gl/r_math.c
    ref_gl/r_mesh.c
    ref_gl/r_program.c
    ref_gl/r_surf.c
    ref_gl/r_varray.c

    ref_gl/asset_manager.cpp
    ref_gl/r_script.cpp
)

set(AA_UNIX_SOURCES
    unix/gl_glx.c
    unix/glob.c
    unix/net_udp.c
    unix/q_shunix.c
    unix/qal_unix.c
    unix/qgl_unix.c
    unix/rw_unix.c
    unix/sys_unix.c
    unix/vid_so.c
)

# shared lib???
set(AA_GAME_SOURCES
    game/c_cam.c
    game/g_ai.c
    game/g_chase.c
    game/g_cmds.c
    game/g_combat.c
    game/g_ctf.c
    game/g_deathray.c
    game/g_func.c
    game/g_items.c
    game/g_local.h
    game/g_main.c
    game/g_misc.c
    game/g_monster.c
    game/g_phys.c
    game/g_save.c
    game/g_spawn.c
    game/g_spider.c
    game/g_svcmds.c
    game/g_target.c
    game/g_trigger.c
    game/g_unlagged.c
    game/g_utils.c
    game/g_vehicles.c
    game/g_weapon.c
    game/m_move.c
    game/p_client.c
    game/p_hud.c
    game/p_trail.c
    game/p_view.c
    game/p_weapon.c
    game/q_shared.c
    game/acesrc/acebot_ai.c
    game/acesrc/acebot_cmds.c
    game/acesrc/acebot.h
    game/acesrc/acebot_items.c
    game/acesrc/acebot_movement.c
    game/acesrc/acebot_nodes.c
    game/acesrc/acebot_spawn.c
)

find_package(CURL REQUIRED)
find_package(OpenAL REQUIRED)
find_package(Vorbis REQUIRED)
find_package(Threads REQUIRED)
find_package(JPEG REQUIRED)
find_package(Freetype REQUIRED)
find_package(X11 REQUIRED)
find_package(ODE REQUIRED)

add_executable(alien_arena ${AA_CLIENT_SOURCES} ${AA_COMMON_SOURCES} ${AA_SERVER_SOURCES} ${AA_RENDER_SOURCES} ${AA_UNIX_SOURCES} ${AA_GAME_SOURCES} )
target_link_libraries(alien_arena CURL::libcurl ${OPENAL_LIBRARY} Vorbis::vorbis Vorbis::vorbisfile Threads::Threads m JPEG::JPEG Freetype::Freetype dl ${X11_LIBRARIES} ODE::ODE)
target_include_directories(alien_arena PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${OPENAL_INCLUDE_DIR} unix)
target_compile_definitions(alien_arena PRIVATE UNIX_VARIANT HAVE_AL_AL_H OPENGL_DRIVER="libGL.so.1" HAVE_DLFCN_H HAVE_UNISTD_H HAVE_PUTENV HAVE_UNLINK HAVE_STRDUP HAVE_DIRECT_H HAVE_GETCWD)
target_compile_options(alien_arena PRIVATE -msse4.1)
