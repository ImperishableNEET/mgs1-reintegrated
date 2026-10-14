#include "mgs_texture_correction.h"
/* Recovered asset header uses the PSYQ unsigned-int alias. */
typedef unsigned int u_int;
#include "../vendor/mgs/source/include/fmt_kmd.h"
#include "mgs_ordered_model.h"
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static MATRIX transform = {{{4096, 0, 0}, {0, 4096, 0}, {0, 0, 4096}}, {0, 0, 0}};
static PsyzGeometryCallback callback;
static int resets;
static unsigned controls[32] = {[26] = 160};
static unsigned control_reads[32];

int Psyz_GpuConfigureGeometry(PsyzGeometryCallback value, int textures, int precision, int depth)
{
    assert(!value || textures || precision || depth);
    callback = value;
    return 0;
}

void Psyz_GpuResetGeometryDepth(void) { ++resets; }
MATRIX *ReadRotMatrix(MATRIX *output) { *output = transform; return output; }
unsigned Psyz_GteCtrlRead(unsigned reg) { ++control_reads[reg]; return controls[reg]; }

static void CheckCaptureControls(void)
{
    DVECTOR xy[2] = {{0, 0}, {0, 0}};
    SVECTOR vertices[2] = {{1, -1, 300, 0}, {-1, 3, 320, 0}};
    PsyzProjectedVertex result;
    controls[24] = 163840; /* +2.5 pixels */
    controls[25] = (unsigned)-212992; /* -3.25 pixels */
    memset(control_reads, 0, sizeof(control_reads));
    MgsTextureCorrectionBegin(0);
    MgsTextureCorrectionCapture(NULL, 0, NULL);
    assert(!control_reads[24] && !control_reads[25] && !control_reads[26]);
    MgsTextureCorrectionCapture(vertices, 2, xy);
    assert(control_reads[24] == 1 && control_reads[25] == 1 && control_reads[26] == 1);
    assert(callback(&xy[0], &result));
    assert(fabsf(result.x - (2.5f + 160.0f / 300)) < 0.00001f);
    assert(fabsf(result.y - (-3.25f - 160.0f / 300)) < 0.00001f);
    assert(callback(&xy[1], &result) && result.x == 2 && result.y == -1.75f);
    controls[24] = 65536;
    controls[25] = 131072;
    controls[26] = 320;
    MgsTextureCorrectionCapture(vertices, 2, xy);
    assert(callback(&xy[1], &result) && result.x == 0 && result.y == 5);
    controls[24] = controls[25] = 0;
    controls[26] = 160;
}

static void CheckMidpoints(void)
{
    uint32_t xy[5] = {0};
    SVECTOR first = {-51, 73, 300, 0}, second = {88, -22, 901, 0};
    PsyzProjectedVertex midpoint, reversed, nested;
    MgsTextureCorrectionBegin(0);
    MgsTextureCorrectionProject(&xy[0], &first);
    MgsTextureCorrectionProject(&xy[1], &second);
    MgsTextureCorrectionMidpoint(&xy[2], &xy[0], &xy[1]);
    MgsTextureCorrectionMidpoint(&xy[3], &xy[1], &xy[0]);
    assert(callback(&xy[2], &midpoint) && callback(&xy[3], &reversed));
    assert(midpoint.depth == 600.5f);
    assert(fabsf(midpoint.x - 160 * 18.5f / 600.5f) < 0.00001f);
    assert(fabsf(midpoint.y - 160 * 25.5f / 600.5f) < 0.00001f);
    assert(midpoint.x == reversed.x && midpoint.y == reversed.y && midpoint.depth == reversed.depth);
    MgsTextureCorrectionOrderOnly(&xy[0]);
    assert(callback(&xy[0], &midpoint) == (PSYZ_GEOMETRY_VALID | PSYZ_GEOMETRY_ORDERED));
    MgsTextureCorrectionCopy(&xy[3], &xy[0]);
    assert(callback(&xy[3], &midpoint) & PSYZ_GEOMETRY_ORDERED);
    MgsTextureCorrectionMidpoint(&xy[2], &xy[0], &xy[1]);
    assert(callback(&xy[2], &midpoint) & PSYZ_GEOMETRY_ORDERED);
    MgsTextureCorrectionMidpoint(&xy[4], &xy[0], &xy[2]);
    assert(callback(&xy[4], &nested) && nested.depth == 450.25f);
    assert(fabsf(nested.x + 160 * 16.25f / 450.25f) < 0.00001f);
    ++xy[0];
    MgsTextureCorrectionMidpoint(&xy[2], &xy[0], &xy[1]);
    assert(!callback(&xy[2], &midpoint));
    MgsTextureCorrectionMidpoint(&xy[4], &xy[2], &xy[1]);
    assert(!callback(&xy[4], &nested));
}


static void CheckOfficeFloorIdentity(void)
{
    DG_DEF *def = calloc(1, sizeof(*def) + 16 * sizeof(DG_MDL));
    assert(def);
    def->n_models = def->n_x_models = 16;
    DG_MDL *model = &def->models[15];
    SVECTOR vertices[44] = {0};
    unsigned short textures[16];
    for (int i = 0; i < 16; ++i) textures[i] = 0xed51;
    model->flag = 0x3203;
    model->n_faces = 16;
    model->n_verts = 44;
    model->parent = model->extend = -1;
    model->verts = vertices;
    model->texids = textures;
    assert(MgsOfficeFloorOrdered(def, def, model));
    assert(!MgsOfficeFloorOrdered(NULL, def, model));
    assert(!MgsOfficeFloorOrdered(def, def, &def->models[11]));
    DG_DEF other = {0};
    assert(!MgsOfficeFloorOrdered(&other, def, model));
    vertices[20].vy = 1;
    assert(!MgsOfficeFloorOrdered(def, def, model));
    vertices[20].vy = 0;
    textures[4] = 0x70e0;
    assert(!MgsOfficeFloorOrdered(def, def, model));
    textures[4] = 0xed51;
    model->flag = 0x3201;
    assert(!MgsOfficeFloorOrdered(def, def, model));
    free(def);
}

static void CheckControlRoomGlowIdentity(void)
{
    DG_DEF *def = calloc(1, sizeof(*def) + 32 * sizeof(DG_MDL));
    assert(def);
    def->n_models = 32; def->n_x_models = 49;
    SVECTOR vertices[16] = {0};
    unsigned short textures[4] = {0x2531,0x2531,0x2531,0x2531};
    DG_DEF unrelated = {0};
    for (int part = 7; part <= 8; ++part) {
        SVECTOR minimum = part == 7 ? (SVECTOR){-4230,2290,6750,0} : (SVECTOR){-3125,2275,6805,0};
        SVECTOR maximum = part == 7 ? (SVECTOR){5145,3074,10130,0} : (SVECTOR){2916,2584,6894,0};
        for (int i=0;i<16;++i) vertices[i]=minimum;
        vertices[1]=maximum;
        DG_MDL *model = &def->models[part];
        model->flag = part == 7 ? 0x3107 : 0x3207;
        model->n_faces = part == 7 ? 4 : 2;
        model->n_verts = part == 7 ? 16 : 8;
        model->parent = model->extend = -1;
        model->verts = vertices;
        model->texids = textures;
        assert(MgsControlRoomGlowOrdered(def,def,model));
        /* The renderer only supplies this cached definition in s16d. Similar
         * geometry elsewhere and models from another resource must reject. */
        assert(!MgsControlRoomGlowOrdered(NULL,def,model));
        assert(!MgsControlRoomGlowOrdered(&unrelated,def,model));
        assert(!MgsControlRoomGlowOrdered(def,&unrelated,model));
        assert(!MgsControlRoomGlowOrdered(def,def,&def->models[6]));
        unsigned flag=model->flag;
        model->flag ^= 0x100;
        assert(!MgsControlRoomGlowOrdered(def,def,model));
        model->flag=flag;
        ++model->n_faces;
        assert(!MgsControlRoomGlowOrdered(def,def,model));
        --model->n_faces;
        ++model->n_verts;
        assert(!MgsControlRoomGlowOrdered(def,def,model));
        --model->n_verts;
        model->parent=0;
        assert(!MgsControlRoomGlowOrdered(def,def,model));
        model->parent=-1; model->extend=0;
        assert(!MgsControlRoomGlowOrdered(def,def,model));
        model->extend=-1;
        model->tx=1;
        assert(!MgsControlRoomGlowOrdered(def,def,model));
        model->tx=0;
        ++vertices[1].vx;
        assert(!MgsControlRoomGlowOrdered(def,def,model));
        --vertices[1].vx;
        textures[model->n_faces-1]=0x2530;
        assert(!MgsControlRoomGlowOrdered(def,def,model));
        textures[model->n_faces-1]=0x2531;
        model->verts=NULL;
        assert(!MgsControlRoomGlowOrdered(def,def,model));
        model->verts=vertices; model->texids=NULL;
        assert(!MgsControlRoomGlowOrdered(def,def,model));
        model->texids=textures;
        unsigned count=def->n_models;
        def->n_models=part;
        assert(!MgsControlRoomGlowOrdered(def,def,model));
        def->n_models=count;
        --def->n_x_models;
        assert(!MgsControlRoomGlowOrdered(def,def,model));
        ++def->n_x_models;
        uint32_t packed=0;
        SVECTOR point={7,-3,500,0};
        PsyzProjectedVertex before,after;
        MgsTextureCorrectionBegin(0);
        MgsTextureCorrectionProject(&packed,&point);
        assert(callback(&packed,&before)==PSYZ_GEOMETRY_VALID);
        if (MgsControlRoomGlowOrdered(def,def,model))
            MgsTextureCorrectionOrderOnly(&packed);
        assert(callback(&packed,&after)==(PSYZ_GEOMETRY_VALID|PSYZ_GEOMETRY_ORDERED));
        assert(!memcmp(&before,&after,sizeof(before)));
        assert(packed==0); /* Ordering policy never changes native packet XY. */
    }
    free(def);
}

static void CheckWallLightIdentity(void)
{
    const int approved[][2] = {{0,8},{1,6},{2,4},{0,10},{1,9},
        {3,2},{3,4},{3,8},{3,14},{3,16},{3,21},{3,23},{3,27},{3,33},{3,35}};
    unsigned seen=0;
    assert(sizeof(MgsOrderedLights)/sizeof(MgsOrderedLights[0]) == 15);
    for (unsigned entry=0;entry<15;++entry) {
        const MgsOrderedLightSignature *signature=&MgsOrderedLights[entry];
        int approved_match=0;
        for (unsigned j=0;j<15;++j) {
            if(signature->resource==approved[j][0] && signature->part==approved[j][1]) {
                assert(!(seen & (1u<<j))); seen |= 1u<<j; approved_match=1;
            }
        }
        assert(approved_match);
        DG_DEF *def=calloc(1,sizeof(*def)+(size_t)signature->models*sizeof(DG_MDL));
        assert(def);
        def->n_models=signature->models; def->n_x_models=signature->xmodels;
        const DG_DEF *cached[4]={0}; cached[signature->resource]=def;
        DG_DEF unrelated={0};
        DG_MDL *model=&def->models[signature->part];
        SVECTOR vertices[16];
        unsigned short textures[4];
        for(int i=0;i<16;++i) vertices[i]=(SVECTOR){signature->bounds[0],signature->bounds[2],signature->bounds[4],0};
        vertices[1]=(SVECTOR){signature->bounds[1],signature->bounds[3],signature->bounds[5],0};
        for(int i=0;i<4;++i) textures[i]=signature->texture;
        *model=(DG_MDL){.flag=signature->flag,.n_faces=signature->faces,.n_verts=signature->vertices,
            .parent=-1,.extend=-1,.verts=vertices,.texids=textures};
        assert(MgsWallLightOrdered(cached,def,model));
        cached[signature->resource]=NULL;
        assert(!MgsWallLightOrdered(cached,def,model));
        cached[signature->resource]=&unrelated;
        assert(!MgsWallLightOrdered(cached,def,model));
        cached[signature->resource]=def;
        assert(!MgsWallLightOrdered(cached,&unrelated,model));
        assert(!MgsWallLightOrdered(cached,def,&def->models[0]));
        model->flag^=0x100;
        assert(!MgsWallLightOrdered(cached,def,model));
        model->flag=signature->flag;
        ++model->n_faces; assert(!MgsWallLightOrdered(cached,def,model)); --model->n_faces;
        ++model->n_verts; assert(!MgsWallLightOrdered(cached,def,model)); --model->n_verts;
        model->parent=0; assert(!MgsWallLightOrdered(cached,def,model)); model->parent=-1;
        model->extend=0; assert(!MgsWallLightOrdered(cached,def,model)); model->extend=-1;
        model->ty=1; assert(!MgsWallLightOrdered(cached,def,model)); model->ty=0;
        ++vertices[1].vx; assert(!MgsWallLightOrdered(cached,def,model)); --vertices[1].vx;
        ++textures[model->n_faces-1]; assert(!MgsWallLightOrdered(cached,def,model)); --textures[model->n_faces-1];
        ++def->n_models; assert(!MgsWallLightOrdered(cached,def,model)); --def->n_models;
        ++def->n_x_models; assert(!MgsWallLightOrdered(cached,def,model)); --def->n_x_models;
        /* A higher-detail warehouse resource must not match this policy. */
        if(signature->resource==3) {
            def->n_models=def->n_x_models=136;
            assert(!MgsWallLightOrdered(cached,def,model));
        }
        free(def);
    }
    assert(seen==0x7fff);
}

int main(void)
{
    uint32_t packed = 0, copied = 0;
    SVECTOR vertex = {1, -1, 300, 0};
    PsyzProjectedVertex result;
    assert(MgsTextureCorrectionConfigure(1, 1, 1));
    CheckCaptureControls();
    CheckMidpoints();
    CheckOfficeFloorIdentity();
    CheckControlRoomGlowIdentity();
    CheckWallLightIdentity();
    MgsTextureCorrectionBegin(0);
    MgsTextureCorrectionProject(&packed, &vertex);
    assert(callback(&packed, &result));
    assert(fabsf(result.x - 160.0f / 300) < 0.00001f);
    assert(fabsf(result.y + 160.0f / 300) < 0.00001f && result.depth == 300);
    MgsTextureCorrectionCopy(&copied, &packed);
    assert(callback(&copied, &result));
    assert(callback(&copied, &result) == PSYZ_GEOMETRY_VALID);
    ++packed;
    assert(!callback(&packed, &result));
    MgsTextureCorrectionBegin(1);
    assert(!callback(&copied, &result));
    vertex.vz = 900;
    MgsTextureCorrectionProject(&copied, &vertex);
    MgsTextureCorrectionSelect(0);
    assert(callback(&copied, &result) && result.depth == 300 && resets == 1);
    MgsTextureCorrectionSelect(1);
    assert(callback(&copied, &result) && result.depth == 900);
    MgsTextureCorrectionBegin(0);
    assert(!callback(&copied, &result));
    uint32_t points[8192] = {0};
    for (unsigned i = 0; i < 8192; ++i) {
        vertex.vz = (short)(1000 + i);
        MgsTextureCorrectionProject(&points[i], &vertex);
    }
    for (unsigned i = 0; i < 8192; ++i)
        assert(callback(&points[i], &result) && result.depth == 1000 + i);
    vertex.vz = 0;
    MgsTextureCorrectionProject(&copied, &vertex);
    assert(!callback(&copied, &result));
    vertex.vz = 50;
    MgsTextureCorrectionProject(&copied, &vertex);
    assert(!callback(&copied, &result));
    assert(MgsTextureCorrectionConfigure(0, 0, 0) && !callback);
    assert(!MgsTextureCorrectionGet(&points[0], &result));
    puts("Projected geometry: projection, subdivision midpoints, copy, invalidation, double buffering and growth passed.");
    return 0;
}
