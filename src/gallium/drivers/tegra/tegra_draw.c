#include <stdio.h>

#include "pipe/p_state.h"

#include "tegra_common.h"
#include "tegra_context.h"
#include "tegra_draw.h"
#include "tegra_state.h"

#include "tgr_3d.xml.h"
#include "host1x01_hardware.h"

static void
tegra_draw_vbo(struct pipe_context *pcontext,
               const struct pipe_draw_info *info)
{
   int err;
   uint32_t value;
   struct tegra_context *context = tegra_context(pcontext);
   struct tegra_stream *stream = &context->gr3d->stream;
   uint16_t out_mask = 0x0081;

   err = tegra_stream_begin(stream);
   if (err < 0) {
      fprintf(stderr, "tegra_stream_begin() failed: %d\n", err);
      return;
   }

   tegra_stream_push_setclass(stream, HOST1X_CLASS_GR3D);

   tegra_emit_state(context);

   assert(!info->index_size);

   tegra_stream_push(stream, host1x_opcode_incr(TGR3D_VP_ATTRIB_IN_OUT_SELECT, 1));
   tegra_stream_push(stream, ((uint32_t)context->vs->mask << 16) | out_mask);

   /* draw params */
   value  = TGR3D_VAL(DRAW_PARAMS, INDEX_MODE, TGR3D_INDEX_MODE_NONE);
   /* TODO: provoking vertex (comes from pipe_rasterizer_state) */
   value |= TGR3D_VAL(DRAW_PARAMS, PRIMITIVE_TYPE, TGR3D_PRIMITIVE_TYPE_TRIANGLES); /* TODO: derive from info */
   value |= TGR3D_VAL(DRAW_PARAMS, FIRST, info->start);
   value |= 0xC0000000; /* flush input caches? */

   tegra_stream_push(stream, host1x_opcode_incr(TGR3D_DRAW_PARAMS, 1));
   tegra_stream_push(stream, value);

   assert(info->count > 0 && info->count < (1 << 11));
   value  = TGR3D_VAL(DRAW_PRIMITIVES, INDEX_COUNT, info->count - 1);
   value |= TGR3D_VAL(DRAW_PRIMITIVES, OFFSET, 0); /* TODO: derive from info */
   tegra_stream_push(stream, host1x_opcode_incr(TGR3D_DRAW_PRIMITIVES, 1));
   tegra_stream_push(stream, value);

   tegra_stream_end(stream);

   tegra_stream_flush(stream);
}

void
tegra_context_draw_init(struct pipe_context *pcontext)
{
   pcontext->draw_vbo = tegra_draw_vbo;
}
