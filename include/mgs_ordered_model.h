#ifndef MGS_ORDERED_MODEL_H
#define MGS_ORDERED_MODEL_H

/* Include after fmt_kmd.h. This policy reads runtime data without changing it. */
static inline int MgsOfficeFloorOrdered(const DG_DEF *cached, const DG_DEF *def,
                                      const DG_MDL *model)
{
    if (!cached || def != cached || def->n_models <= 15 ||
        def->n_x_models <= 15 || model != &def->models[15] ||
        model->flag != 0x3203 || model->n_faces != 16 ||
        model->n_verts != 44 || model->parent != -1 || model->extend != -1 ||
        model->tx || model->ty || model->tz || !model->texids || !model->verts)
        return 0;
    for (int face = 0; face < 16; ++face)
        if (model->texids[face] != 0xed51) return 0;
    for (int vertex = 0; vertex < 44; ++vertex)
        if (model->verts[vertex].vy != 0) return 0;
    return 1;
}
/* Verified control-room CRT/console glow sheets retain their authored OT
 * raises. Other raised or translucent models remain depth tested. */
static inline int MgsControlRoomGlowOrdered(const DG_DEF *cached, const DG_DEF *def,
                                           const DG_MDL *model)
{
    if (!cached || def != cached || def->n_models != 32 || def->n_x_models != 49)
        return 0;
    int index = model == &def->models[7] ? 7 : model == &def->models[8] ? 8 : -1;
    if (index < 0 || model->flag != (index == 7 ? 0x3107u : 0x3207u) ||
        model->n_faces != (index == 7 ? 4 : 2) || model->n_verts != (index == 7 ? 16 : 8) ||
        model->parent != -1 || model->extend != -1 || model->tx || model->ty || model->tz ||
        !model->texids || !model->verts)
        return 0;
    for (int face = 0; face < model->n_faces; ++face)
        if (model->texids[face] != 0x2531) return 0;
    int minimum[3] = {model->verts[0].vx, model->verts[0].vy, model->verts[0].vz};
    int maximum[3] = {minimum[0], minimum[1], minimum[2]};
    for (int vertex = 1; vertex < model->n_verts; ++vertex) {
        int position[3] = {model->verts[vertex].vx, model->verts[vertex].vy, model->verts[vertex].vz};
        for (int axis = 0; axis < 3; ++axis) {
            if (position[axis] < minimum[axis]) minimum[axis] = position[axis];
            if (position[axis] > maximum[axis]) maximum[axis] = position[axis];
        }
    }
    if (index == 7)
        return minimum[0] == -4230 && maximum[0] == 5145 && minimum[1] == 2290 &&
               maximum[1] == 3074 && minimum[2] == 6750 && maximum[2] == 10130;
    return minimum[0] == -3125 && maximum[0] == 2916 && minimum[1] == 2275 &&
           maximum[1] == 2584 && minimum[2] == 6805 && maximum[2] == 6894;
}

/* Coplanar lamp/glow sheets verified in the freight lift and cold warehouse.
 * Resource identity and geometry signatures reject unrelated/modded artwork. */
typedef struct MgsOrderedLightSignature {
    int resource, models, xmodels, part;
    unsigned flag;
    int faces, vertices;
    unsigned short texture;
    int bounds[6]; /* minimum/maximum for X, Y, Z */
} MgsOrderedLightSignature;
static const MgsOrderedLightSignature MgsOrderedLights[] = {
    {0, 15, 25, 8, 0x107, 4, 16, 0xe173, {-3500, 3500, 6000, 6000, -500, 5500}},
    {2, 12, 17, 4, 0x107, 4, 16, 0xe173, {-3600, 3600, 4000, 5235, -16600, -7525}},
    {1, 14, 19, 6, 0x107, 4, 16, 0xe173, {-3500, 3500, 6000, 6000, -5000, 2000}},
    {3, 38, 44, 2, 0x22107, 2, 8, 0x9847, {-12375, -4875, 4000, 4500, -9000, -9000}},
    {3, 38, 44, 4, 0x23107, 2, 8, 0x9847, {-13000, -13000, 3375, 4000, -8375, -750}},
    {3, 38, 44, 8, 0x22107, 2, 8, 0x9847, {-3375, 3375, 4000, 4500, -9000, -9000}},
    {3, 38, 44, 14, 0x22107, 2, 8, 0x9847, {4875, 12375, 4000, 4500, -9000, -9000}},
    {3, 38, 44, 16, 0x23107, 2, 8, 0x9847, {13000, 13000, 3375, 4000, -8374, -749}},
    {3, 38, 44, 21, 0x22107, 2, 8, 0x9847, {-12375, -4875, 4000, 4500, 12000, 12000}},
    {3, 38, 44, 23, 0x23107, 2, 8, 0x9847, {-13000, -13000, 3375, 4000, 3749, 11374}},
    {3, 38, 44, 27, 0x22107, 2, 8, 0x9847, {-3375, 3375, 4000, 4500, 12000, 12000}},
    {3, 38, 44, 33, 0x22107, 2, 8, 0x9847, {4875, 12375, 4000, 4500, 12000, 12000}},
    {3, 38, 44, 35, 0x23107, 2, 8, 0x9847, {13000, 13000, 3375, 4000, 3750, 11375}},
    {0, 15, 25, 10, 0x3107, 4, 10, 0x9e0f, {-4000, 4000, 750, 2000, -10500, -9500}},
    {1, 14, 19, 9, 0x3107, 4, 10, 0x9e0f, {-4000, 4000, 750, 2000, -10500, -9500}},
};
static inline int MgsWallLightOrdered(const DG_DEF *const cached[4], const DG_DEF *def,
                                      const DG_MDL *model)
{
    if (!cached || !def || !model) return 0;
    int resource = -1;
    for (int i = 0; i < 4; ++i)
        if (cached[i] && def == cached[i]) resource = i;
    if (resource < 0) return 0;
    for (unsigned entry = 0; entry < sizeof(MgsOrderedLights) / sizeof(MgsOrderedLights[0]); ++entry) {
        const MgsOrderedLightSignature *sig = &MgsOrderedLights[entry];
        if (resource != sig->resource || def->n_models != sig->models ||
            def->n_x_models != sig->xmodels || model != &def->models[sig->part])
            continue;
        if (model->flag != sig->flag || model->n_faces != sig->faces ||
            model->n_verts != sig->vertices || model->parent != -1 || model->extend != -1 ||
            model->tx || model->ty || model->tz || !model->verts || !model->texids)
            return 0;
        for (int face = 0; face < model->n_faces; ++face)
            if (model->texids[face] != sig->texture) return 0;
        int bounds[6] = {model->verts[0].vx, model->verts[0].vx,
                         model->verts[0].vy, model->verts[0].vy,
                         model->verts[0].vz, model->verts[0].vz};
        for (int vertex = 1; vertex < model->n_verts; ++vertex) {
            int position[3] = {model->verts[vertex].vx, model->verts[vertex].vy, model->verts[vertex].vz};
            for (int axis = 0; axis < 3; ++axis) {
                if (position[axis] < bounds[axis * 2]) bounds[axis * 2] = position[axis];
                if (position[axis] > bounds[axis * 2 + 1]) bounds[axis * 2 + 1] = position[axis];
            }
        }
        for (int i = 0; i < 6; ++i)
            if (bounds[i] != sig->bounds[i]) return 0;
        return 1;
    }
    return 0;
}
#endif
