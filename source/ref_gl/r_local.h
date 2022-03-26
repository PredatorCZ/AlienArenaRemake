/*
Copyright (C) 1997-2001 Id Software, Inc.

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

#if defined WIN32_VARIANT
#  include <windows.h>
#endif

#include <stdio.h>

#include <GL/gl.h>
#include <math.h>
#include "glext.h"

#include "client/ref.h"
#include "client/vid.h"

#include "qgl.h"
#include "r_math.h"

#if !defined min
#define min(a,b) (((a)<(b)) ? (a) : (b))
#endif

#if !defined max
#define max(a,b)  (((a)<(b)) ? (b) : (a))
#endif

#if !defined clamp
#define clamp(x,low,high) (min(max(low,x),high))
#endif

// up / down
#define	PITCH	0

// left / right
#define	YAW		1

// fall over
#define	ROLL	2

#define	DLIGHT_CUTOFF	64

extern	viddef_t	vid;

#include "r_image.h"

//===================================================================

typedef enum
{
	rserr_ok,

	rserr_invalid_fullscreen,
	rserr_invalid_mode,

	rserr_unknown
} rserr_t;

#include "r_model.h"

extern float	r_frametime;

void GL_SetDefaultState( void );
void GL_UpdateSwapInterval( void );

extern	float	gldepthmin, gldepthmax;

#define	MAX_LBM_HEIGHT		2048

#define BACKFACE_EPSILON	0.01

extern	entity_t	*currententity;
extern	model_t		*currentmodel;
extern	int			r_visframecount;
extern	int			r_framecount;
extern  qboolean			r_sunShadowsOn; // any sun shadows this frame?
extern  qboolean			r_nonSunStaticShadowsOn; // any non-sun static shadows?
extern	cplane_t	frustum[4];
extern	int			c_brush_polys, c_alias_polys;
extern 	int c_flares;
extern  int c_grasses;
extern	int c_beams;

extern	int			gl_filter_min, gl_filter_max;

//
// screen size info
//
extern	refdef_t	r_newrefdef;
extern	int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;
extern mleaf_t			*r_viewleaf, *r_viewleaf2;

#define DRAWDIST 15000 // TODO: use this constant in more places

// Make sure the furthest possible corner of the skybox is closer than the
// draw distance we supplied to glFrustum. 
#define SKYDIST (sqrt(DRAWDIST*DRAWDIST/3.0)-1.0)

extern	cvar_t	*r_norefresh;
extern	cvar_t	*r_lefthand;
extern	cvar_t	*r_drawentities;
extern	cvar_t	*r_drawworld;
extern	cvar_t	*r_fullbright;
extern	cvar_t	*r_novis;
extern	cvar_t	*r_nocull;
extern	cvar_t	*r_lerpmodels;

extern	cvar_t	*gl_bitdepth;
extern	cvar_t	*gl_mode;
extern	cvar_t	*gl_lightmap;
extern	cvar_t	*gl_dynamic;
extern	cvar_t	*gl_nobind;
extern	cvar_t	*gl_picmip;
extern	cvar_t	*gl_skymip;
extern	cvar_t	*gl_showtris;
extern	cvar_t	*gl_showpolys;
extern	cvar_t	*gl_showcollisionmesh;
extern	cvar_t	*gl_finish;
extern	cvar_t	*gl_clear;
extern	cvar_t	*gl_cull;
extern	cvar_t	*gl_polyblend;
extern	cvar_t	*gl_modulate;
extern	cvar_t	*gl_drawbuffer;
extern	cvar_t	*gl_driver;
extern	cvar_t	*gl_swapinterval;
extern	cvar_t	*gl_texturemode;
extern	cvar_t	*gl_texturealphamode;
extern	cvar_t	*gl_texturesolidmode;
extern	cvar_t	*gl_lockpvs;
extern	cvar_t	*gl_vlights;

extern	cvar_t	*vid_fullscreen;
extern	cvar_t	*vid_preferred_fullscreen;
extern	cvar_t	*vid_gamma;
extern  cvar_t	*vid_contrast;

extern cvar_t *r_anisotropic;
extern cvar_t *r_alphamasked_anisotropic;
extern cvar_t *r_ext_max_anisotropy;
extern cvar_t *r_antialiasing;

extern cvar_t	*r_overbrightbits;

extern cvar_t	*r_meshnormalmaps;
extern cvar_t	*r_worldnormalmaps;
extern cvar_t	*gl_shadowmaps;
extern cvar_t	*gl_fog;

extern	cvar_t	*r_shaders;
extern	cvar_t	*r_bloom;
extern	cvar_t	*r_lensflare;
extern	cvar_t	*r_lensflare_intens;
extern	cvar_t	*r_drawsun;
extern  cvar_t	*r_godrays;
extern  cvar_t	*r_godray_intensity;
extern	cvar_t	*r_optimize;

extern	cvar_t	*r_lightmapfiles;

extern  cvar_t	*r_ragdolls;
extern  cvar_t  *r_ragdoll_debug;

extern	qboolean	map_fog;
#if defined WIN32_VARIANT
extern	char		map_music[MAX_PATH];
#else
extern	char		map_music[MAX_OSPATH];
#endif
extern  unsigned	r_weather;
extern unsigned		r_nosun;
extern float		r_sunX;
extern float		r_sunY;
extern float		r_sunZ;
extern float		r_sunIntens;
extern float		r_skyangleX;
extern float		r_skyangleY;
extern float		r_skyangleZ;

extern  cvar_t		*r_minimap;
extern  cvar_t		*r_minimap_size;
extern  cvar_t		*r_minimap_zoom;
extern  cvar_t		*r_minimap_style;

extern	cvar_t	*gl_mirror;

extern	cvar_t	*sys_affinity;
extern	cvar_t	*sys_priority;

extern	cvar_t	*gl_screenshot_type;
extern	cvar_t	*gl_screenshot_jpeg_quality;

extern  cvar_t  *r_test;

extern cvar_t *r_showpolycounts;
extern cvar_t *gl_showdecals;

extern	int		gl_lightmap_format;
extern	int		gl_solid_format;
extern	int		gl_alpha_format;
extern	int		gl_tex_solid_format;
extern	int		gl_tex_alpha_format;

extern	int		c_visible_lightmaps;
extern	int		c_visible_textures;

extern	float	r_world_matrix[16];
extern	float	r_project_matrix[16];
extern	int		r_viewport[4];
extern	float	r_farclip, r_farclip_min, r_farclip_bias;

extern	int		r_origin_leafnum;

//Image
void R_InitImageSubsystem(void);
void GL_Bind (int texnum);
void GL_MBind (int target, int texnum);
void GL_TexEnv (GLenum mode);
void GL_MTexEnv (int target, GLenum mode);
void GL_EnableTexture (int target, qboolean enable);
void GL_SelectTexture (int target);
void GL_InvalidateTextureState (void);

void vectoangles (vec3_t value1, vec3_t angles);

// dynamic lights
qboolean R_StaticLightPoint (const vec3_t p, vec3_t color);
void R_DynamicLightPoint (const vec3_t p, vec3_t color);
void R_LightPoint (vec3_t p, vec3_t color, qboolean addDynamic);
void R_PushDlights (void);
void R_UpdateDlights (void);
void R_PushDlightsForBModel (entity_t *e);

//====================================================================
extern	model_t	*r_worldmodel;

extern	unsigned	d_8to24table[256];

extern	int		registration_sequence;

void V_AddBlend (float r, float g, float b, float a, float *v_blend);

// "fake" terrain entities parsed directly out of the BSP on the client side.
extern int			num_terrain_entities;
extern entity_t	terrain_entities[MAX_MAP_MODELS];
extern int			num_rock_entities;
extern entity_t	rock_entities[MAX_ROCKS];
extern int			num_decal_entities;
extern entity_t	decal_entities[MAX_MAP_MODELS];

//Renderer main loop
int	R_Init( void *hinstance, void *hWnd );
void R_Shutdown( void );

void R_SetupFog (float distance_boost);
void R_SetupViewport (void);
void R_RenderView (refdef_t *fd);
void GL_ScreenShot_f (void);
void R_Mesh_Draw (entity_t *ent, const model_t *mod);
void R_Mesh_ExtractLightmap (entity_t *ent, const model_t *mod);
void R_DrawBrushModel (void);
void R_DrawWorldSurfs (void);
void R_DrawAlphaSurfaces (void);
void R_InitParticleTexture (void);
void R_DrawParticles (void);
void R_DrawRadar(void);
void R_DrawSkyBox (void);
void Draw_InitLocal (void);

//Instrumentation and visualization (TODO: new file for this?)
void R_DrawMark (vec3_t origin, float size, const float rgba[]);
// for hovering polycount display

typedef struct renderedmodels_s {
	const entity_t *ent;
	const model_t *mod;
} renderedmodels_t;

extern renderedmodels_t rendered_models[MAX_EDICTS+2*MAX_MAP_MODELS+MAX_ROCKS];
extern int num_rendered_models;

//Renderer utils
void R_SubdivideSurface (msurface_t *fa, int firstedge, int numedges);
qboolean R_CullBox (vec3_t mins, vec3_t maxs);
qboolean R_CullOrigin(vec3_t origin);
qboolean R_CullSphere( const vec3_t centre, const float radius, const int clipflags );
void R_RotateForEntity (const entity_t *e);
void R_MarkWorldSurfs (void);
void R_RenderWaterPolys (msurface_t *fa);
void R_ReadFogScript(char config_file[128]);
void R_ReadMusicScript(char config_file[128]);
int SignbitsForPlane (cplane_t *out);

//Lights
extern vec3_t lightspot;
void  VLight_Init (void);
float VLight_GetLightValue ( vec3_t normal, vec3_t dir, float apitch, float ayaw );

//BSP 
extern image_t *r_droplets;
extern image_t *r_droplets_nm;
extern image_t *r_blooddroplets;
extern image_t *r_blooddroplets_nm;
void BSP_DrawTexturelessBrushModel (entity_t *e);
void BSP_InvalidateVBO (void);
void BSP_DrawVBOAccum (void);
void BSP_ClearVBOAccum (void);
void BSP_FlushVBOAccum (void);
void BSP_AddSurfToVBOAccum (msurface_t *surf);
void R_SetLightingMode (void);

//Postprocess
image_t *R_Postprocess_AllocFBOTexture (char *name, int width, int height, GLuint *FBO);
void R_GLSLPostProcess(void);
void R_FB_InitTextures(void);

//VBO
extern GLuint minimap_vboId;
void R_LoadVBOSubsystem (void);
void R_VCShutdown (void);
void VB_WorldVCInit (void);
void VB_BuildWorldVBO (void);
void GL_SetupWorldVBO (void);
void GL_SetupSkyboxVBO (void);
void GL_SetupVegetationVBO (void);
void GL_SetupLensFlareVBO (void);
typedef enum
{
    wholescreen_blank, wholescreen_textured, wholescreen_fliptextured
} wholescreen_drawtype_t;
void GL_SetupWholeScreen2DVBO (wholescreen_drawtype_t drawtype);
void GL_DrawNullModel (void);
void GL_DrawCollisionMesh (void);
#define GL_BindVBO(ptr) qglBindBufferARB (GL_ARRAY_BUFFER_ARB, ptr)
#define GL_BindIBO(ptr) qglBindBufferARB (GL_ELEMENT_ARRAY_BUFFER, ptr)

//Light Bloom
void R_BloomBlend( refdef_t *fd );
void R_InitBloomTextures( void );

//Flares and Sun
extern int r_numflares;
extern int c_flares;
extern flare_t r_flares[MAX_FLARES];
void Mod_AddFlareSurface (msurface_t *surf, int type );
void R_RenderFlares (void);
void R_ClearFlares(void);

extern vec3_t sun_origin;
extern qboolean spacebox;
extern qboolean draw_sun;
extern float sun_x;
extern float sun_y;
extern float sun_size;
extern float sun_alpha;
void R_InitSun();
void R_RenderSun();

//Vegetation
extern int r_numgrasses;
extern int c_grasses;
extern grass_t r_grasses[MAX_GRASSES];
extern qboolean r_hasleaves;
void Mod_AddVegetationSurface (msurface_t *surf, image_t *tex, vec3_t color, float size, char name[MAX_QPATH], int type);
void Mod_AddVegetation (vec3_t origin, vec3_t normal, image_t *tex, vec3_t color, float size, char name[MAX_OSPATH], int type);
void R_DrawVegetationSurface (void);
void R_ClearGrasses(void);
void R_FinalizeGrass(void); 

//Simple Items
void R_SI_InitTextures( void );
void R_DrawSimpleItems( void );
void R_SetSimpleTexnum (model_t *loadmodel, const char *pathname);

//Light beams/volumes
extern int r_numbeams;
extern int c_beams;
extern beam_t r_beams[MAX_BEAMS];
void Mod_AddBeamSurface (msurface_t *surf, int texnum, vec3_t color, float size, char name[MAX_QPATH], int type, float xang, float yang,
	qboolean rotating);
void R_DrawBeamSurface ( void );
void R_ClearBeams(void);

//Player icons
extern float	scr_playericonalpha;

//Team colors
extern int r_teamColor;
extern qboolean r_gotFlag;
extern qboolean r_lostFlag;

void	Draw_GetPicSize (int *w, int *h, const char *name);
void	Draw_Pic (float x, float y, const char *name);
void	Draw_ScaledPic (float x, float y, float scale, const char *pic);
void	Draw_StretchPic (float x, float y, float w, float h, const char *name);
void Draw_AngledScaledPic (float x, float y, float scale, const char *pic, qboolean isNum);

void	R_BeginFrame( float camera_separation );
void	R_SwapBuffers( int );
int	Draw_GetPalette (void);

image_t *R_RegisterSkin (char *name);
image_t *R_RegisterParticlePic(const char *name);
image_t *R_RegisterParticleNormal(const char *name);
image_t *R_RegisterGfxPic(const char *name);

image_t *GL_FindFreeImage (const char *name, int width, int height, imagetype_t type);
image_t *GL_LoadPic (const char *name, byte *pic, int width, int height, imagetype_t type, int bits);
image_t *GL_GetImage (const char * name );
image_t	*GL_FindImage (const char *name, imagetype_t type);
void	GL_FreeImage( image_t * image );
void	GL_TextureMode( char *string );
void	GL_ImageList_f (void);

void	GL_InitImages (void);
void	GL_ShutdownImages (void);

void	GL_FreeUnusedImages (void);

void	GL_TextureAlphaMode( char *string );
void	GL_TextureSolidMode( char *string );

/*
** GL config stuff
*/
#define GLSTATE_DISABLE_ALPHATEST	{ if (gl_state.alpha_test) { qglDisable(GL_ALPHA_TEST); gl_state.alpha_test=false; } }
#define GLSTATE_ENABLE_ALPHATEST	{ if (!gl_state.alpha_test) { qglEnable(GL_ALPHA_TEST); gl_state.alpha_test=true; } }

#define GLSTATE_DISABLE_BLEND		{ if (gl_state.blend) { qglDisable(GL_BLEND); gl_state.blend=false; } }
#define GLSTATE_ENABLE_BLEND		{ if (!gl_state.blend) { qglEnable(GL_BLEND); gl_state.blend=true; } }

typedef struct
{
	const char *renderer_string;
	const char *vendor_string;
	const char *version_string;
	const char *extensions_string;
} glconfig_t;

typedef struct
{
	 float       inverse_intensity;
    qboolean    fullscreen;

    int         prev_mode;

#define MAX_TMUS 16
    int         currenttextures[MAX_TMUS];
    int			currenttexturemodes[MAX_TMUS];
    qboolean	enabledtmus[MAX_TMUS];
    int         currenttmu;
    qboolean	tmuswitch_done;
    qboolean    currenttmu_defined;
	
	GLenum bFunc1;
	GLenum bFunc2;

    float       camera_separation;
    qboolean    stereo_enabled;

    qboolean    alpha_test;
    qboolean    blend;
    qboolean    separateStencil;
    qboolean    stencil_wrap;
    qboolean    fbo;
    qboolean    hasFBOblit;

	qboolean	ati;
	
	qboolean	msaaEnabled;

} glstate_t;

extern glconfig_t  gl_config;
extern glstate_t   gl_state;

// fbo
extern GLuint pp_FBO;

// vertex arrays
#define MAX_ARRAY (MAX_PARTICLES*4)

#define VA_SetElem2(v,a,b)		((v)[0]=(a),(v)[1]=(b))
#define VA_SetElem3(v,a,b,c)	((v)[0]=(a),(v)[1]=(b),(v)[2]=(c))

extern float	tex_array[MAX_ARRAY][2];
extern float	vert_array[MAX_ARRAY][3];

#define MAX_VARRAY_VERTS (MAX_VERTS + 2)
#define MAX_VARRAY_VERTEX_SIZE 11

#define MAX_VERTICES 16384
#define MAX_INDICES	(MAX_VERTICES * 4)
#define MAX_SHADOW_VERTS 16384

extern float VArrayVerts[MAX_VARRAY_VERTS * MAX_VARRAY_VERTEX_SIZE];
extern int VertexSizes[];
extern float *VArray;

// define our vertex types
#define VERT_SINGLE_TEXTURED			0		// verts and st for 1 tmu

// vertex array subsystem
void R_InitVArrays (int varraytype);
void R_KillVArrays (void);
void R_DrawVarrays(GLenum mode, GLint first, GLsizei count);
void R_InitQuadVarrays(void);

void R_TexCoordPointer (int tmu, GLsizei stride, const GLvoid *pointer);
void R_VertexPointer (GLint size, GLsizei stride, const GLvoid *pointer);
void R_NormalPointer (GLsizei stride, const GLvoid *pointer);
void R_AttribPointer (	GLuint index, GLint size, GLenum type,
						GLboolean normalized, GLsizei stride,
						const GLvoid *pointer );

//shadows
extern	cvar_t		*r_shadowmapscale;
extern  int			r_lightgroups;
enum {deptex_dynamic, deptex_sunstatic, deptex_otherstatic, deptex_num};
extern image_t *r_depthtextures[deptex_num];
extern  image_t		*r_colorbuffer;
extern vec3_t	r_worldLightVec;
typedef struct	LightGroup 
{
	vec3_t	group_origin;
	vec3_t	accum_origin;
	float	avg_intensity;
	float	dist;
} LightGroup_t;
extern			LightGroup_t LightGroups[MAX_LIGHTS];

void		R_CheckFBOExtensions (void);
void		R_GenerateShadowFBO(void);
void		R_Mesh_DrawCaster (entity_t *ent, const model_t *mod);
void			R_GenerateGlobalShadows (void);
void		R_GenerateEntityShadow (entity_t *ent, const vec3_t statLightPosition);
extern int				FB_texture_width, FB_texture_height;
extern float			fadeShadow;
extern cvar_t			*r_shadowcutoff;

//shader programs
void	R_LoadGLSLPrograms(void);

#define GLSL_MAX_DLIGHTS 6
#define CUR_NUM_DLIGHTS min (min (r_newrefdef.num_dlights, gl_dynamic->integer), GLSL_MAX_DLIGHTS)

//glsl
extern GLhandleARB g_worldprogramObj[GLSL_MAX_DLIGHTS+1];
extern GLhandleARB g_shadowprogramObj;
extern GLhandleARB g_warpprogramObj;
extern GLhandleARB g_minimapprogramObj;
extern GLhandleARB g_rscriptprogramObj[GLSL_MAX_DLIGHTS+1];
extern GLhandleARB g_waterprogramObj;
extern GLhandleARB g_meshprogramObj[GLSL_MAX_DLIGHTS+1];
extern GLhandleARB g_vertexonlymeshprogramObj[GLSL_MAX_DLIGHTS+1];
extern GLhandleARB g_glassprogramObj;
extern GLhandleARB g_blankmeshprogramObj;
extern GLhandleARB g_extractlightmapmeshprogramObj;
extern GLhandleARB g_fbprogramObj;
extern GLhandleARB g_blurprogramObj;
extern GLhandleARB g_defringeprogramObj;
extern GLhandleARB g_kawaseprogramObj;
extern GLhandleARB g_colorscaleprogramObj;
extern GLhandleARB g_colorexpprogramObj;
extern GLhandleARB g_rblurprogramObj;
extern GLhandleARB g_dropletsprogramObj;
extern GLhandleARB g_DOFprogramObj;
extern GLhandleARB g_godraysprogramObj;
extern GLhandleARB g_vegetationprogramObj;
extern GLhandleARB g_lensflareprogramObj;

extern GLhandleARB g_vertexShader;
extern GLhandleARB g_fragmentShader;

// vertex attribute indexes
// blame NVIDIA for this idiocy:
// http://stackoverflow.com/questions/528028/glvertexattrib-which-attribute-indices-are-predefined

// Mesh rendering
#define ATTR_TANGENT_IDX	1
#define ATTR_WEIGHTS_IDX	11
#define ATTR_BONES_IDX 		12
#define ATTR_OLDVTX_IDX		13
#define ATTR_OLDNORM_IDX	14
#define ATTR_OLDTAN_IDX		15

// Minimap rendering
#define ATTR_MINIMAP_DATA_IDX	1

// Particle rendering
#define ATTR_SWAYCOEF_DATA_IDX  1 // vegetation
#define ATTR_SIZE_DATA_IDX  	1 // lens flare
#define ATTR_ADDUP_DATA_IDX		11
#define	ATTR_ADDRIGHT_DATA_IDX	12

#define MAX_ATTR_IDX		16

// Uniform locations for GLSL shaders that support dynamic lighting
typedef struct
{
	GLuint	lightAmountSquared;
	GLuint	lightCutoffSquared;
	GLuint	lightPosition; // in eye space
} dlight_uniform_location_t;

void R_SetDlightUniforms (dlight_uniform_location_t *uniforms); // See r_light.c

// Uniform locations for GLSL shaders that support shadows being cast on them
typedef struct
{
	GLuint	enabled, pixelOffset, texture;
} shadowmap_channel_uniform_location_t;
typedef struct
{
	shadowmap_channel_uniform_location_t sunStatic, otherStatic, dynamic;
} shadowmap_uniform_location_t;

void R_SetShadowmapUniforms (shadowmap_uniform_location_t *uniforms, int tmu, qboolean enable); // see r_shadowmaps.c

//standard bsp surfaces
typedef struct worldsurfuniforms_s {
	dlight_uniform_location_t dlight_uniforms;
	shadowmap_uniform_location_t shadowmap_uniforms;
	GLuint	surfTexture, heightTexture, lmTexture, normalTexture;
	GLuint	fog, parallax;
	GLuint	staticLightPosition; 
	GLuint	liquid, shiny;
	GLuint	liquidTexture, liquidNormTex, chromeTex;
	GLuint	rsTime;
} worldsurfuniforms_t;


extern worldsurfuniforms_t worldsurf_uniforms[GLSL_MAX_DLIGHTS+1];

//shadow on white bsp polys
typedef struct secondpassbspshadowuniforms_s
{
	shadowmap_uniform_location_t shadowmap_uniforms;
	GLuint fade;
} secondpassbspshadowuniforms_t;

extern secondpassbspshadowuniforms_t secondpass_bsp_shadow_uniforms;
// Old-style per-vertex water effects
typedef struct warpuniforms_s
{
	GLuint	time;
	GLuint	warpvert;
	GLuint	envmap;
} warpuniforms_t;

extern warpuniforms_t warp_uniforms;

//rscripts
typedef struct rscriptuniforms_s
{
	dlight_uniform_location_t	dlight_uniforms;
	shadowmap_uniform_location_t shadowmap_uniforms;
	GLuint						static_normalmaps;
	GLuint						staticLightPosition;
	GLuint						envmap;
	GLuint						numblendtextures, numblendnormalmaps;
	GLuint						lightmap;
	GLuint						fog;
	GLuint						mainTexture, mainTexture2;
	GLuint						lightmapTexture;
	GLuint						blendscales;
	GLuint						blendTexture[6];
	GLuint						normalblendindices;
	GLuint						blendNormalmap[3];
	GLuint						meshPosition;
	GLuint						meshRotation;
} rscriptuniforms_t;
extern rscriptuniforms_t rscript_uniforms[GLSL_MAX_DLIGHTS+1];

//water
extern GLuint		g_location_baseTexture;
extern GLuint		g_location_normTexture;
extern GLuint		g_location_refTexture;
extern GLuint		g_location_time;
extern GLuint		g_location_lightPos;
extern GLuint		g_location_reflect;
extern GLuint		g_location_trans;
extern GLuint		g_location_fogamount;

// All the GLSL shaders that render animated meshes have these uniforms.
typedef struct
{
	// 0 for no animation, 1 for IQM skeletal, 2 for MD2 lerp
	GLuint	useGPUanim; 
	GLuint	outframe;
	GLuint	lerp;
} mesh_anim_uniform_location_t;

// Uniform locations for standard full mesh rendering.
// Most of these will be -1 for the version of the GLSL program that doesn't
// have the fragment shader.
typedef struct
{
	mesh_anim_uniform_location_t	anim_uniforms;
	dlight_uniform_location_t		dlight_uniforms;
	shadowmap_uniform_location_t	shadowmap_uniforms;
	GLuint							staticLightPosition, staticLightColor;
	GLuint							totalLightPosition, totalLightColor;
	GLuint							meshPosition, meshRotation;
	GLuint							baseTex, normTex, fxTex, fx2Tex;
	GLuint							lightmapTexture;
	GLuint							lightmap;
	GLuint							time;
	GLuint							fog;
	GLuint							useFX;
	GLuint							useGlow;
	GLuint							useShell;
	GLuint							shellAlpha;
	GLuint							useCube;
	GLuint							fromView;
	GLuint							doShading;
	GLuint							team;
} mesh_uniform_location_t;

extern mesh_uniform_location_t	mesh_uniforms[GLSL_MAX_DLIGHTS+1], mesh_vertexonly_uniforms[GLSL_MAX_DLIGHTS+1];

// Uniform locations for glass rendering.
typedef struct glassuniform_s
{
	mesh_anim_uniform_location_t	anim_uniforms;
	// 1 means mirror only, 2 means glass only, 3 means both
	GLuint							type;
	GLuint							left, up;
	GLuint							mirTexture, refTexture;
	GLuint							lightPos;
	GLuint							fog;
} glassuniform_t;
extern glassuniform_t glass_uniforms;

typedef struct
{
	mesh_anim_uniform_location_t	anim_uniforms;
	GLuint							baseTex;
} mesh_blank_uniform_location_t;

// Uniform locations for blank meshes.
extern mesh_blank_uniform_location_t blankmesh_uniforms;

// Uniform locations for dumping out the static lighting into a lightmap texture
typedef struct meshextractlightmapuniforms_s
{
	mesh_anim_uniform_location_t	anim_uniforms;
	GLuint							staticLightPosition, staticLightColor;
} meshextractlightmapuniforms_t;
extern meshextractlightmapuniforms_t mesh_extract_lightmap_uniforms;

// fullscreen distortion effects
typedef struct distortuniforms_s
{
	GLuint	framebuffTex, distortTex;
	GLuint	intensity;
} distortuniforms_t;

extern distortuniforms_t distort_uniforms;

// color scale shader, gaussian blur, kawase filter, defringe
typedef struct 
{
	GLuint  scale, source;
} scuniforms_t;
extern scuniforms_t colorscale_uniforms, gaussian_uniforms, defringe_uniforms, kawase_uniforms;

// color exponentiation shader
typedef struct 
{
	GLuint  exponent, source;
} esuniforms_t;
extern esuniforms_t colorexp_uniforms;

// radial blur	
extern GLuint		g_location_rsource;
extern GLuint		g_location_rparams;

// water droplets
extern GLuint		g_location_drSource;
extern GLuint		g_location_drTex;
extern GLuint		g_location_drTime;

// DOF
extern GLuint		g_location_dofSource;
extern GLuint		g_location_dofDepth;

// god rays
extern GLuint		g_location_lightPositionOnScreen;
extern GLuint		g_location_sunTex;
extern GLuint		g_location_godrayScreenAspect;
extern GLuint		g_location_sunRadius;

// vegetation
typedef struct
{
	GLuint	rsTime;
	GLuint	up, right;
} vegetationuniforms_t;
extern vegetationuniforms_t vegetation_uniforms;

// lens flare
typedef struct
{
	GLuint	up, right;
} lensflareuniforms_t;
extern lensflareuniforms_t lensflare_uniforms;

// Shared mesh items
extern image_t	*r_mirrortexture;
extern cvar_t	*cl_gun;
void R_ModelViewTransform(const vec3_t in, vec3_t out);
void GL_BlendFunction (GLenum sfactor, GLenum dfactor);

// iqm
qboolean Mod_INTERQUAKEMODEL_Load(model_t *mod, void *buffer);
qboolean IQM_InAnimGroup(int frame, int oldframe);
void IQM_DrawFrame(int skinnum, qboolean ragdoll, float shellAlpha);
void IQM_AnimateFrame (const entity_t *ent, const model_t *mod, matrix3x4_t outframe[SKELETAL_MAX_BONEMATS]);

// md2
void Mod_LoadMD2Model (model_t *mod, void *buffer);
void MD2_SelectFrame (entity_t *ent, const model_t *mod);

// terrain
void Mod_LoadTerrainModel (model_t *mod, void *_buf);
// vegetation, rocks/pebbles, etc.
void Mod_LoadTerrainDecorations (char *path, vec3_t angles, vec3_t origin);

// terrain decals
void R_Decals_InitFBO (void);
void Mod_LoadDecalModel (model_t *mod, void *_buf);
void R_ParseDecalEntity (char *match, char *block);

// GL Extensions
qboolean GL_QueryExtension(const char *extension_name);
void GL_PrintExtensions(void);
void GL_Strings_f(void);

/*
====================================================================

IMPLEMENTATION SPECIFIC FUNCTIONS

====================================================================
*/

void		GLimp_BeginFrame( float camera_separation );
void		GLimp_EndFrame( void );
qboolean	GLimp_Init( void *hinstance, void *hWnd );
void		GLimp_Shutdown( void );
rserr_t	GLimp_SetMode( unsigned *pwidth, unsigned *pheight, int mode, windowmode_t windowmode );
void		GLimp_AppActivate( qboolean active );

/*
====================================================================

IMPORTED FUNCTIONS

====================================================================
*/

#include "r_script.h"
