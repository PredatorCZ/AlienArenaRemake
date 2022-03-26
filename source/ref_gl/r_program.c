/*
Copyright (C) 2009-2014 COR Entertainment, LLC

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
// r_program.c

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"

//GLSL
PFNGLCREATEPROGRAMOBJECTARBPROC		glCreateProgramObjectARB	= NULL;
PFNGLDELETEOBJECTARBPROC			glDeleteObjectARB			= NULL;
PFNGLUSEPROGRAMOBJECTARBPROC		glUseProgramObjectARB		= NULL;
PFNGLCREATESHADEROBJECTARBPROC		glCreateShaderObjectARB		= NULL;
PFNGLSHADERSOURCEARBPROC			glShaderSourceARB			= NULL;
PFNGLCOMPILESHADERARBPROC			glCompileShaderARB			= NULL;
PFNGLGETOBJECTPARAMETERIVARBPROC	glGetObjectParameterivARB	= NULL;
PFNGLATTACHOBJECTARBPROC			glAttachObjectARB			= NULL;
PFNGLGETINFOLOGARBPROC				glGetInfoLogARB				= NULL;
PFNGLLINKPROGRAMARBPROC				glLinkProgramARB			= NULL;
PFNGLGETUNIFORMLOCATIONARBPROC		glGetUniformLocationARB		= NULL;
PFNGLUNIFORM4IARBPROC				glUniform4iARB				= NULL;
PFNGLUNIFORM4FARBPROC				glUniform4fARB				= NULL;
PFNGLUNIFORM3FARBPROC				glUniform3fARB				= NULL;
PFNGLUNIFORM2FARBPROC				glUniform2fARB				= NULL;
PFNGLUNIFORM1IARBPROC				glUniform1iARB				= NULL;
PFNGLUNIFORM1FARBPROC				glUniform1fARB				= NULL;
PFNGLUNIFORM4IVARBPROC				glUniform4ivARB				= NULL;
PFNGLUNIFORM4FVARBPROC				glUniform4fvARB				= NULL;
PFNGLUNIFORM3FVARBPROC				glUniform3fvARB				= NULL;
PFNGLUNIFORM2FVARBPROC				glUniform2fvARB				= NULL;
PFNGLUNIFORM1IVARBPROC				glUniform1ivARB				= NULL;
PFNGLUNIFORM1FVARBPROC				glUniform1fvARB				= NULL;
PFNGLUNIFORMMATRIX3FVARBPROC		glUniformMatrix3fvARB		= NULL;
PFNGLUNIFORMMATRIX3X4FVARBPROC		glUniformMatrix3x4fvARB		= NULL;
PFNGLVERTEXATTRIBPOINTERARBPROC	 glVertexAttribPointerARB	= NULL;
PFNGLENABLEVERTEXATTRIBARRAYARBPROC glEnableVertexAttribArrayARB = NULL;
PFNGLDISABLEVERTEXATTRIBARRAYARBPROC glDisableVertexAttribArrayARB = NULL;
PFNGLBINDATTRIBLOCATIONARBPROC		glBindAttribLocationARB		= NULL;
PFNGLGETSHADERINFOLOGPROC			glGetShaderInfoLog			= NULL;

// Used for dumping internal assembly of GLSL programs. Purely for developer
// use, end-users need not have this OpenGL extension supported.
/*#define DUMP_GLSL_ASM*/
#ifdef DUMP_GLSL_ASM
static void (*glGetProgramBinary) (GLuint program, GLsizei bufsize, GLsizei *length, GLenum *binaryFormat, void *binary) = NULL;
#endif

typedef struct {
	const char	*name;
	int			index;
} vertex_attribute_t;

// add new vertex attributes here
#define NO_ATTRIBUTES			0
const vertex_attribute_t standard_attributes[] = 
{
	#define	ATTRIBUTE_TANGENT	(1<<0)
	{"tangent",		ATTR_TANGENT_IDX},
	#define	ATTRIBUTE_WEIGHTS	(1<<1)
	{"weights",		ATTR_WEIGHTS_IDX},
	#define ATTRIBUTE_BONES		(1<<2)
	{"bones",		ATTR_BONES_IDX},
	#define ATTRIBUTE_OLDVTX	(1<<3)
	{"oldvertex",	ATTR_OLDVTX_IDX},
	#define ATTRIBUTE_OLDNORM	(1<<4)
	{"oldnormal",	ATTR_OLDNORM_IDX},
	#define ATTRIBUTE_OLDTAN	(1<<5)
	{"oldtangent",	ATTR_OLDTAN_IDX},
	#define ATTRIBUTE_MINIMAP	(1<<6)
	{"colordata",	ATTR_MINIMAP_DATA_IDX},
	#define ATTRIBUTE_SWAYCOEF	(1<<7)
	{"swaycoef",	ATTR_SWAYCOEF_DATA_IDX},
	#define ATTRIBUTE_ADDUP		(1<<8)
	{"addup",		ATTR_ADDUP_DATA_IDX},
	#define ATTRIBUTE_ADDRIGHT	(1<<9)
	{"addright",	ATTR_ADDRIGHT_DATA_IDX},
	#define ATTRIBUTE_SIZE		(1<<10)
	{"size",		ATTR_SIZE_DATA_IDX},
};
const int num_standard_attributes = sizeof(standard_attributes)/sizeof(vertex_attribute_t);
	
static void R_LoadGLSLProgram (const char *name, char *vertex_library, char *fragment_library, int attributes, int ndynamic, GLhandleARB *program)
{
	char		static_buffer[0x4000], header[64];
	char *currentShaderCode;
	const char	*shaderStrings[5];
	int			nResult;
	int			i;

	Com_sprintf (header, sizeof (header), "#version 120\n#define DYNAMIC %d\n", ndynamic);
	shaderStrings[0] = header;

	*program = glCreateProgramObjectARB();

	if (vertex_library != NULL)
	{
		if (FS_LoadFile_TryStatic(va("shaders/%s.vert", name), &currentShaderCode, static_buffer, sizeof(static_buffer)) < 1) {
			Com_Error (ERR_FATAL, "Cannot load shaders/%s.vert\n", name);
		}
		g_vertexShader = glCreateShaderObjectARB (GL_VERTEX_SHADER_ARB);

		shaderStrings[1] = vertex_library;
		if (fragment_library == NULL)
			shaderStrings[2] = "const int vertexOnly = 1;";
		else
			shaderStrings[2] = "const int vertexOnly = 0;";
		shaderStrings[3] = currentShaderCode;

		glShaderSourceARB (g_vertexShader, 4, shaderStrings, NULL);
		glCompileShaderARB (g_vertexShader);
		glGetObjectParameterivARB (g_vertexShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult);

		if (nResult)
		{
			glAttachObjectARB (*program, g_vertexShader);
		}
		else
		{
			Com_Printf ("...%s_%ddynamic Vertex Shader Compile Error\n", name, ndynamic);
			if (glGetShaderInfoLog != NULL)
			{
				glGetShaderInfoLog (g_vertexShader, sizeof(static_buffer), NULL, static_buffer);
				Com_Printf ("%s\n", static_buffer);
			}
		}

		if (currentShaderCode != static_buffer) {
			FS_FreeFile(currentShaderCode);
		}
	}

	if (fragment_library != NULL)
	{
		if (FS_LoadFile_TryStatic(va("shaders/%s.frag", name), &currentShaderCode, static_buffer, sizeof(static_buffer)) < 1) {
			Com_Error (ERR_FATAL, "Cannot load shaders/%s.frag\n", name);
		}

		g_fragmentShader = glCreateShaderObjectARB( GL_FRAGMENT_SHADER_ARB );

		if (gl_state.ati)
			shaderStrings[1] = "#define AMD_GPU\n#define shadowsampler_t sampler2D\n";
		else
			shaderStrings[1] = "#define shadowsampler_t sampler2DShadow\n";
		shaderStrings[2] = fragment_library;
		shaderStrings[3] = currentShaderCode;

		glShaderSourceARB (g_fragmentShader, 4, shaderStrings, NULL);
		glCompileShaderARB (g_fragmentShader);
		glGetObjectParameterivARB( g_fragmentShader, GL_OBJECT_COMPILE_STATUS_ARB, &nResult );

		if (nResult)
		{
			glAttachObjectARB (*program, g_fragmentShader);
		}
		else
		{
			Com_Printf ("...%s_%ddynamic Fragment Shader Compile Error\n", name, ndynamic);
			if (glGetShaderInfoLog != NULL)
			{
				glGetShaderInfoLog (g_fragmentShader, sizeof(static_buffer), NULL, static_buffer);
				Com_Printf ("%s\n", static_buffer);
			}
		}

		if (currentShaderCode != static_buffer) {
			FS_FreeFile(currentShaderCode);
		}
	}

	for (i = 0; i < num_standard_attributes; i++)
	{
		if (attributes & (1<<i))
			glBindAttribLocationARB (*program, standard_attributes[i].index, standard_attributes[i].name);
	}

	glLinkProgramARB (*program);
	glGetObjectParameterivARB (*program, GL_OBJECT_LINK_STATUS_ARB, &nResult);

	glGetInfoLogARB (*program, sizeof(static_buffer), NULL, static_buffer);
	if (!nResult)
	{
		Com_Printf("...%s_%d Shader Linking Error\n%s\n", name, ndynamic, static_buffer);
	}
#ifdef DUMP_GLSL_ASM
	else
	{
		char	binarydump[1<<16];
		GLsizei	binarydump_length;
		GLenum	format;
		FILE	*out;
		
		// use this to get the actual assembly
		// for i in *dump_*.dat ; do strings "$i" > "$i".txt ; rm "$i" ; done
		Com_sprintf (str, sizeof (str), "dump_%s_%d.dat", name, ndynamic);
		out = fopen (str, "wb");
		
		glGetProgramBinary (*program, sizeof(binarydump), &binarydump_length, &format, binarydump);
		fwrite (binarydump, 1, sizeof(binarydump), out);
		
		fclose (out); 
	}
#endif
}

static void get_dlight_uniform_locations (GLhandleARB programObj, dlight_uniform_location_t *out)
{
	out->lightAmountSquared = glGetUniformLocationARB (programObj, "lightAmount");
	out->lightPosition = glGetUniformLocationARB (programObj, "lightPosition");
	out->lightCutoffSquared = glGetUniformLocationARB (programObj, "lightCutoffSquared");
}

static void get_mesh_anim_uniform_locations (GLhandleARB programObj, mesh_anim_uniform_location_t *out)
{
	out->useGPUanim = glGetUniformLocationARB (programObj, "GPUANIM");
	out->outframe = glGetUniformLocationARB (programObj, "bonemats");
	out->lerp = glGetUniformLocationARB (programObj, "lerp");
}

static void get_shadowmap_channel_uniform_locations (GLhandleARB programObj, shadowmap_channel_uniform_location_t *out, const char *prefix)
{
	char name[64];
	
	Com_sprintf (name, sizeof (name), "%s_enabled", prefix);
	out->enabled = glGetUniformLocationARB (programObj, name);
	
	Com_sprintf (name, sizeof (name), "%s_pixelOffset", prefix);
	out->pixelOffset = glGetUniformLocationARB (programObj, name);
	
	Com_sprintf (name, sizeof (name), "%s_texture", prefix);
	out->texture = glGetUniformLocationARB (programObj, name);
}

static void get_shadowmap_uniform_locations (GLhandleARB programObj, shadowmap_uniform_location_t *out)
{
	get_shadowmap_channel_uniform_locations (programObj, &out->sunStatic, "sunstatic");
	get_shadowmap_channel_uniform_locations (programObj, &out->otherStatic, "otherstatic");
	get_shadowmap_channel_uniform_locations (programObj, &out->dynamic, "dynamic");
}

static void get_mesh_uniform_locations (GLhandleARB programObj, mesh_uniform_location_t *out)
{
	get_mesh_anim_uniform_locations (programObj, &out->anim_uniforms);
	get_dlight_uniform_locations (programObj, &out->dlight_uniforms);
	get_shadowmap_uniform_locations (programObj, &out->shadowmap_uniforms);
	out->staticLightPosition = glGetUniformLocationARB (programObj, "staticLightPosition");
	out->staticLightColor = glGetUniformLocationARB (programObj, "staticLightColor");
	out->totalLightPosition = glGetUniformLocationARB (programObj, "totalLightPosition");
	out->totalLightColor = glGetUniformLocationARB (programObj, "totalLightColor");
	out->meshPosition = glGetUniformLocationARB (programObj, "meshPosition");
	out->meshRotation = glGetUniformLocationARB (programObj, "meshRotation");
	out->baseTex = glGetUniformLocationARB (programObj, "baseTex");
	out->normTex = glGetUniformLocationARB (programObj, "normalTex");
	out->fxTex = glGetUniformLocationARB (programObj, "fxTex");
	out->fx2Tex = glGetUniformLocationARB (programObj, "fx2Tex");
	out->lightmapTexture = glGetUniformLocationARB (programObj, "lightmapTexture");
	out->time = glGetUniformLocationARB (programObj, "time");
	out->lightmap = glGetUniformLocationARB (programObj, "lightmap");
	out->fog = glGetUniformLocationARB (programObj, "FOG");
	out->useFX = glGetUniformLocationARB (programObj, "useFX");
	out->useGlow = glGetUniformLocationARB (programObj, "useGlow");
	out->useShell = glGetUniformLocationARB (programObj, "useShell");
	out->shellAlpha = glGetUniformLocationARB (programObj, "shellAlpha");
	out->useCube = glGetUniformLocationARB (programObj, "useCube");
	out->fromView = glGetUniformLocationARB (programObj, "fromView");
	out->doShading = glGetUniformLocationARB (programObj, "doShading");
	out->team = glGetUniformLocationARB (programObj, "TEAM");
}

void R_LoadGLSLPrograms(void)
{
	int i, j;
	
	//load glsl (to do - move to own file)
	if ( GL_QueryExtension("GL_ARB_shader_objects") )
	{
		glCreateProgramObjectARB  = (PFNGLCREATEPROGRAMOBJECTARBPROC)qwglGetProcAddress("glCreateProgramObjectARB");
		glDeleteObjectARB		 = (PFNGLDELETEOBJECTARBPROC)qwglGetProcAddress("glDeleteObjectARB");
		glUseProgramObjectARB	 = (PFNGLUSEPROGRAMOBJECTARBPROC)qwglGetProcAddress("glUseProgramObjectARB");
		glCreateShaderObjectARB   = (PFNGLCREATESHADEROBJECTARBPROC)qwglGetProcAddress("glCreateShaderObjectARB");
		glShaderSourceARB		 = (PFNGLSHADERSOURCEARBPROC)qwglGetProcAddress("glShaderSourceARB");
		glCompileShaderARB		= (PFNGLCOMPILESHADERARBPROC)qwglGetProcAddress("glCompileShaderARB");
		glGetObjectParameterivARB = (PFNGLGETOBJECTPARAMETERIVARBPROC)qwglGetProcAddress("glGetObjectParameterivARB");
		glAttachObjectARB		 = (PFNGLATTACHOBJECTARBPROC)qwglGetProcAddress("glAttachObjectARB");
		glGetInfoLogARB		   = (PFNGLGETINFOLOGARBPROC)qwglGetProcAddress("glGetInfoLogARB");
		glLinkProgramARB		  = (PFNGLLINKPROGRAMARBPROC)qwglGetProcAddress("glLinkProgramARB");
		glGetUniformLocationARB   = (PFNGLGETUNIFORMLOCATIONARBPROC)qwglGetProcAddress("glGetUniformLocationARB");
		glUniform4iARB			= (PFNGLUNIFORM4IARBPROC)qwglGetProcAddress("glUniform4iARB");
		glUniform4fARB			= (PFNGLUNIFORM4FARBPROC)qwglGetProcAddress("glUniform4fARB");
		glUniform3fARB			= (PFNGLUNIFORM3FARBPROC)qwglGetProcAddress("glUniform3fARB");
		glUniform2fARB			= (PFNGLUNIFORM2FARBPROC)qwglGetProcAddress("glUniform2fARB");
		glUniform1iARB			= (PFNGLUNIFORM1IARBPROC)qwglGetProcAddress("glUniform1iARB");
		glUniform1fARB		  = (PFNGLUNIFORM1FARBPROC)qwglGetProcAddress("glUniform1fARB");
		glUniform4ivARB			= (PFNGLUNIFORM4IVARBPROC)qwglGetProcAddress("glUniform4ivARB");
		glUniform4fvARB			= (PFNGLUNIFORM4FVARBPROC)qwglGetProcAddress("glUniform4fvARB");
		glUniform3fvARB			= (PFNGLUNIFORM3FVARBPROC)qwglGetProcAddress("glUniform3fvARB");
		glUniform2fvARB			= (PFNGLUNIFORM2FVARBPROC)qwglGetProcAddress("glUniform2fvARB");
		glUniform1ivARB			= (PFNGLUNIFORM1IVARBPROC)qwglGetProcAddress("glUniform1ivARB");
		glUniform1fvARB		  = (PFNGLUNIFORM1FVARBPROC)qwglGetProcAddress("glUniform1fvARB");
		glUniformMatrix3fvARB	  = (PFNGLUNIFORMMATRIX3FVARBPROC)qwglGetProcAddress("glUniformMatrix3fvARB");
		glUniformMatrix3x4fvARB	  = (PFNGLUNIFORMMATRIX3X4FVARBPROC)qwglGetProcAddress("glUniformMatrix3x4fv");
		glVertexAttribPointerARB = (PFNGLVERTEXATTRIBPOINTERARBPROC)qwglGetProcAddress("glVertexAttribPointerARB");
		glEnableVertexAttribArrayARB = (PFNGLENABLEVERTEXATTRIBARRAYARBPROC)qwglGetProcAddress("glEnableVertexAttribArrayARB");
		glDisableVertexAttribArrayARB = (PFNGLDISABLEVERTEXATTRIBARRAYARBPROC)qwglGetProcAddress("glDisableVertexAttribArrayARB");
		glBindAttribLocationARB = (PFNGLBINDATTRIBLOCATIONARBPROC)qwglGetProcAddress("glBindAttribLocationARB");
		glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)qwglGetProcAddress("glGetShaderInfoLog");

#ifdef DUMP_GLSL_ASM
		glGetProgramBinary = qwglGetProcAddress("glGetProgramBinary");
#endif

		if( !glCreateProgramObjectARB || !glDeleteObjectARB || !glUseProgramObjectARB ||
			!glCreateShaderObjectARB || !glCreateShaderObjectARB || !glCompileShaderARB ||
			!glGetObjectParameterivARB || !glAttachObjectARB || !glGetInfoLogARB ||
			!glLinkProgramARB || !glGetUniformLocationARB || !glUniform3fARB ||
				!glUniform4fARB || !glUniform4fvARB || !glUniform4ivARB ||
				!glUniform4iARB || !glUniform1iARB || !glUniform1fARB ||
				!glUniform3fvARB || !glUniform2fvARB || !glUniform1fvARB ||
				!glUniformMatrix3fvARB || !glUniformMatrix3x4fvARB ||
				!glVertexAttribPointerARB || !glEnableVertexAttribArrayARB ||
				!glBindAttribLocationARB)
		{
			Com_Error (ERR_FATAL, "...One or more GL_ARB_shader_objects functions were not found\n");
		}
	}
	else
	{
		Com_Error (ERR_FATAL, "...One or more GL_ARB_shader_objects functions were not found\n");
	}

	gl_dynamic = Cvar_Get ("gl_dynamic", "1", CVAR_ARCHIVE);

	char *commonVertex;
	char vertexStatic[0x4000];
	if (FS_LoadFile_TryStatic("shaders/common.vert", &commonVertex, vertexStatic, sizeof(vertexStatic)) < 1) {
		Com_Error (ERR_FATAL, "Cannot load shaders/common.vert\n");
	}

	char *commonFragment;
	char fragmentStatic[0x4000];
	if (FS_LoadFile_TryStatic("shaders/common.frag", &commonFragment, fragmentStatic, sizeof(fragmentStatic)) < 1) {
		Com_Error (ERR_FATAL, "Cannot load shaders/common.frag\n");
	}

	//standard bsp surfaces
	for (i = 0; i <= GLSL_MAX_DLIGHTS; i++)
	{
		R_LoadGLSLProgram ("world", commonVertex, commonFragment, ATTRIBUTE_TANGENT, i, &g_worldprogramObj[i]);

		get_dlight_uniform_locations (g_worldprogramObj[i], &worldsurf_uniforms[i].dlight_uniforms);
		get_shadowmap_uniform_locations (g_worldprogramObj[i], &worldsurf_uniforms[i].shadowmap_uniforms);
		worldsurf_uniforms[i].surfTexture = glGetUniformLocationARB (g_worldprogramObj[i], "surfTexture");
		worldsurf_uniforms[i].heightTexture = glGetUniformLocationARB (g_worldprogramObj[i], "HeightTexture");
		worldsurf_uniforms[i].lmTexture = glGetUniformLocationARB (g_worldprogramObj[i], "lmTexture");
		worldsurf_uniforms[i].normalTexture = glGetUniformLocationARB (g_worldprogramObj[i], "NormalTexture");
		worldsurf_uniforms[i].fog = glGetUniformLocationARB (g_worldprogramObj[i], "FOG");
		worldsurf_uniforms[i].parallax = glGetUniformLocationARB (g_worldprogramObj[i], "PARALLAX");
		worldsurf_uniforms[i].staticLightPosition = glGetUniformLocationARB (g_worldprogramObj[i], "staticLightPosition");
		worldsurf_uniforms[i].liquid = glGetUniformLocationARB (g_worldprogramObj[i], "LIQUID");
		worldsurf_uniforms[i].shiny = glGetUniformLocationARB (g_worldprogramObj[i], "SHINY");
		worldsurf_uniforms[i].rsTime = glGetUniformLocationARB (g_worldprogramObj[i], "rsTime");
		worldsurf_uniforms[i].liquidTexture = glGetUniformLocationARB (g_worldprogramObj[i], "liquidTexture");
		worldsurf_uniforms[i].liquidNormTex = glGetUniformLocationARB (g_worldprogramObj[i], "liquidNormTex");
		worldsurf_uniforms[i].chromeTex = glGetUniformLocationARB (g_worldprogramObj[i], "chromeTex");
	}

	//shadowed white bsp surfaces
	R_LoadGLSLProgram ("shadow", commonVertex, commonFragment, NO_ATTRIBUTES, 0, &g_shadowprogramObj);

	// Locate some parameters by name so we can set them later...
	secondpass_bsp_shadow_uniforms.fade = glGetUniformLocationARB( g_shadowprogramObj, "fadeShadow" );
	get_shadowmap_uniform_locations (g_shadowprogramObj, &secondpass_bsp_shadow_uniforms.shadowmap_uniforms);
	
	// Old-style per-vertex water effects
	R_LoadGLSLProgram ("warp", commonVertex, NULL, NO_ATTRIBUTES, 0, &g_warpprogramObj);
	
	warp_uniforms.time = glGetUniformLocationARB (g_warpprogramObj, "time");
	warp_uniforms.warpvert = glGetUniformLocationARB (g_warpprogramObj, "warpvert");
	warp_uniforms.envmap = glGetUniformLocationARB (g_warpprogramObj, "envmap");
	
	// Minimaps
	R_LoadGLSLProgram ("minimap", commonVertex, NULL, ATTRIBUTE_MINIMAP, 0, &g_minimapprogramObj);
	
	//rscript surfaces
	for (i = 0; i <= GLSL_MAX_DLIGHTS; i++)
	{
		R_LoadGLSLProgram ("rscript", commonVertex, commonFragment, ATTRIBUTE_TANGENT, i, &g_rscriptprogramObj[i]);
	
		get_dlight_uniform_locations (g_rscriptprogramObj[i], &rscript_uniforms[i].dlight_uniforms);
		get_shadowmap_uniform_locations (g_rscriptprogramObj[i], &rscript_uniforms[i].shadowmap_uniforms);
		rscript_uniforms[i].staticLightPosition = glGetUniformLocationARB (g_rscriptprogramObj[i], "staticLightPosition");
		rscript_uniforms[i].envmap = glGetUniformLocationARB (g_rscriptprogramObj[i], "envmap");
		rscript_uniforms[i].numblendtextures = glGetUniformLocationARB (g_rscriptprogramObj[i], "numblendtextures");
		rscript_uniforms[i].numblendnormalmaps = glGetUniformLocationARB (g_rscriptprogramObj[i], "numblendnormalmaps");
		rscript_uniforms[i].static_normalmaps = glGetUniformLocationARB (g_rscriptprogramObj[i], "static_normalmaps");
		rscript_uniforms[i].lightmap = glGetUniformLocationARB (g_rscriptprogramObj[i], "lightmap");
		rscript_uniforms[i].fog = glGetUniformLocationARB (g_rscriptprogramObj[i], "FOG");
		rscript_uniforms[i].mainTexture = glGetUniformLocationARB (g_rscriptprogramObj[i], "mainTexture");
		rscript_uniforms[i].mainTexture2 = glGetUniformLocationARB (g_rscriptprogramObj[i], "mainTexture2");
		rscript_uniforms[i].lightmapTexture = glGetUniformLocationARB (g_rscriptprogramObj[i], "lightmapTexture");
		rscript_uniforms[i].blendscales = glGetUniformLocationARB (g_rscriptprogramObj[i], "blendscales");
		rscript_uniforms[i].normalblendindices = glGetUniformLocationARB (g_rscriptprogramObj[i], "normalblendindices");
		rscript_uniforms[i].meshPosition = glGetUniformLocationARB (g_rscriptprogramObj[i], "meshPosition");
		rscript_uniforms[i].meshRotation = glGetUniformLocationARB (g_rscriptprogramObj[i], "meshRotation");

		for (j = 0; j < 6; j++)
		{
			char uniformname[] = "blendTexture.";
		
			assert (j < 10); // We only have space for one digit.
			uniformname[12] = '0'+j;
			rscript_uniforms[i].blendTexture[j] = glGetUniformLocationARB (g_rscriptprogramObj[i], uniformname);
		}
		
		for (j = 0; j < 3; j++)
		{
			char uniformname[] = "blendNormalmap.";
		
			assert (j < 10); // We only have space for one digit.
			uniformname[14] = '0'+j;
			rscript_uniforms[i].blendNormalmap[j] = glGetUniformLocationARB (g_rscriptprogramObj[i], uniformname);
		}
	}

	// per-pixel warp(water) bsp surfaces
	R_LoadGLSLProgram ("water", commonVertex, commonFragment, ATTRIBUTE_TANGENT, 0, &g_waterprogramObj);

	// Locate some parameters by name so we can set them later...
	g_location_baseTexture = glGetUniformLocationARB( g_waterprogramObj, "baseTexture" );
	g_location_normTexture = glGetUniformLocationARB( g_waterprogramObj, "normalMap" );
	g_location_refTexture = glGetUniformLocationARB( g_waterprogramObj, "refTexture" );
	g_location_time = glGetUniformLocationARB( g_waterprogramObj, "time" );
	g_location_lightPos = glGetUniformLocationARB( g_waterprogramObj, "LightPos" );
	g_location_reflect = glGetUniformLocationARB( g_waterprogramObj, "REFLECT" );
	g_location_trans = glGetUniformLocationARB( g_waterprogramObj, "TRANSPARENT" );
	g_location_fogamount = glGetUniformLocationARB( g_waterprogramObj, "FOG" );

	for (i = 0; i <= GLSL_MAX_DLIGHTS; i++)
	{
		//meshes
		R_LoadGLSLProgram (
			"mesh", commonVertex, commonFragment,
			ATTRIBUTE_TANGENT|ATTRIBUTE_WEIGHTS|ATTRIBUTE_BONES|ATTRIBUTE_OLDVTX|ATTRIBUTE_OLDNORM|ATTRIBUTE_OLDTAN,
			i, &g_meshprogramObj[i]
		);
	
		get_mesh_uniform_locations (g_meshprogramObj[i], &mesh_uniforms[i]);

		//vertex-only meshes
		R_LoadGLSLProgram (
			"mesh", commonVertex, NULL,
			ATTRIBUTE_TANGENT|ATTRIBUTE_WEIGHTS|ATTRIBUTE_BONES|ATTRIBUTE_OLDVTX|ATTRIBUTE_OLDNORM|ATTRIBUTE_OLDTAN,
			i, &g_vertexonlymeshprogramObj[i]
		);
	
		get_mesh_uniform_locations (g_vertexonlymeshprogramObj[i], &mesh_vertexonly_uniforms[i]);
	}
	
	//Glass
	R_LoadGLSLProgram ("glass", commonVertex, commonFragment, ATTRIBUTE_WEIGHTS|ATTRIBUTE_BONES|ATTRIBUTE_OLDVTX|ATTRIBUTE_OLDNORM, 0, &g_glassprogramObj);

	// Locate some parameters by name so we can set them later...
	get_mesh_anim_uniform_locations (g_glassprogramObj, &glass_uniforms.anim_uniforms);
	glass_uniforms.fog = glGetUniformLocationARB (g_glassprogramObj, "FOG");
	glass_uniforms.type = glGetUniformLocationARB (g_glassprogramObj, "type");
	glass_uniforms.left = glGetUniformLocationARB (g_glassprogramObj, "left");
	glass_uniforms.up = glGetUniformLocationARB (g_glassprogramObj, "up");
	glass_uniforms.lightPos = glGetUniformLocationARB (g_glassprogramObj, "LightPos");
	glass_uniforms.mirTexture = glGetUniformLocationARB (g_glassprogramObj, "mirTexture");
	glass_uniforms.refTexture = glGetUniformLocationARB (g_glassprogramObj, "refTexture");

	//Blank mesh (for shadowmapping efficiently)
	R_LoadGLSLProgram ("blank", commonVertex, commonFragment, ATTRIBUTE_WEIGHTS|ATTRIBUTE_BONES|ATTRIBUTE_OLDVTX, 0, &g_blankmeshprogramObj);

	// Locate some parameters by name so we can set them later...
	get_mesh_anim_uniform_locations (g_blankmeshprogramObj, &blankmesh_uniforms.anim_uniforms);
	blankmesh_uniforms.baseTex = glGetUniformLocationARB (g_blankmeshprogramObj, "baseTex");

	//Per-pixel static lightmapped mesh rendered into texture space
	R_LoadGLSLProgram ("mesh_extract_lightmap", commonVertex, commonFragment, ATTRIBUTE_TANGENT|ATTRIBUTE_WEIGHTS|ATTRIBUTE_BONES|ATTRIBUTE_OLDVTX|ATTRIBUTE_OLDNORM|ATTRIBUTE_OLDTAN, 0, &g_extractlightmapmeshprogramObj);
	
	get_mesh_anim_uniform_locations (g_extractlightmapmeshprogramObj, &mesh_extract_lightmap_uniforms.anim_uniforms);
	mesh_extract_lightmap_uniforms.staticLightPosition = glGetUniformLocationARB (g_extractlightmapmeshprogramObj, "staticLightPosition");
	mesh_extract_lightmap_uniforms.staticLightColor = glGetUniformLocationARB (g_extractlightmapmeshprogramObj, "staticLightColor");
	
	
	//fullscreen distortion effects
	R_LoadGLSLProgram ("framebuffer_distortion", commonVertex, commonFragment, NO_ATTRIBUTES, 0, &g_fbprogramObj);

	// Locate some parameters by name so we can set them later...
	distort_uniforms.framebuffTex = glGetUniformLocationARB (g_fbprogramObj, "fbtexture");
	distort_uniforms.distortTex = glGetUniformLocationARB (g_fbprogramObj, "distortiontexture");
	distort_uniforms.intensity = glGetUniformLocationARB (g_fbprogramObj, "intensity");

	//gaussian blur
	R_LoadGLSLProgram ("blur", commonVertex, commonFragment, NO_ATTRIBUTES, 0, &g_blurprogramObj);

	// Locate some parameters by name so we can set them later...
	gaussian_uniforms.scale = glGetUniformLocationARB( g_blurprogramObj, "ScaleU" );
	gaussian_uniforms.source = glGetUniformLocationARB( g_blurprogramObj, "textureSource");

	//defringe filter (transparent-only blur)
	R_LoadGLSLProgram ("defringe", commonVertex, commonFragment, NO_ATTRIBUTES, 0, &g_defringeprogramObj);

	// Locate some parameters by name so we can set them later...
	defringe_uniforms.scale = glGetUniformLocationARB (g_defringeprogramObj, "ScaleU");
	defringe_uniforms.source = glGetUniformLocationARB (g_defringeprogramObj, "textureSource");

	//kawase filter blur
	R_LoadGLSLProgram ("kawase", commonVertex, commonFragment, NO_ATTRIBUTES, 0, &g_kawaseprogramObj);

	// Locate some parameters by name so we can set them later...
	kawase_uniforms.scale = glGetUniformLocationARB( g_blurprogramObj, "ScaleU" );
	kawase_uniforms.source = glGetUniformLocationARB( g_blurprogramObj, "textureSource");
	
	// Color scaling
	R_LoadGLSLProgram ("colorscale", NULL, commonFragment, NO_ATTRIBUTES, 0, &g_colorscaleprogramObj);
	colorscale_uniforms.scale = glGetUniformLocationARB (g_colorscaleprogramObj, "scale");
	colorscale_uniforms.source = glGetUniformLocationARB (g_colorscaleprogramObj, "textureSource");
	
	// Color exponentiation
	R_LoadGLSLProgram ("color_exponent", NULL, commonFragment, NO_ATTRIBUTES, 0, &g_colorexpprogramObj);
	colorexp_uniforms.exponent = glGetUniformLocationARB (g_colorexpprogramObj, "exponent");
	colorexp_uniforms.source = glGetUniformLocationARB (g_colorexpprogramObj, "textureSource");
	
	// Radial blur
	R_LoadGLSLProgram ("rblur", commonVertex, commonFragment, NO_ATTRIBUTES, 0, &g_rblurprogramObj);

	// Locate some parameters by name so we can set them later...
	g_location_rsource = glGetUniformLocationARB( g_rblurprogramObj, "rtextureSource");
	g_location_rparams = glGetUniformLocationARB( g_rblurprogramObj, "radialBlurParams");

	// Water droplets
	R_LoadGLSLProgram ("droplets", commonVertex, commonFragment, NO_ATTRIBUTES, 0, &g_dropletsprogramObj);

	// Locate some parameters by name so we can set them later...
	g_location_drSource = glGetUniformLocationARB( g_dropletsprogramObj, "drSource" );
	g_location_drTex = glGetUniformLocationARB( g_dropletsprogramObj, "drTex");
	g_location_drTime = glGetUniformLocationARB( g_dropletsprogramObj, "drTime" );

	// Depth of field
	R_LoadGLSLProgram("dof", commonVertex, commonFragment, NO_ATTRIBUTES, 0, &g_DOFprogramObj);

	// Locate some parameters by name so we can set them later...
	g_location_dofSource = glGetUniformLocationARB(g_DOFprogramObj, "renderedTexture");
	g_location_dofDepth = glGetUniformLocationARB(g_DOFprogramObj, "depthTexture");

	// God rays
	R_LoadGLSLProgram ("godrays", commonVertex, commonFragment, NO_ATTRIBUTES, 0, &g_godraysprogramObj);

	// Locate some parameters by name so we can set them later...
	g_location_lightPositionOnScreen = glGetUniformLocationARB( g_godraysprogramObj, "lightPositionOnScreen" );
	g_location_sunTex = glGetUniformLocationARB( g_godraysprogramObj, "sunTexture");
	g_location_godrayScreenAspect = glGetUniformLocationARB( g_godraysprogramObj, "aspectRatio");
	g_location_sunRadius = glGetUniformLocationARB( g_godraysprogramObj, "sunRadius");
	
	// Vegetation
	R_LoadGLSLProgram ("vegetation", commonVertex, NULL, ATTRIBUTE_SWAYCOEF|ATTRIBUTE_ADDUP|ATTRIBUTE_ADDRIGHT, 0, &g_vegetationprogramObj);
	
	vegetation_uniforms.rsTime = glGetUniformLocationARB (g_vegetationprogramObj, "rsTime");
	vegetation_uniforms.up = glGetUniformLocationARB (g_vegetationprogramObj, "up");
	vegetation_uniforms.right = glGetUniformLocationARB (g_vegetationprogramObj, "right");
	
	// Lens flare
	R_LoadGLSLProgram ("lensflare", commonVertex, NULL, ATTRIBUTE_SIZE|ATTRIBUTE_ADDUP|ATTRIBUTE_ADDRIGHT, 0, &g_lensflareprogramObj);
	
	lensflare_uniforms.up = glGetUniformLocationARB (g_lensflareprogramObj, "up");
	lensflare_uniforms.right = glGetUniformLocationARB (g_lensflareprogramObj, "right");

	if (commonVertex != vertexStatic) {
		FS_FreeFile(commonVertex);
	}

	if (commonFragment != fragmentStatic) {
		FS_FreeFile(commonFragment);
	}
}
