#define BSP2_MAGIC      ('I'|('B'<<8)|('S'<<16)|('P'<<24))
#define BSP2_VERSION    38

#define BSP2_MAX_ENTSTRING      0x00040000
#define BSP2_MAX_PLANES         0x00010000
#define BSP2_MAX_VERTICES       0x00010000
#define BSP2_MAX_VISIBILITY     0x00100000
#define BSP2_MAX_NODES          0x00010000
#define BSP2_MAX_TEXINFO        0x00002000
#define BSP2_MAX_FACES          0x00010000
#define BSP2_MAX_LIGHTMAP       0x00200000
#define BSP2_MAX_LEAVES         0x00010000
#define BSP2_MAX_LEAFFACES      0x00010000
#define BSP2_MAX_LEAFBRUSHES    0x00010000
#define BSP2_MAX_EDGES          128000
#define BSP2_MAX_SURFEDGES      256000
#define BSP2_MAX_MODELS         0x00000400
#define BSP2_MAX_BRUSHES        0x00002000
#define BSP2_MAX_BRUSHSIDES     0x00010000
#define BSP2_MAX_POP            1
#define BSP2_MAX_AREAS          0x00000100
#define BSP2_MAX_AREAPORTALS    0x00000400

#define BSP2_MIN_CLUSTERS       8

enum {
    BSP2_LUMP_ENTSTRING,
    BSP2_LUMP_PLANES,
    BSP2_LUMP_VERTICES,
    BSP2_LUMP_VISIBILITY,
    BSP2_LUMP_NODES,
    BSP2_LUMP_TEXINFO,
    BSP2_LUMP_FACES,
    BSP2_LUMP_LIGHTMAP,
    BSP2_LUMP_LEAVES,
    BSP2_LUMP_LEAFFACES,
    BSP2_LUMP_LEAFBRUSHES,
    BSP2_LUMP_EDGES,
    BSP2_LUMP_SURFEDGES,
    BSP2_LUMP_MODELS,
    BSP2_LUMP_BRUSHES,
    BSP2_LUMP_BRUSHSIDES,
    BSP2_LUMP_POP,
    BSP2_LUMP_AREAS,
    BSP2_LUMP_AREAPORTALS,

    BSP2_LUMP_TOTAL
};

typedef struct {
    uint32_t    offset;
    uint32_t    length;
} bsp2lump_t;

typedef struct {
    uint32_t    magic;
    uint32_t    version;
    bsp2lump_t  lumps[BSP2_LUMP_TOTAL];
} bsp2header_t;

typedef struct {
    float       normal[3];
    float       dist;
    uint32_t    type;
} bsp2plane_t;

typedef struct {
    float       point[3];
} bsp2vertex_t;

typedef struct {
    uint32_t    numclusters;
    uint32_t    offsets[BSP2_MIN_CLUSTERS][2];
} bsp2vis_t;

typedef struct {
    uint32_t    planenum;
    uint32_t    children[2];
    int16_t     mins[3];
    int16_t     maxs[3];
    uint16_t    firstface;
    uint16_t    numfaces;
} bsp2node_t;

typedef struct {
    float       vecs[2][4];
    uint32_t    flags;
    uint32_t    value;
    char        name[32];
    uint32_t    next;
} bsp2texinfo_t;

typedef struct {
    uint16_t    planenum;
    uint16_t    planeside;
    uint32_t    firstedge;
    uint16_t    numedges;
    uint16_t    texinfo;
    uint8_t     styles[4];
    uint32_t    lightmap;
} bsp2face_t;

typedef struct {
    uint32_t    contents;
    uint16_t    cluster;
    uint16_t    area;
    int16_t     mins[3];
    int16_t     maxs[3];
    uint16_t    firstface;
    uint16_t    numfaces;
    uint16_t    firstbrush;
    uint16_t    numbrushes;
} bsp2leaf_t;

typedef struct {
    uint16_t    vertices[2];
} bsp2edge_t;

typedef struct {
    float       mins[3];
    float       maxs[3];
    float       origin[3];
    uint32_t    headnode;
    uint32_t    firstface;
    uint32_t    numfaces;
} bsp2model_t;

typedef struct {
    uint32_t    firstside;
    uint32_t    numsides;
    uint32_t    contents;
} bsp2brush_t;

typedef struct {
    uint16_t    planenum;
    uint16_t    texinfo;
} bsp2brushside_t;

typedef struct {
    uint32_t    numportals;
    uint32_t    firstportal;
} bsp2area_t;

typedef struct {
    uint32_t    portalnum;
    uint32_t    otherarea;
} bsp2areaportal_t;
