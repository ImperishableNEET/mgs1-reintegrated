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
#endif
