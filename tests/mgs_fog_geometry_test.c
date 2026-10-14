#include "mgs_native.h"
#undef fprintf
#include "libdg/libdg.h"
#include "mgs_ordered_model.h"
#include <assert.h>
#include <stddef.h>
#include <math.h>

static unsigned scratch[256];
#undef SCRPAD_ADDR
#define SCRPAD_ADDR ((uintptr_t)scratch)
#undef getScratchAddr
#define getScratchAddr(offset) (scratch + (offset))
static MATRIX matrix = {{{4096,0,0},{0,4096,0},{0,0,4096}}, {0,0,1000}};
static PsyzGeometryCallback callback;
static const SVECTOR *active_vertices;
int GV_Clock;
int Psyz_GpuConfigureGeometry(PsyzGeometryCallback value, int textures, int precision, int depth)
{ (void)textures; (void)precision; (void)depth; callback = value; return 0; }
void Psyz_GpuResetGeometryDepth(void) {}
MATRIX *ReadRotMatrix(MATRIX *out) { *out = matrix; return out; }
unsigned Psyz_GteCtrlRead(unsigned reg) { return reg == 26 ? 160 : 0; }
int GV_StrCode(const char *text) { assert(!strcmp(text, "shadow")); return 0xed51; }
int GV_CacheID(int root, int spec)
{
    assert(spec == 'k');
    assert(root == 0xcb24 || root == 0x973d || root == 0x933d || root == 0x9b3d || root == 0xcb01);
    return root;
}
void *GV_GetCache(int id)
{
    assert(id == 0xcb24 || id == 0x973d || id == 0x933d || id == 0x9b3d || id == 0xcb01);
    return NULL;
}
static void ProjectScratch(PVECTOR *vertices, int count)
{
    active_vertices = (const SVECTOR *)vertices;
    for (int i = 0; i < count; ++i) {
        DVECTOR point = {(short)(i * 7 + 1), (short)(i * 11 + 2)};
        memcpy(scratch + i, &point, 4);
        scratch[128 + i] = 200 + i * 10;
    }
}
void s12c_800D5C48(PVECTOR *vertices, int count) { ProjectScratch(vertices, count); }
static void DG_TransVerticesBound(PVECTOR *vertices, int count) { ProjectScratch(vertices, count); }
#undef gte_ldsxy3
#undef gte_nclip_b
#undef gte_read_opz
#undef gte_nop
#define gte_ldsxy3(a,b,c) ((void)(a), (void)(b), (void)(c))
#define gte_nclip_b() ((void)0)
#define gte_read_opz(out) ((out) = 100)
#define gte_nop() ((void)0)
#undef gte_SetRotMatrix
#undef gte_SetTransMatrix
#define gte_SetRotMatrix(value) ((void)(value))
#define gte_SetTransMatrix(value) ((void)(value))
void DG_Clip(RECT *rect, int distance) { (void)rect; (void)distance; }
#include "fog_geometry_helpers.inc"

static void CheckMetadata(const void *xy, int ordered)
{
    PsyzProjectedVertex result;
    int flags = MgsTextureCorrectionGet(xy, &result);
    assert(flags & PSYZ_GEOMETRY_VALID);
    assert(!!(flags & PSYZ_GEOMETRY_ORDERED) == !!ordered);
    assert(result.depth > 0 && isfinite(result.x) && isfinite(result.y));
}

static void CheckParentPackets(void)
{
    SVECTOR vertices[4] = {{1,2,30,0},{3,4,40,0},{5,6,50,0},{7,8,60,0}};
    POLY_GT4 parent = {0};
    short *parent_xy[4] = {&parent.x0,&parent.x1,&parent.x2,&parent.x3};
    for (int i = 0; i < 4; ++i) {
        DVECTOR point = {(short)(90+i), (short)(100+i)};
        memcpy(parent_xy[i], &point, 4);
        vertices[i].pad = (short)((char *)parent_xy[i] - (char *)&parent);
    }
    const unsigned shifts[4] = {0,8,24,16};
    for (unsigned mask = 0; mask < 16; ++mask) {
        unsigned indices = 0;
        for (unsigned i = 0; i < 4; ++i)
            indices |= (i | ((mask >> i & 1) ? 128 : 0)) << shifts[i];
        for (unsigned shade = 0; shade <= 4; shade += 4) {
            POLY_GT4 off, on;
            memset(&off, 0x5a, sizeof(off)); on = off;
            ProjectScratch((PVECTOR *)vertices, 4);
            *(unsigned short *)(SCRPAD_ADDR + 0x1fe) = (unsigned short)shade;
            assert(MgsTextureCorrectionConfigure(0,0,0));
            unsigned off_tag = s12c_800D5CDC(indices, &off, &parent, vertices);
            assert(MgsTextureCorrectionConfigure(1,1,1));
            MgsTextureCorrectionBegin(0);
            MgsTextureCorrectionCapture(vertices, 4, (DVECTOR *)scratch);
            for (int i = 0; i < 4; ++i) MgsTextureCorrectionProject(parent_xy[i], vertices+i);
            unsigned on_tag = s12c_800D5CDC(indices, &on, &parent, vertices);
            assert(on_tag == off_tag && !memcmp(&off, &on, sizeof(on)));
            unsigned total = 0, count = 0;
            short *xy[4] = {&on.x0,&on.x1,&on.x2,&on.x3};
            for (unsigned i = 0; i < 4; ++i) {
                const void *source = mask >> i & 1 ? (void *)parent_xy[i] : (void *)(scratch+i);
                assert(!memcmp(xy[i], source, 4));
                CheckMetadata(xy[i], 0);
                PsyzProjectedVertex a,b;
                assert(MgsTextureCorrectionGet(source,&a));
                assert(MgsTextureCorrectionGet(xy[i],&b));
                assert(!memcmp(&a,&b,sizeof(a)));
                if (!(mask >> i & 1)) { total += scratch[128+i]; ++count; }
            }
            assert(on_tag == (count ? total/count : 0));
            POLY_GT4 untouched = on;
            assert(!s12c_800D5CDC(indices,&on,NULL,vertices));
            assert(!memcmp(&untouched,&on,sizeof(on)));
        }
    }
}

static void CheckObjectPackets(void)
{
    SVECTOR vertices[4] = {{1,2,30,0},{3,4,40,0},{5,6,50,0},{7,8,60,0}};
    unsigned indices[2] = {0x02030100,0x02030100};
    unsigned short textures[2] = {0x1234,0xed51};
    DG_MDL model = {.verts=vertices,.n_verts=4,.vindices=(unsigned char *)indices,.texids=textures,.flag=DG_MODEL_BOTHFACE};
    POLY_GT4 off[2], on[2];
    DG_OBJ object = {.model=&model,.n_packs=2};
    for (int bound = 0; bound < 2; ++bound) {
        for (int ordered = 0; ordered < 2; ++ordered) {
            memset(off,0x5a,sizeof(off)); memcpy(on,off,sizeof(on));
            memset(scratch,0,sizeof(scratch));
            *(unsigned short *)(SCRPAD_ADDR+0x1fe)=(unsigned short)bound;
            object.packs[0]=off;
            assert(MgsTextureCorrectionConfigure(0,0,0));
            s12c_800D6020(&object,0,NULL,ordered);
            object.packs[0]=on;
            assert(MgsTextureCorrectionConfigure(1,1,1));
            MgsTextureCorrectionBegin(0);
            s12c_800D6020(&object,0,NULL,ordered);
            assert(active_vertices == vertices && !memcmp(off,on,sizeof(on)));
            for (int face=0;face<2;++face) {
                short *xy[4]={&on[face].x0,&on[face].x1,&on[face].x2,&on[face].x3};
                for(int i=0;i<4;++i) CheckMetadata(xy[i],ordered || face==1);
            }
        }
    }
}

static void CheckSignedDepthTags(void)
{
    SVECTOR vertices[4] = {0};
    POLY_GT4 parent = {0}, packet;
    const unsigned shifts[4] = {0,8,24,16};
    const unsigned depths[][4] = {
        {0x01000100,0x80000100,0xff000100,0x00000100},
        {0x7fffffff,0x7fffffff,0x7fffffff,0x7fffffff},
        {0x80000000,0x80000000,0x80000000,0x80000000},
        {0xffffffff,0x01000100,0xff000100,0x80000100}
    };
    short *parent_xy[4]={&parent.x0,&parent.x1,&parent.x2,&parent.x3};
    for (int i=0;i<4;++i)
        vertices[i].pad=(short)((char *)parent_xy[i]-(char *)&parent);
    assert(MgsTextureCorrectionConfigure(0,0,0));
    for (unsigned case_no=0;case_no<sizeof(depths)/sizeof(depths[0]);++case_no) {
        for (unsigned mask=1;mask<15;++mask) {
            unsigned indices=0, wrapped_sum=0, count=0;
            memset(scratch,0,sizeof(scratch)); memset(&packet,0,sizeof(packet));
            for (unsigned i=0;i<4;++i) {
                indices|=(i | ((mask>>i&1)?128:0))<<shifts[i];
                scratch[128+i]=depths[case_no][i];
                if (!(mask>>i&1)) { wrapped_sum+=depths[case_no][i]; ++count; }
            }
            /* Original MIPS uses modulo-32 ADDU then signed DIV. */
            unsigned expected=(unsigned)((int32_t)wrapped_sum/(int)count);
            assert(s12c_800D5CDC(indices,&packet,&parent,vertices)==expected);
        }
    }
}

static void CheckRootModel(void)
{
    typedef struct { DG_OBJS group; DG_OBJ objects[1]; } Group;
    Group group = {0};
    SVECTOR vertices[4] = {{1,2,30,0},{3,4,40,0},{5,6,50,0},{7,8,60,0}};
    unsigned indices = 0x02030100;
    DG_MDL model = {.verts=vertices,.n_verts=4,.vindices=(unsigned char *)&indices,.flag=DG_MODEL_BOTHFACE};
    POLY_GT4 packets[2];
    group.group.bound_mode=1; group.group.n_models=1;
    group.group.flag=DG_FLAG_NATIVE_ORDERED;
    group.objects[0].model=&model; group.objects[0].bound_mode=1;
    group.objects[0].n_packs=1;
    DG_OBJS *queue[1]={&group.group};
    DG_CHANL channel={0};
    channel.queue=queue; channel.objs_index=1;
    const int absent_parents[2]={-1,-100000};
    for (int index=0;index<2;++index) {
        GV_Clock=index;
        memset(&packets[index],0x5a,sizeof(packets[index]));
        group.objects[0].packs[index]=&packets[index];
        model.parent=absent_parents[index];
        memset(scratch,0,sizeof(scratch)); scratch[254]=0x12345678;
        assert(MgsTextureCorrectionConfigure(1,1,1));
        MgsTextureCorrectionBegin(index);
        FogTransChanl_800D63B0(&channel,index);
        /* Native parent pointers must never occupy PS1 scratch pointer words. */
        assert(scratch[254]==0x12345678);
        short *xy[4]={&packets[index].x0,&packets[index].x1,&packets[index].x2,&packets[index].x3};
        for(int i=0;i<4;++i) CheckMetadata(xy[i],1);
    }
}

int main(void)
{
    CheckParentPackets();
    CheckObjectPackets();
    CheckSignedDepthTags();
    CheckRootModel();
    puts("Fog direct/parent metadata, ordered shadows and option-off packet parity passed");
    return 0;
}
