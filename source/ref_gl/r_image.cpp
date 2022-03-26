/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2014 COR Entertainment, LLC.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include "asset_manager.hpp"
#include "qcommon/mm_hash.h"
#include <map>
#include <string_view>

extern "C" {
#include "r_local.h"
#undef max
#undef min

extern cvar_t *cl_hudimage1; // custom huds
extern cvar_t *cl_hudimage2;
extern cvar_t *cl_hudimage3;

int gl_filter_min = GL_LINEAR_MIPMAP_LINEAR;
int gl_filter_max = GL_LINEAR;
int gl_tex_alpha_format = 4;
int gl_tex_solid_format = 3;
static char deptex_names[deptex_num][32];

void GL_TextureMode(char *string) {
  struct GLMode {
    uint32_t minimize;
    uint32_t maximize;
  };

  static const std::map<std::string_view, GLMode> MODES{
      {"GL_NEAREST", {GL_NEAREST, GL_NEAREST}},
      {"GL_LINEAR", {GL_LINEAR, GL_LINEAR}},
      {"GL_NEAREST_MIPMAP_NEAREST", {GL_NEAREST_MIPMAP_NEAREST, GL_NEAREST}},
      {"GL_LINEAR_MIPMAP_NEAREST", {GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR}},
      {"GL_NEAREST_MIPMAP_LINEAR", {GL_NEAREST_MIPMAP_LINEAR, GL_NEAREST}},
      {"GL_LINEAR_MIPMAP_LINEAR", {GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR}},
  };

  auto foundMode = MODES.find(string);

  if (foundMode == MODES.end()) {
    Com_Printf("bad filter name\n");
    return;
  }
  const auto mode = foundMode->second;

  gl_filter_min = mode.minimize;
  gl_filter_max = mode.maximize;

  GL_SelectTexture(0);

  // change all the existing mipmap texture objects
  asset_manager::IterateGroup(
      asset_manager::AssetType::Texture, [](void *data) {
        image_t *image = static_cast<image_t *>(data);

        if (image->type != it_pic && image->type != it_particle &&
            image->type != it_sky) {
          GL_Bind(image->index);
          qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_min);
          qglTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
        }
      });
}

void GL_TextureAlphaMode(char *string) {
  static const std::map<std::string_view, uint32_t> MODES{
      /**/ //
      {"default", 4},
      {"GL_RGBA", GL_RGBA},
      {"GL_RGBA8", GL_RGBA8},
      {"GL_RGB5_A1", GL_RGB5_A1},
      {"GL_RGBA4", GL_RGBA4},
      {"GL_RGBA2", GL_RGBA2},
  };

  auto foundMode = MODES.find(string);

  if (foundMode == MODES.end()) {
    Com_Printf("bad alpha texture mode name\n");
    return;
  }

  gl_tex_alpha_format = foundMode->second;
}

void GL_TextureSolidMode(char *string) {
  static const std::map<std::string_view, uint32_t> MODES{
      /**/ //
      {"default", 3},
      {"GL_RGB", GL_RGB},
      {"GL_RGB8", GL_RGB8},
      {"GL_RGB5", GL_RGB5},
      {"GL_RGB4", GL_RGB4},
      {"GL_R3_G3_B2", GL_R3_G3_B2},
#ifdef GL_RGB2_EXT
      {"GL_RGB2", GL_RGB2_EXT},
#endif
  };

  auto foundMode = MODES.find(string);

  if (foundMode == MODES.end()) {
    Com_Printf("bad solid texture mode name\n");
    return;
  }

  gl_tex_solid_format = foundMode->second;
}

void GL_ImageList_f() {
  Com_Printf("------------------\n");
  uint64_t texels = 0;

  asset_manager::IterateGroup(
      asset_manager::AssetType::Texture, [&](void *data) {
        image_t *image = static_cast<image_t *>(data);

        texels += image->upload_width * image->upload_height;
        switch (image->type) {
        case it_skin:
          Com_Printf("M");
          break;
        case it_sprite:
          Com_Printf("S");
          break;
        case it_wall:
          Com_Printf("W");
          break;
        case it_pic:
          Com_Printf("P");
          break;
        case it_particle:
          Com_Printf("A");
          break;
        default:
          Com_Printf(" ");
          break;
        }

        Com_Printf(" %3i %3i: %s\n", image->upload_width, image->upload_height,
                   image->name);
      });
  Com_Printf("Total texel count (not counting mipmaps): %i\n", texels);
}

/*
=============================================================================

  scrap allocation

  Allocate all the little status bar obejcts into a single texture
  to crutch up inefficient hardware / drivers

=============================================================================
*/

#define MAX_SCRAPS 1
#define BLOCK_WIDTH 1024
#define BLOCK_HEIGHT 512

int scrap_allocated[MAX_SCRAPS][BLOCK_WIDTH];
byte scrap_texels[MAX_SCRAPS][BLOCK_WIDTH * BLOCK_HEIGHT * 4];

// returns a texture number and the position inside it
int Scrap_AllocBlock(int w, int h, int *x, int *y) {
  int i, j;
  int best, best2;
  int texnum;

  for (texnum = 0; texnum < MAX_SCRAPS; texnum++) {
    best = BLOCK_HEIGHT;

    for (i = 0; i < BLOCK_WIDTH - w; i++) {
      best2 = 0;

      for (j = 0; j < w; j++) {
        if (scrap_allocated[texnum][i + j] >= best)
          break;
        if (scrap_allocated[texnum][i + j] > best2)
          best2 = scrap_allocated[texnum][i + j];
      }
      if (j == w) { // this is a valid spot
        *x = i;
        *y = best = best2;
      }
    }

    if (best + h > BLOCK_HEIGHT)
      continue;

    for (i = 0; i < w; i++)
      scrap_allocated[texnum][*x + i] = best + h;

    return texnum;
  }

  return -1;
  /*	Sys_Error ("Scrap_AllocBlock: full");*/
}

qboolean GL_Upload32(byte *data, int width, int height, int picmip,
                     qboolean filter, qboolean modulate,
                     qboolean force_standard_mipmap);
qboolean GL_Upload8(byte *data, int width, int height, int picmip,
                    qboolean filter);

static void Scrap_Upload(image_t *image) {
  GL_SelectTexture(0);
  GL_Bind(image->index);
  GL_Upload32(scrap_texels[image->scrapIndex], BLOCK_WIDTH, BLOCK_HEIGHT, 0,
              false, false, true);
}

image_t *GL_FindFreeImage(const char *name, int width, int height,
                          imagetype_t type) {
  asset_manager::AssetHash newHash(MurmurHash(name, strlen(name)),
                                   asset_manager::AssetType::Texture);
  image_t *image = nullptr;

  // try {
  image = static_cast<image_t *>(asset_manager::NewAsset(newHash));
  /*} catch (...) {
    Com_Error(ERR_DROP, "MAX_GLTEXTURES");
  }*/

  if (name[0] != '*') { // not a special name, remove the path part
    strcpy(image->bare_name, COM_SkipPath(name));
  }

  image->registration_sequence = registration_sequence;
  image->width = width;
  image->height = height;
  image->type = type;

  return image;
}

image_t *GL_GetImage(const char *name) {
  asset_manager::AssetHash hash(MurmurHash(name, strlen(name)),
                                asset_manager::AssetType::Texture);

  image_t *image = static_cast<image_t *>(asset_manager::GetAsset(hash));

  if (image) {
    image->registration_sequence = registration_sequence;
    return image;
  }

  return nullptr;
}

enum TEXFlags {
  Cubemap = 1,
  Array = 2,
  Volume = 4,
  Compressed = 8,
  AlphaMasked = 0x10,
};

struct TEXEntry {
  uint16_t target;
  uint8_t level;
  uint8_t reserved;
  uint32_t bufferSize;
  uint64_t bufferOffset;
};

struct TEX {
  static constexpr uint32_t ID = 'TEX0';
  uint32_t id;
  uint32_t dataSize;
  uint16_t numEnries;
  uint16_t width;
  uint16_t height;
  uint16_t depth;
  uint16_t internalFormat;
  uint16_t format;
  uint16_t type;
  uint16_t target;
  uint8_t numMips;
  uint8_t numDims;
  TEXFlags flags;
};

FILE *OpenFile(const char *path) {
  FILE *h = nullptr;
  int len = FS_FOpenFile(path, &h);

  if (!h) {
    char lc_path[MAX_OSPATH];
    Q_strncpyz2(lc_path, path, sizeof(lc_path));
    std::transform(std::begin(lc_path), std::end(lc_path), std::begin(lc_path),
                   [](auto item) { return std::tolower(item); });

    if (strcmp(path, lc_path)) { // lowercase conversion changed something
      len = FS_FOpenFile(lc_path, &h);
    }
  }

  return h;
}

void LoadTEX(const char *name, imagetype_t type) {
  FILE *h = OpenFile(va("%s.gth", name));

  if (!h) {
    return; // todo error
  }

  TEX header;
  fread(&header, sizeof(header), 1, h);

  if (header.id != header.ID) {
    return; // todo error
  }

  std::vector<TEXEntry> entries;
  entries.resize(header.numEnries);
  fread(entries.data(), sizeof(TEXEntry), header.numEnries, h);
  fclose(h);

  std::string buffer;
  buffer.resize(header.dataSize);
  h = OpenFile(va("%s.gtb", name));

  if (!h) {
    return; // todo error
  }

  fread(buffer.data(), buffer.size(), 1, h);
  fclose(h);

  image_t *image = GL_FindFreeImage(name, header.width, header.height, type);
  GL_SelectTexture(0);
  GL_Bind(image->index);
  int max_size;
  qglGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);
  qglTexParameteri(header.target, GL_TEXTURE_BASE_LEVEL, 0);
  qglTexParameteri(header.target, GL_TEXTURE_MAX_LEVEL, header.numMips);
  qglTexParameterf(header.target, GL_TEXTURE_MIN_FILTER, gl_filter_min);
  qglTexParameterf(header.target, GL_TEXTURE_MAG_FILTER, gl_filter_max);

  if (header.flags & TEXFlags::AlphaMasked) {
    qglTexParameterf(header.target, GL_TEXTURE_MAX_ANISOTROPY_EXT,
                     r_alphamasked_anisotropic->integer);
  } else {
    qglTexParameterf(header.target, GL_TEXTURE_MAX_ANISOTROPY_EXT,
                     r_anisotropic->integer);
  }

  if (header.flags & TEXFlags::Compressed) {
    if (header.flags & TEXFlags::Volume) {
      // not implemented
    } else if (header.flags & TEXFlags::Array) {
      // not implemented
    } else {
      if (header.numDims == 1) {
        for (auto &e : entries) {
          uint32_t width = header.width >> e.level;
          qglCompressedTexImage1D(e.target, e.level, header.internalFormat,
                                  width, 0, buffer.size(),
                                  buffer.data() + e.bufferOffset);
        }
      } else if (header.numDims == 2) {
        for (auto &e : entries) {
          uint32_t height = header.height >> e.level;
          uint32_t width = header.width >> e.level;
          qglCompressedTexImage2D(e.target, e.level, header.internalFormat,
                                  width, height, 0, buffer.size(),
                                  buffer.data() + e.bufferOffset);
        }
      }
    }
  } else {
    if (header.flags & TEXFlags::Volume) {
      // not implemented
    } else if (header.flags & TEXFlags::Array) {
      // not implemented
    } else {
      if (header.numDims == 1) {
        for (auto &e : entries) {
          uint32_t width = header.width >> e.level;
          qglTexImage1D(e.target, e.level, header.internalFormat, width, 0,
                        header.format, header.type,
                        buffer.data() + e.bufferOffset);
        }
      } else if (header.numDims == 2) {
        for (auto &e : entries) {
          uint32_t height = header.height >> e.level;
          uint32_t width = header.width >> e.level;
          qglTexImage2D(e.target, e.level, header.internalFormat, width, height,
                        0, header.format, header.type,
                        buffer.data() + e.bufferOffset);
        }
      }
    }
  }
}

void R_FloodFillSkin(byte *skin, int skinwidth, int skinheight);
extern int upload_width, upload_height;
extern int crop_left, crop_right, crop_top, crop_bottom;
extern cvar_t *gl_picmip;

/*
================
GL_LoadPic

This is also used as an entry point for the generated r_notexture
================
*/
image_t *GL_LoadPic(const char *name, byte *pic, int width, int height,
                    imagetype_t type, int bits) {
  image_t *image;
  int i;
  int picmip;

  image = GL_FindFreeImage(name, width, height, type);

  if (type == it_skin && bits == 8)
    R_FloodFillSkin(pic, width, height);

  if (type == it_particle || type == it_pic)
    picmip = 0;
  else
    picmip = gl_picmip->integer;

  // load little particles into the scrap
  if (type == it_particle && bits != 8 && image->width <= 128 &&
      image->height <= 128) {
    int x, y;
    int i, j, k, l;

    image->scrapIndex = Scrap_AllocBlock(image->width, image->height, &x, &y);
    if (image->scrapIndex == -1)
      goto nonscrap;

    // copy the texels into the scrap block
    k = 0;
    for (i = 0; i < image->height; i++)
      for (j = 0; j < image->width; j++)
        for (l = 0; l < 4; l++, k++)
          scrap_texels[image->scrapIndex]
                      [((y + i) * BLOCK_WIDTH + x + j) * 4 + l] = pic[k];
    image->sl = (double)(x + 0.5) / (double)BLOCK_WIDTH;
    image->sh = (double)(x + image->width - 0.5) / (double)BLOCK_WIDTH;
    image->tl = (double)(y + 0.5) / (double)BLOCK_HEIGHT;
    image->th = (double)(y + image->height - 0.5) / (double)BLOCK_HEIGHT;

    // Send updated scrap to OpenGL. Wasteful to do it over and over like
    // this, but we don't care.
    Scrap_Upload(image);
  } else {
  nonscrap:
    image->scrapIndex = -1;
    GL_SelectTexture(0);
    GL_Bind(image->index);
    if (bits == 8) {
      GL_Upload8(pic, width, height, picmip, type <= it_wall);
    } else {
      GL_Upload32(pic, width, height, picmip, type <= it_wall,
                  type == it_lightmap, type >= it_bump);
    }

    if (type == it_pic)
      qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    // size in pixels after power of 2 and scales
    image->upload_width = upload_width;
    image->upload_height = upload_height;

    // vertex offset to apply when cropping
    image->crop_left = crop_left;
    image->crop_top = crop_top;

    // size in pixels after cropping
    image->crop_width = upload_width - crop_left - crop_right;
    image->crop_height = upload_height - crop_top - crop_bottom;

    // texcoords to use when not cropping
    image->sl = 0;
    image->sh = 1;
    image->tl = 0;
    image->th = 1;

    // texcoords to use when cropping
    image->crop_sl = 0 + (float)crop_left / (float)upload_width;
    image->crop_sh = 1 - (float)crop_right / (float)upload_width;
    image->crop_tl = 0 + (float)crop_top / (float)upload_height;
    image->crop_th = 1 - (float)crop_bottom / (float)upload_height;
  }

  return image;
}

void LoadJPG(char *filename, byte **pic, int *width, int *height);
void LoadPCX(char *filename, byte **pic, byte **palette, int *width,
             int *height);
image_t *GL_LoadWal(char *name);
rscript_t *RS_FindScript(char *name);

image_t *GL_FindImage(const char *name, imagetype_t type) {
  image_t *image = NULL;
  int len;
  byte *pic, *palette;
  int width, height;
  char shortname[MAX_QPATH];

  if (!name)
    goto ret_image; //	Com_Error (ERR_DROP, "GL_FindImage: NULL name");
  len = strlen(name);
  if (len < 5)
    goto ret_image; //	Com_Error (ERR_DROP, "GL_FindImage: bad name: %s",
                    // name);

  // if HUD, then we want to load the one according to what it is set to.
  if (!strcmp(name, "pics/i_health.pcx"))
    return GL_FindImage(cl_hudimage1->string, type);
  if (!strcmp(name, "pics/i_score.pcx"))
    return GL_FindImage(cl_hudimage2->string, type);
  if (!strcmp(name, "pics/i_ammo.pcx"))
    return GL_FindImage(cl_hudimage3->string, type);

  // look for it
  image = GL_GetImage(name);
  if (image != NULL)
    return image;

  // strip off .pcx, .tga, etc...
  COM_StripExtension(name, shortname);

  //
  // load the pic from disk
  //
  pic = NULL;
  palette = NULL;

  // Try to load the image with different file extensions, in the following
  // order of decreasing preference: TGA, JPEG, PCX, and WAL.

  LoadTGA(va("%s.tga", shortname), &pic, &width, &height);
  if (pic) {
    image = GL_LoadPic(name, pic, width, height, type, 32);
    goto done;
  }

  LoadJPG(va("%s.jpg", shortname), &pic, &width, &height);
  if (pic) {
    image = GL_LoadPic(name, pic, width, height, type, 32);
    goto done;
  }

  // TGA and JPEG are the only file types used for heightmaps and
  // normalmaps, so if we haven't found it yet, it isn't there, and we can
  // save ourselves a file lookup.
  if (type == it_bump)
    goto done;

  LoadPCX(va("%s.pcx", shortname), &pic, &palette, &width, &height);
  if (pic) {
    image = GL_LoadPic(name, pic, width, height, type, 8);
    goto done;
  }

  if (type == it_wall)
    image = GL_LoadWal(va("%s.wal", shortname));

done:
  if (pic)
    free(pic);
  if (palette)
    free(palette);

  if (image != NULL)
    image->script = RS_FindScript(shortname);

ret_image:
  return image;
}

image_s *R_RegisterSkin(char *name) { return GL_FindImage(name, it_skin); }

/*
================
GL_FreeUnusedImages

Any image that was not touched on this registration sequence
will be freed.
================
*/
void GL_FreeUnusedImages() {
  // never free r_notexture
  r_notexture->registration_sequence = registration_sequence;

  asset_manager::IterateGroup(
      asset_manager::AssetType::Texture, [&](void *data) {
        image_t *image = static_cast<image_t *>(data);

        if (image->registration_sequence == registration_sequence)
          return; // used this sequence
        if (image->type == it_pic || image->type == it_sprite ||
            image->type == it_particle)
          return; // don't free pics or particles

        // free it
        qglDeleteTextures(1, &image->index);
        image->markAsUnused = true;
      });

  asset_manager::ClearUnused(asset_manager::AssetType::Texture);
}

// TODO: have r_newrefdef available before calling these, put r_mirroretxture
// in that struct, base it on the viewport specified in that struct. (Needed
// for splitscreen.)
void R_InitMirrorTextures(void) {
  byte *data;
  int size;
  int size_oneside;

  // init the partial screen texture
  size_oneside = ceil((512.0f / 1080.0f) * (float)vid.height);
  size = size_oneside * size_oneside * 4;
  data = static_cast<byte *>(malloc(size));
  memset(data, 255, size);
  r_mirrortexture = GL_LoadPic("***r_mirrortexture***", (byte *)data,
                               size_oneside, size_oneside, it_pic, 32);
  free(data);
}

void R_InitDepthTextures(void) {
  byte *data;
  int size, buffersize;
  int bigsize, littlesize;
  int i;

  // init the framebuffer textures
  bigsize = std::max(vid.width, vid.height) * 2 * r_shadowmapscale->value;
  littlesize = std::min(vid.width, vid.height) * r_shadowmapscale->value;
  bigsize = littlesize;
  buffersize = bigsize * bigsize * 4;

  data = static_cast<byte *>(malloc(buffersize));
  memset(data, 255, buffersize);

  for (i = 0; i < deptex_num; i++) {
    Com_sprintf(deptex_names[i], sizeof(deptex_names[i]),
                "***r_depthtexture%d***", i);
    if (i == deptex_sunstatic)
      size = bigsize;
    else
      size = littlesize;
    r_depthtextures[i] =
        GL_LoadPic(deptex_names[i], (byte *)data, size, size, it_pic, 32);
  }

  free(data);
}

void GL_InitImages(void) {

  registration_sequence = 1;

  gl_state.inverse_intensity = 1;

  Draw_GetPalette();

  R_InitMirrorTextures(); // MIRRORS
  R_InitDepthTextures();  // DEPTH(SHADOWMAPS)
  R_FB_InitTextures();    // FULLSCREEN EFFECTS
  R_SI_InitTextures();    // SIMPLE ITEMS
  R_InitBloomTextures();  // BLOOMS
  R_Decals_InitFBO();     // DECALS
}

void GL_ShutdownImages() {
  asset_manager::IterateGroup(asset_manager::AssetType::Texture,
                              [&](void *data) {
                                image_t *image = static_cast<image_t *>(data);
                                qglDeleteTextures(1, &image->index);
                              });
  asset_manager::DestroyGroup(asset_manager::AssetType::Texture);

  memset(scrap_allocated, 0, sizeof(scrap_allocated));
  memset(scrap_texels, 0, sizeof(scrap_texels));
}
}