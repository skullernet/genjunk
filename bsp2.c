#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include "bsp2.h"

#define MAP_RADIUS  64

// lightmaps
#define SURF_SIZE   (((MAP_RADIUS * 2) >> 4) + 1)
#define SURF_AREA   (SURF_SIZE * SURF_SIZE)
#define SURF_BYTES  (SURF_AREA * 3)

#define COUNT_ENTSTRING     128
#define COUNT_PLANES        6
#define COUNT_VERTICES      8
#define COUNT_VISIBILITY    (sizeof(bsp2vis_t) + BSP2_MIN_CLUSTERS * 2)
#define COUNT_NODES         6
#define COUNT_TEXINFO       (6 + 1)
#define COUNT_FACES         (6 + 1)
#define COUNT_LIGHTMAP      (6 * SURF_BYTES)
#define COUNT_LEAVES        (6 + 1)
#define COUNT_LEAFFACES     6
#define COUNT_LEAFBRUSHES   6
#define COUNT_EDGES         24
#define COUNT_SURFEDGES     24
#define COUNT_MODELS        2
#define COUNT_BRUSHES       6
#define COUNT_BRUSHSIDES    6
#define COUNT_POP           1
#define COUNT_AREAS         2
#define COUNT_AREAPORTALS   1

#define LUMPS \
    L(char,             entstring,      ENTSTRING  ) \
    L(bsp2plane_t,      planes,         PLANES     ) \
    L(bsp2vertex_t,     vertices,       VERTICES   ) \
    L(uint8_t,          visibility,     VISIBILITY ) \
    L(bsp2node_t,       nodes,          NODES      ) \
    L(bsp2texinfo_t,    texinfo,        TEXINFO    ) \
    L(bsp2face_t,       faces,          FACES      ) \
    L(uint8_t,          lightmap,       LIGHTMAP   ) \
    L(bsp2leaf_t,       leaves,         LEAVES     ) \
    L(uint16_t,         leaffaces,      LEAFFACES  ) \
    L(uint16_t,         leafbrushes,    LEAFBRUSHES) \
    L(bsp2edge_t,       edges,          EDGES      ) \
    L(uint32_t,         surfedges,      SURFEDGES  ) \
    L(bsp2model_t,      models,         MODELS     ) \
    L(bsp2brush_t,      brushes,        BRUSHES    ) \
    L(bsp2brushside_t,  brushsides,     BRUSHSIDES ) \
    L(uint32_t,         pop,            POP        ) \
    L(bsp2area_t,       areas,          AREAS      ) \
    L(bsp2areaportal_t, areaportals,    AREAPORTALS)

#define L(type, name1, name2) \
    static type bsp_##name1[COUNT_##name2];

LUMPS

#undef L

static bsp2header_t bsp_header, bsp_header_copy;
static size_t bsp_size, bsp_junk;

typedef struct {
    void *data;
    size_t disksize;
    size_t memcount;
    size_t maxcount;
} mlump_t;

static const mlump_t bsp_lumps[] = {
#define L(type, name1, name2) \
    { &bsp_##name1, sizeof(type), COUNT_##name2, BSP2_MAX_##name2 },

    LUMPS

#undef L
};

static const int8_t vertices[24] = {
    -1, -1, -1, -1, -1, 1, -1, 1, 1, -1, 1, -1, 1, 1, -1, 1, 1, 1, 1, -1, 1, 1, -1, -1
};

static const uint8_t indices[24] = {
    0, 1, 2, 3, 4, 5, 6, 7, 7, 6, 1, 0, 3, 2, 5, 4, 3, 4, 7, 0, 5, 2, 1, 6
};

static void init_bsp(void)
{
    int             i, j;
    int             side, axis;
    bsp2leaf_t      *l;
    bsp2node_t      *c;
    bsp2plane_t     *p;
    bsp2brushside_t *s;
    bsp2face_t      *f;
    bsp2vertex_t    *v;
    bsp2edge_t      *e;
    bsp2vis_t       *vis;
    bsp2texinfo_t   *t;
    bsp2brush_t     *b;
    bsp2model_t     *m;
    uint8_t         *light;
    size_t          ofs, len;

    bsp_pop[0] = 0xdeadf00d;

    memset(bsp_entstring, 0, sizeof(bsp_entstring));
    strcpy(bsp_entstring,
           "{ classname worldspawn } "
           "{ classname info_player_start }");

    // visibility
    vis = (bsp2vis_t *)bsp_visibility;
    vis->numclusters = 1;
    for (i = 0; i < BSP2_MIN_CLUSTERS; i++) {
        vis->offsets[i][0] = sizeof(bsp2vis_t) + i * 2 + 0;
        vis->offsets[i][1] = sizeof(bsp2vis_t) + i * 2 + 1;
        bsp_visibility[sizeof(bsp2vis_t) + i * 2 + 0] = 0xff;
        bsp_visibility[sizeof(bsp2vis_t) + i * 2 + 1] = 0xff;
    }

    // texinfo
    for (i = 0; i < COUNT_TEXINFO; i++) {
        t = &bsp_texinfo[i];
        strcpy(t->name, "e1u1/metal6_1");
        t->next = -1;
        t->flags = 0;
        t->value = 0;
    }

    // south/north
    for (i = 0; i < 2; i++) {
        t = &bsp_texinfo[i];
        t->vecs[0][0] = 0;
        t->vecs[0][1] = 1;
        t->vecs[0][2] = 0;
        t->vecs[0][3] = 0;
        t->vecs[1][0] = 0;
        t->vecs[1][1] = 0;
        t->vecs[1][2] = -1;
        t->vecs[1][3] = 0;
    }

    // east/west
    for (i = 2; i < 4; i++) {
        t = &bsp_texinfo[i];
        t->vecs[0][0] = 1;
        t->vecs[0][1] = 0;
        t->vecs[0][2] = 0;
        t->vecs[0][3] = 0;
        t->vecs[1][0] = 0;
        t->vecs[1][1] = 0;
        t->vecs[1][2] = -1;
        t->vecs[1][3] = 0;
    }

    // floor/ceiling
    for (i = 4; i < 6; i++) {
        t = &bsp_texinfo[i];
        t->vecs[0][0] = 1;
        t->vecs[0][1] = 0;
        t->vecs[0][2] = 0;
        t->vecs[0][3] = 0;
        t->vecs[1][0] = 0;
        t->vecs[1][1] = -1;
        t->vecs[1][2] = 0;
        t->vecs[1][3] = 0;
    }

    // transparent
    t = &bsp_texinfo[6];
    t->flags |= 0x10; // transparent 0.33

    // vertices
    for (i = 0; i < 8; i++) {
        v = &bsp_vertices[i];
        v->point[0] = MAP_RADIUS * vertices[i * 3 + 0];
        v->point[1] = MAP_RADIUS * vertices[i * 3 + 1];
        v->point[2] = MAP_RADIUS * vertices[i * 3 + 2];
    }

    // edges
    for (i = 0; i < 24; i++) {
        e = &bsp_edges[i];
        e->vertices[0] = indices[i];
        e->vertices[1] = 0;
    }

    // surf edges
    for (i = 0; i < 24; i++) {
        bsp_surfedges[i] = i;
    }

    // lightmaps
    light = bsp_lightmap;
    for (i = 0; i < SURF_AREA; i++) {
        light[0] = 0xff;
        light[1] = 0;
        light[2] = 0;
        light += 3;
    }
    for (i = 0; i < SURF_AREA; i++) {
        light[0] = 0;
        light[1] = 0xff;
        light[2] = 0;
        light += 3;
    }
    for (i = 0; i < SURF_AREA; i++) {
        light[0] = 0;
        light[1] = 0;
        light[2] = 0xff;
        light += 3;
    }
    for (i = 0; i < SURF_AREA; i++) {
        light[0] = 0xff;
        light[1] = 0xff;
        light[2] = 0;
        light += 3;
    }
    for (i = 0; i < SURF_AREA; i++) {
        light[0] = 0;
        light[1] = 0xff;
        light[2] = 0xff;
        light += 3;
    }
    for (i = 0; i < SURF_AREA; i++) {
        light[0] = 0xff;
        light[1] = 0;
        light[2] = 0xff;
        light += 3;
    }

    for (i = 0; i < 6; i++) {
        side = i & 1;
        axis = i >> 1;

        // planes
        p = &bsp_planes[i];
        if (side) {
            p->type = 3 + axis;
            p->normal[axis] = -1;
        } else {
            p->type = axis;
            p->normal[axis] = 1;
        }
        p->dist = -MAP_RADIUS;

        // brush sides
        s = &bsp_brushsides[i];
        s->planenum = i;
        s->texinfo = 0;

        // brushes
        b = &bsp_brushes[i];
        b->numsides = 1;
        b->firstside = i;
        b->contents = 1; // solid

        // leaf brushes
        bsp_leafbrushes[i] = i;

        // faces
        f = &bsp_faces[i];
        f->planenum = i;
        f->planeside = 0;
        f->firstedge = i * 4;
        f->numedges = 4;
        f->texinfo = i;
        f->styles[0] = 0;
        f->styles[1] = 255;
        f->styles[2] = 255;
        f->styles[3] = 255;
        f->lightmap = i * SURF_BYTES;

        // leaf faces
        bsp_leaffaces[i] = i;

        // solid leaves
        l = &bsp_leaves[i];
        l->contents = 1; // solid
        l->cluster = -1;
        l->area = 0;
        for (j = 0; j < 3; j++) {
            l->mins[j] = 0;
            l->maxs[j] = 0;
        }
        l->firstface = 0;
        l->numfaces = 0;
        l->firstbrush = i;
        l->numbrushes = 1;

        // nodes
        c = &bsp_nodes[i];
        c->planenum = i;
        c->children[1] = ~i; // solid leaf
        if (i != 5)
            c->children[0] = i + 1;
        else
            c->children[0] = ~6; // empty leaf
        for (j = 0; j < 3; j++) {
            c->mins[j] = -MAP_RADIUS;
            c->maxs[j] = MAP_RADIUS;
        }
        c->firstface = i;
        c->numfaces = 1;
    }

    // empty leaf
    l = &bsp_leaves[6];
    l->contents = 0;
    l->cluster = 0;
    l->area = 1;
    for (i = 0; i < 3; i++) {
        l->mins[i] = -MAP_RADIUS;
        l->maxs[i] = MAP_RADIUS;
    }
    l->firstface = 0;
    l->numfaces = 6;
    l->firstbrush = 0;
    l->numbrushes = 0;

    // submodel face
    f = &bsp_faces[6];
    f->planenum = 0;
    f->planeside = 0;
    f->firstedge = 0;
    f->numedges = 4;
    f->texinfo = 6; // transparent
    f->styles[0] = 255;
    f->styles[1] = 255;
    f->styles[2] = 255;
    f->styles[3] = 255;
    f->lightmap = -1;

    // main model
    m = &bsp_models[0];
    for (i = 0; i < 3; i++) {
        m->mins[i] = -MAP_RADIUS;
        m->maxs[i] = MAP_RADIUS;
        m->origin[i] = 0;
    }
    m->headnode = 0;
    m->firstface = 0;
    m->numfaces = 6;

    // submodel
    m = &bsp_models[1];
    for (i = 0; i < 3; i++) {
        m->mins[i] = -MAP_RADIUS;
        m->maxs[i] = MAP_RADIUS;
        m->origin[i] = 0;
    }
    m->headnode = ~6; // empty leaf
    m->firstface = 6;
    m->numfaces = 1;

    // areas
    bsp_areas[0].numportals = 0;
    bsp_areas[0].firstportal = 0;
    bsp_areas[1].numportals = 1;
    bsp_areas[1].firstportal = 0;

    // area portals
    bsp_areaportals[0].portalnum = 0;
    bsp_areaportals[0].otherarea = 0;

    // header
    bsp_header.magic = BSP2_MAGIC;
    bsp_header.version = BSP2_VERSION;

    ofs = sizeof(bsp_header);
    for (i = 0; i < BSP2_LUMP_TOTAL; i++) {
        len = bsp_lumps[i].disksize * bsp_lumps[i].memcount;
        bsp_header.lumps[i].offset = ofs;
        bsp_header.lumps[i].length = len;
        ofs += (len + 3) & ~3;
    }

    bsp_header_copy = bsp_header;

    bsp_size = ofs;
    bsp_junk = 0;
}

static void write_bsp(FILE *fp)
{
    int i;

    fwrite(&bsp_header, 1, sizeof(bsp_header), fp);
    for (i = 0; i < BSP2_LUMP_TOTAL; i++) {
        fseek(fp, bsp_header_copy.lumps[i].offset, SEEK_SET);
        fwrite(bsp_lumps[i].data, 1, bsp_header_copy.lumps[i].length, fp);
    }

    if (bsp_junk) {
        uint8_t block[64];

        memset(block, 0xff, 64);
        fseek(fp, bsp_size, SEEK_SET);
        for (i = 0; i < bsp_junk; i++)
            fwrite(block, 1, 64, fp);
    }
}

static void header_badofs(void)
{
    int i;

    for (i = 0; i < BSP2_LUMP_TOTAL; i++) {
        bsp_header.lumps[i].offset = INT32_MAX;
    }
}

static void header_badlen(void)
{
    size_t len;
    int i;

    for (i = 0; i < BSP2_LUMP_TOTAL; i++) {
        len = bsp_lumps[i].disksize * bsp_lumps[i].maxcount;
        bsp_header.lumps[i].length = len;
    }
}

// combine badofs and badlen to cause signed integer overflow
static void header_overflow(void)
{
    size_t len;
    int i;

    for (i = 0; i < BSP2_LUMP_TOTAL; i++) {
        len = bsp_lumps[i].disksize * bsp_lumps[i].maxcount;
        bsp_header.lumps[i].offset = INT32_MAX;
        bsp_header.lumps[i].length = len;
    }
}

static void header_misalign(void)
{
    int i;

    for (i = 0; i < BSP2_LUMP_TOTAL; i++) {
        bsp_header.lumps[i].offset++;
        bsp_header_copy.lumps[i].offset++;
    }
}

static void vis_badnumclusters(void)
{
    bsp2vis_t *vis;

    vis = (bsp2vis_t *)bsp_visibility;
    vis->numclusters = INT32_MAX;
}

static void vis_badoffsets(void)
{
    bsp2vis_t *vis;
    int i;

    vis = (bsp2vis_t *)bsp_visibility;
    for (i = 0; i < BSP2_MIN_CLUSTERS; i++) {
        vis->offsets[i][0] = INT32_MAX;
        vis->offsets[i][1] = INT32_MAX;
    }
}

// stack overwrite in CM_DecompressVis
static void vis_overflow(void)
{
    bsp2vis_t *vis;

    vis = (bsp2vis_t *)bsp_visibility;
    vis->numclusters = BSP2_MAX_LEAVES + 8192;
}

// unlimited recursion in BSP_SetParent
static void nodes_loop(void)
{
    bsp_nodes[3].children[0] = 2;
}

static void nodes_badplanenum(void)
{
    int i;

    for (i = 0; i < 6; i++) {
        bsp_nodes[i].planenum = INT32_MAX / sizeof(bsp2plane_t);
    }
}

static void nodes_badchildren(void)
{
    int i;

    for (i = 0; i < 6; i++) {
        bsp_nodes[i].children[i & 1] = INT32_MAX;
        bsp_nodes[i].children[(i & 1) ^ 1] = INT32_MIN;
    }
}

static void nodes_badfirstface(void)
{
    int i;

    for (i = 0; i < 6; i++) {
        bsp_nodes[i].firstface = INT16_MAX;
    }
}

static void nodes_badnumfaces(void)
{
    int i;

    for (i = 0; i < 6; i++) {
        bsp_nodes[i].numfaces = INT16_MAX;
    }
}

static void leaves_badcluster(void)
{
    int i;

    for (i = 0; i < COUNT_LEAVES; i++) {
        bsp_leaves[i].cluster = INT16_MAX;
    }
}

static void leaves_badarea(void)
{
    int i;

    for (i = 0; i < COUNT_LEAVES; i++) {
        bsp_leaves[i].area = INT16_MAX;
    }
}

static void leaves_badfirstbrush(void)
{
    int i;

    for (i = 0; i < 6; i++) {
        bsp_leaves[i].firstbrush = INT16_MAX;
    }
}

static void leaves_badnumbrushes(void)
{
    int i;

    for (i = 0; i < 6; i++) {
        bsp_leaves[i].numbrushes = INT16_MAX;
    }
}

static void leaves_badfirstface(void)
{
    // corrupt empty leaf
    bsp_leaves[6].firstface = INT16_MAX;
}

static void leaves_badnumfaces(void)
{
    // corrupt empty leaf
    bsp_leaves[6].numfaces = INT16_MAX;
}

// crash engines that differentiate nodes from
// leafs by checking for contents == -1
static void leaves_badcontents(void)
{
    int i;

    // don't touch leaf 0 (solid leaf)
    for (i = 1; i < 6; i++) {
        bsp_leaves[i].contents = 0xffffffff;
    }
}

static void brushes_badfirstside(void)
{
    int i;

    for (i = 0; i < 6; i++) {
        bsp_brushes[i].firstside = INT32_MAX;
    }
}

static void brushes_badnumsides(void)
{
    int i;

    for (i = 0; i < 6; i++) {
        bsp_brushes[i].numsides = INT32_MAX;
    }
}

static void leafbrushes_badindices(void)
{
    int i;

    for (i = 0; i < 6; i++) {
        bsp_leafbrushes[i] = INT16_MAX;
    }
}

static void leaffaces_badindices(void)
{
    int i;

    for (i = 0; i < 6; i++) {
        bsp_leaffaces[i] = INT16_MAX;
    }
}

static void surfedges_badindices(void)
{
    int i;

    for (i = 0; i < 24; i++) {
        bsp_surfedges[i] = (i & 1) ? INT32_MIN : INT32_MAX;
    }
}

static void edges_badindices(void)
{
    int i;

    for (i = 0; i < 24; i++) {
        bsp_edges[i].vertices[0] = INT16_MAX;
        bsp_edges[i].vertices[1] = INT16_MAX;
    }
}

static void faces_badfirstedge(void)
{
    int i;

    for (i = 0; i < 6; i++) {
        bsp_faces[i].firstedge = INT32_MAX - 4;
    }
}

static void faces_toomanyedges(void)
{
    int i;

    for (i = 0; i < 6; i++) {
        bsp_faces[i].numedges = INT16_MAX;
    }
}

static void faces_toofewedges(void)
{
    int i;

    for (i = 0; i < 6; i++) {
        bsp_faces[i].numedges = 2;
    }
}

static void faces_badplanenum(void)
{
    int i;

    for (i = 0; i < 6; i++) {
        bsp_faces[i].planenum = INT16_MAX;
    }
}

static void faces_badtexinfo(void)
{
    int i;

    for (i = 0; i < 6; i++) {
        bsp_faces[i].texinfo = INT16_MAX;
    }
}

static void faces_badlightmap(void)
{
    int i;

    for (i = 0; i < 6; i++) {
        bsp_faces[i].lightmap = INT32_MAX;
    }
}

static void brushsides_badplanenum(void)
{
    int i;

    for (i = 0; i < 6; i++) {
        bsp_brushsides[i].planenum = INT16_MAX;
    }
}

static void brushsides_badtexinfo(void)
{
    int i;

    for (i = 0; i < 6; i++) {
        bsp_brushsides[i].texinfo = INT16_MIN;
    }
}

static void planes_badtype(void)
{
    int i;

    for (i = 0; i < 6; i++) {
        bsp_planes[i].type = INT32_MIN;
    }
}

static void models_badfirstface(void)
{
    strcat(bsp_entstring, " { classname func_water model *1 }");
    bsp_models[0].firstface = INT32_MAX;
    bsp_models[1].firstface = INT32_MAX;
}

static void models_badnumfaces(void)
{
    strcat(bsp_entstring, " { classname func_water model *1 }");
    bsp_models[0].numfaces = INT32_MAX;
    bsp_models[1].numfaces = INT32_MAX;
}

static void models_badheadnode(void)
{
    strcat(bsp_entstring, " { classname func_water model *1 }");
    bsp_models[0].headnode = INT32_MIN;
    bsp_models[1].headnode = INT32_MIN;
}

static void models_faceloop(void)
{
    strcat(bsp_entstring, " { classname func_water model *1 }");
    strcat(bsp_entstring, " { classname func_water model *1 }");
}

static void texinfo_badnext(void)
{
    int i;

    for (i = 0; i < 6; i++) {
        bsp_texinfo[i].next = INT32_MAX;
    }
}

static void texinfo_loopnext(void)
{
    bsp_texinfo[0].next = 1;
    bsp_texinfo[1].next = 2;
    bsp_texinfo[2].next = 1;
}

// overflow Com_sprintf's bigbuffer
static void texinfo_overflow(void)
{
    size_t size = 0x15000 - (0x15000 % sizeof(bsp2texinfo_t));

    bsp_header.lumps[BSP2_LUMP_TEXINFO].offset = bsp_size;
    bsp_header.lumps[BSP2_LUMP_TEXINFO].length = size;
    bsp_junk = (size + 63) / 64;
}

static void areas_badfirstportal(void)
{
    bsp_areas[1].firstportal = INT32_MAX;
}

static void areas_badnumportals(void)
{
    bsp_areas[1].numportals = INT32_MAX;
}

static void areaportals_badportalnum(void)
{
    bsp_areaportals[0].portalnum = INT32_MAX;
}

static void areaportals_badotherarea(void)
{
    bsp_areaportals[0].otherarea = INT32_MAX;
}

static void extents_overflow(void)
{
    int i, j;

    for (i = 0; i < 6; i++) {
        for (j = 0; j < 2; j++) {
            bsp_texinfo[i].vecs[j][0] =
            bsp_texinfo[i].vecs[j][1] =
            bsp_texinfo[i].vecs[j][2] =
            bsp_texinfo[i].vecs[j][3] = NAN;
        }
    }
}

static void entstring_unterminated(void)
{
    bsp_header.lumps[BSP2_LUMP_ENTSTRING].offset = bsp_size;
    bsp_header.lumps[BSP2_LUMP_ENTSTRING].length = BSP2_MAX_ENTSTRING;
    bsp_junk = BSP2_MAX_ENTSTRING / 64;
}

static void bad_vector(void)
{
    strcpy(bsp_entstring,
           "{ classname worldspawn } "
           "{ classname info_player_start origin \"\" }");
}

typedef struct {
    void (*func)(void);
    const char *name;
} mangler_t;

#define M(x)    { x, #x }

static const mangler_t manglers[] = {
    { NULL, "none" },
    M(header_badofs),
    M(header_badlen),
    M(header_overflow),
    M(header_misalign),
    M(vis_badnumclusters),
    M(vis_badoffsets),
    M(vis_overflow),
    M(nodes_loop),
    M(nodes_badplanenum),
    M(nodes_badchildren),
    M(nodes_badfirstface),
    M(nodes_badnumfaces),
    M(leaves_badcluster),
    M(leaves_badarea),
    M(leaves_badfirstbrush),
    M(leaves_badnumbrushes),
    M(leaves_badfirstface),
    M(leaves_badnumfaces),
    M(leaves_badcontents),
    M(brushes_badfirstside),
    M(brushes_badnumsides),
    M(leafbrushes_badindices),
    M(leaffaces_badindices),
    M(surfedges_badindices),
    M(edges_badindices),
    M(faces_badfirstedge),
    M(faces_toomanyedges),
    M(faces_toofewedges),
    M(faces_badplanenum),
    M(faces_badtexinfo),
    M(faces_badlightmap),
    M(brushsides_badplanenum),
    M(brushsides_badtexinfo),
    M(planes_badtype),
    M(models_badfirstface),
    M(models_badnumfaces),
    M(models_badheadnode),
    M(models_faceloop),
    M(texinfo_badnext),
    M(texinfo_loopnext),
    M(texinfo_overflow),
    M(areas_badfirstportal),
    M(areas_badnumportals),
    M(areaportals_badportalnum),
    M(areaportals_badotherarea),
    M(extents_overflow),
    M(entstring_unterminated),
    M(bad_vector),
};

#undef M

static const int nummanglers = sizeof(manglers) / sizeof(manglers[0]);

int main(int argc, const char **argv)
{
    char buffer[PATH_MAX];
    FILE *fp;
    int i, len;
    const mangler_t *m;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <output_dir>\n", argv[0]);
        return 1;
    }

    for (i = 0; i < nummanglers; i++) {
        m = &manglers[i];

        len = snprintf(buffer, sizeof(buffer), "%s/%s.bsp", argv[1], m->name);
        if (len < 0 || len >= sizeof(buffer)) {
            fprintf(stderr, "path overflowed\n");
            return 1;
        }

        fp = fopen(buffer, "wb");
        if (!fp) {
            perror("fopen");
            return 1;
        }

        init_bsp();

        if (m->func) {
            m->func();
        }

        write_bsp(fp);

        fclose(fp);
    }

    return 0;
}
