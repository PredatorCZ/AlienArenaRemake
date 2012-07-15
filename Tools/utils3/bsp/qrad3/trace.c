// trace.c

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "qrad.h"

#define	ON_EPSILON	0.1

typedef struct tnode_s
{
	int		type;
	vec3_t	normal;
	float	dist;
	int		children[2];
	int		children_face[2]; //valid if the corresponding child is a leaf
    int     pad;
} tnode_t;

tnode_t		*tnodes, *tnode_p;

/*
==============
MakeTnode

Converts the disk node structure into the efficient tracing structure
==============
*/
static int tnode_mask;
void MakeTnode (int nodenum)
{
	tnode_t			*t;
	dplane_t		*plane;
	int				i;
	dnode_t 		*node;
	
	t = tnode_p++;

	node = dnodes + nodenum;
	plane = dplanes + node->planenum;

	t->type = plane->type;
	VectorCopy (plane->normal, t->normal);
	t->dist = plane->dist;
	
	for (i=0 ; i<2 ; i++)
	{
		if (node->children[i] < 0)
		{
            t->children[i] = (dleafs[-node->children[i] - 1].contents & tnode_mask) | (1<<31);
            t->children_face[i] = dleafs[-node->children[i] - 1].firstleafface;
		}
		else
		{
			t->children[i] = tnode_p - tnodes;
			MakeTnode (node->children[i]);
		}
	}
}


/*
=============
MakeTnodes

Loads the node structure out of a .bsp file to be used for light occlusion
=============
*/
void MakeTnodes (dmodel_t *bm)
{
	// 32 byte align the structs
	tnodes = malloc( (numnodes+1) * sizeof(tnode_t));
	tnodes = (tnode_t *)(((int)tnodes + 31)&~31);
	tnode_p = tnodes;
	tnode_mask = CONTENTS_SOLID;
	//TODO: or-in CONTENTS_WINDOW in response to a command-line argument

	MakeTnode (0);
}

//==========================================================

/*
=============
BuildPolygons

Applies the same basic algorithm as r_surf.c:BSP_BuildPolygonFromSurface to
every surface in the BSP, precomputing the xyz and st coordinates of each 
vertex of each polygon. Skip non-translucent surfaces for now.
=============
*/
void BuildPolygons (void)
{
	//TODO implement
}

/*
=============
GetSTCoords

Gets the ST coordinates within the texture of the polygon of a 3D point on the
polygon.
=============
*/
void GetSTCoords (int facenum, vec3_t point, int *output)
{
	//TODO implement
	//algorithm idea: http://stackoverflow.com/questions/5766830/texture-coordinates-for-point-in-polygon
	//also look at what's being one in BSP_BuildPolygonFromSurface to get the
	//ST data for each polygon vertex; that might be useful too.
	output[0] = output[1] = 0;
}

/*
=============
GetRGBASample

Gets the RGBA value within the texture of the polygon of a 3D point on the 
polygon.
=============
*/
void GetRGBASample (int facenum, vec3_t point, float *output)
{
	//TODO test this
	int st_coords[2];
	int texture_offset;
	int texnum;
	byte *cur_texture_data;
	int i;
	texnum = dfaces[facenum].texinfo;
	cur_texture_data = texture_data[texnum];
	GetSTCoords (facenum, point, st_coords);
	texture_offset = st_coords[1]*texture_sizes[texnum][0]+st_coords[0]*4;
	for (i = 0; i < 4; i++)
	{
		output[i] = cur_texture_data[i+texture_offset];
	}
}

//==========================================================

int TestLine_r (int node, int node_face, vec3_t set_start, vec3_t stop)
{
	tnode_t	*tnode;
	float	front, back;
	vec3_t	mid, _start;
	vec_t *start;
	float	frac;
	int		side;
	int		r;

    start = set_start;

re_test:

	r = 0;
	if (node & (1<<31))
	{
		if ((r = node & ~(1<<31)) != CONTENTS_WINDOW)
		{
			return r;	// nonzero means occluded, 0 means not occluded
		}
	}

	tnode = &tnodes[node];
	switch (tnode->type)
	{
	case PLANE_X:
		front = start[0] - tnode->dist;
		back = stop[0] - tnode->dist;
		break;
	case PLANE_Y:
		front = start[1] - tnode->dist;
		back = stop[1] - tnode->dist;
		break;
	case PLANE_Z:
		front = start[2] - tnode->dist;
		back = stop[2] - tnode->dist;
		break;
	default:
		front = (start[0]*tnode->normal[0] + start[1]*tnode->normal[1] + start[2]*tnode->normal[2]) - tnode->dist;
		back = (stop[0]*tnode->normal[0] + stop[1]*tnode->normal[1] + stop[2]*tnode->normal[2]) - tnode->dist;
		break;
	}
	
	if (r) //translucent, so check texture
	{		       
        frac = front / (front-back);

		mid[0] = start[0] + (stop[0] - start[0])*frac;
		mid[1] = start[1] + (stop[1] - start[1])*frac;
		mid[2] = start[2] + (stop[2] - start[2])*frac;
		
		/*TODO: open the texture up, check the transparency.
        We should have a vec3_t output argument to this function so we
        can indicate how much of each color is occluded. Here's what I 
        propose:
            float rgba_sample[4]; //range 0..1 on each axis
            get_rgba_sample(node_face, mid, rgba_sample);
            for (i = 0; i < 3; i++) {
                occlusion[i] *= rgba_sample[i]*rgba_sample[3];
                if (occlusion[i] < 0.01)
                    occlusion[i] = 0.0;
            }
            if (VectorLength(occlusion) < 0.1)
                return 1; //occluded
            return 0; //not occluded
        Note that occlusion would start out as {1.0,1.0,1.0} and have each
        color reduced as the trace passed through more translucent 
        textures.
        */
        return 0; //not occluded
    }

	if (front >= -ON_EPSILON && back >= -ON_EPSILON)
	{
        node = tnode->children[0];
        node_face = tnode->children_face[0];
        goto re_test;
	}
	
	if (front < ON_EPSILON && back < ON_EPSILON)
	{
        node = tnode->children[1];
        node_face = tnode->children_face[1];
        goto re_test;
	}

	side = front < 0;

    frac = front / (front-back);

	mid[0] = start[0] + (stop[0] - start[0])*frac;
	mid[1] = start[1] + (stop[1] - start[1])*frac;
	mid[2] = start[2] + (stop[2] - start[2])*frac;


    if (r = TestLine_r (tnode->children[side], tnode->children_face[side], start, mid))
		return r;

    node = tnode->children[!side];
    node = tnode->children_face[!side];

    start = _start;
    start[0] = mid[0];
    start[1] = mid[1];
    start[2] = mid[2];

    goto re_test;
}

int TestLine (vec3_t start, vec3_t stop)
{
	return TestLine_r (0, 0, start, stop);
}

/*
==============================================================================

LINE TRACING

The major lighting operation is a point to point visibility test, performed
by recursive subdivision of the line by the BSP tree.

==============================================================================
*/

typedef struct
{
	vec3_t	backpt;
	int		side;
	int		node;
} tracestack_t;


/*
==============
TestLine
==============
*/
qboolean _TestLine (vec3_t start, vec3_t stop)
{
	int				node;
	float			front, back;
	tracestack_t	*tstack_p;
	int				side;
	float 			frontx,fronty, frontz, backx, backy, backz;
	tracestack_t	tracestack[64];
	tnode_t			*tnode;
	
	frontx = start[0];
	fronty = start[1];
	frontz = start[2];
	backx = stop[0];
	backy = stop[1];
	backz = stop[2];
	
	tstack_p = tracestack;
	node = 0;
	
	while (1)
	{
		if (node == CONTENTS_SOLID)
		{
#if 0
			float	d1, d2, d3;

			d1 = backx - frontx;
			d2 = backy - fronty;
			d3 = backz - frontz;

			if (d1*d1 + d2*d2 + d3*d3 > 1)
#endif
				return false;	// DONE!
		}
		
		while (node < 0)
		{
		// pop up the stack for a back side
			tstack_p--;
			if (tstack_p < tracestack)
				return true;
			node = tstack_p->node;
			
		// set the hit point for this plane
			
			frontx = backx;
			fronty = backy;
			frontz = backz;
			
		// go down the back side

			backx = tstack_p->backpt[0];
			backy = tstack_p->backpt[1];
			backz = tstack_p->backpt[2];
			
			node = tnodes[tstack_p->node].children[!tstack_p->side];
		}

		tnode = &tnodes[node];
		
		switch (tnode->type)
		{
		case PLANE_X:
			front = frontx - tnode->dist;
			back = backx - tnode->dist;
			break;
		case PLANE_Y:
			front = fronty - tnode->dist;
			back = backy - tnode->dist;
			break;
		case PLANE_Z:
			front = frontz - tnode->dist;
			back = backz - tnode->dist;
			break;
		default:
			front = (frontx*tnode->normal[0] + fronty*tnode->normal[1] + frontz*tnode->normal[2]) - tnode->dist;
			back = (backx*tnode->normal[0] + backy*tnode->normal[1] + backz*tnode->normal[2]) - tnode->dist;
			break;
		}

		if (front > -ON_EPSILON && back > -ON_EPSILON)
//		if (front > 0 && back > 0)
		{
			node = tnode->children[0];
			continue;
		}
		
		if (front < ON_EPSILON && back < ON_EPSILON)
//		if (front <= 0 && back <= 0)
		{
			node = tnode->children[1];
			continue;
		}

		side = front < 0;
		
		front = front / (front-back);
	
		tstack_p->node = node;
		tstack_p->side = side;
		tstack_p->backpt[0] = backx;
		tstack_p->backpt[1] = backy;
		tstack_p->backpt[2] = backz;
		
		tstack_p++;
		
		backx = frontx + front*(backx-frontx);
		backy = fronty + front*(backy-fronty);
		backz = frontz + front*(backz-frontz);
		
		node = tnode->children[side];		
	}	
}
