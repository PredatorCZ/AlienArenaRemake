#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"

void Terrain_LoadVBO (model_t *mod, float *vposition, float *vnormal, float *vtangent, float *vtexcoord, unsigned int *vtriangles)
{
	R_VCLoadData(VBO_STATIC, mod->numvertexes*sizeof(vec2_t), vtexcoord, VBO_STORE_ST, mod);
	R_VCLoadData(VBO_STATIC, mod->numvertexes*sizeof(vec3_t), vposition, VBO_STORE_XYZ, mod);
	R_VCLoadData(VBO_STATIC, mod->numvertexes*sizeof(vec3_t), vnormal, VBO_STORE_NORMAL, mod);
	R_VCLoadData(VBO_STATIC, mod->numvertexes*sizeof(vec4_t), vtangent, VBO_STORE_TANGENT, mod);
	R_VCLoadData(VBO_STATIC, mod->num_triangles*3*sizeof(unsigned int), vtriangles, VBO_STORE_INDICES, mod);
}

void Mod_LoadTerrainModel (model_t *mod, void *_buf)
{
	int i;
	float *vnormal, *vtangent;
	image_t	*tex = NULL;
	terraindata_t data;
	vec3_t up;
	int ndownward;
	
	LoadTerrainFile (&data, mod->name, false, 2.0, 32, (char *)_buf);
	
	mod->numvertexes = data.num_vertices;
	mod->num_triangles = data.num_triangles;
	
	tex = GL_FindImage (data.texture_path, it_wall);
	Z_Free (data.texture_path);
	
	if (data.vegetation != NULL)
		Z_Free (data.vegetation);
	
	if (!tex)
		Com_Error (ERR_DROP, "Mod_LoadTerrainModel: Missing surface texture in %s!", mod->name);
	
	if (data.lightmap_path != NULL)
	{
		mod->lightmap = GL_FindImage (data.lightmap_path, it_wall);
		Z_Free (data.lightmap_path);
	}
	
	VectorCopy (data.mins, mod->mins);
	VectorCopy (data.maxs, mod->maxs);
	
	mod->skins[0] = tex;
	mod->type = mod_terrain;
	
	if (mod->skins[0] != NULL)
		mod->script = mod->skins[0]->script;
	if (mod->script)
		RS_ReadyScript( mod->script );
	
	for ( i = 0; i < 8; i++ )
	{
		vec3_t   tmp;

		if ( i & 1 )
			tmp[0] = mod->mins[0];
		else
			tmp[0] = mod->maxs[0];

		if ( i & 2 )
			tmp[1] = mod->mins[1];
		else
			tmp[1] = mod->maxs[1];

		if ( i & 4 )
			tmp[2] = mod->mins[2];
		else
			tmp[2] = mod->maxs[2];

		VectorCopy( tmp, mod->bbox[i] );
	}
	
	vnormal = Z_Malloc (mod->numvertexes*sizeof(vec3_t));
	vtangent = Z_Malloc (mod->numvertexes*sizeof(vec4_t));
	
	VectorSet (up, 0, 0, 1);
	ndownward = 0;
	
	// Calculate normals and tangents
	for (i = 0; i < mod->num_triangles; i++)
	{
		int j;
		vec3_t v1, v2, normal;
		unsigned int *triangle = &data.tri_indices[3*i];
		
		VectorSubtract (&data.vert_positions[3*triangle[0]], &data.vert_positions[3*triangle[1]], v1);
		VectorSubtract (&data.vert_positions[3*triangle[2]], &data.vert_positions[3*triangle[1]], v2);
		CrossProduct (v2, v1, normal);
		VectorScale (normal, -1.0/VectorLength(normal), normal);
		
		if (DotProduct (normal, up) < 0)
			ndownward++;
		
		for (j = 0; j < 3; j++)
			VectorAdd (&vnormal[3*triangle[j]], normal, &vnormal[3*triangle[j]]);
	}
	
	if (ndownward > 0)
		Com_Printf ("WARN: %d downward facing polygons in %s!\n", ndownward, mod->name);
	
	// Normalize the average normals and tangents
	for (i = 0; i < mod->numvertexes; i++)
		VectorNormalize (&vnormal[3*i]);
	
	Terrain_LoadVBO (mod, data.vert_positions, vnormal, vtangent, data.vert_texcoords, data.tri_indices);
	
	Z_Free (data.vert_positions);
	Z_Free (data.vert_texcoords);
	Z_Free (vnormal);
	Z_Free (vtangent);
	Z_Free (data.tri_indices);
}

void Mod_LoadTerrainVegetation (char *path, vec3_t angles, vec3_t origin)
{
	char *buf;
	int len;
	int i;
	terraindata_t data;
	vec3_t up = {0, 0, 1};
	vec3_t color = {1, 1, 1};
	
	len = FS_LoadFile (path, (void**)&buf);
	
	if (!buf)
	{
		Com_Printf ("WARN: Could not find %s\n", path);
		return;
	}
	
	LoadTerrainFile (&data, path, true, 0, 0, buf);
	
	FS_FreeFile ((void *)buf);
	
	if (data.vegetation == NULL)
		return;
	
	// TODO: transformed terrain model support!
	for (i = 0; i < data.num_vegetation; i++)
	{
		vec3_t org;
		
		VectorAdd (origin, data.vegetation[i].origin, org);
		
		Mod_AddVegetation (org, up, GL_FindImage ("gfx/grass.tga", it_wall)->texnum, color, data.vegetation[i].size, "gfx/grass.tga", 0);
	}
	
	Z_Free (data.vegetation);
}
