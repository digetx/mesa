#include <stdio.h>

#include "pipe/p_state.h"
#include "util/u_helpers.h"

#include "tegra_common.h"
#include "tegra_context.h"
#include "tegra_draw.h"
#include "tegra_resource.h"
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

   tegra_stream_push(stream, host1x_opcode_incr(TGR3D_VP_ATTRIB_IN_OUT_SELECT, 1));
   tegra_stream_push(stream, ((uint32_t)context->vs->mask << 16) | out_mask);

   struct pipe_resource *index_buffer = NULL;
   unsigned index_offset = 0;
   if (info->index_size > 0) {
      if (info->has_user_indices) {
         if (!util_upload_index_buffer(pcontext, info, &index_buffer, &index_offset)) {
            fprintf(stderr, "util_upload_index_buffer() failed\n");
            return;
         }
      } else
         index_buffer = info->index.resource;

      tegra_stream_push(stream, host1x_opcode_incr(TGR3D_INDEX_PTR, 1));
      tegra_stream_push_reloc(stream, tegra_resource(index_buffer)->bo, index_offset);
   }

   /* draw params */
   assert(info->index_size >= 0 && info->index_size <= 2);
   value  = TGR3D_VAL(DRAW_PARAMS, INDEX_MODE, info->index_size);
   /* TODO: provoking vertex (comes from pipe_rasterizer_state) */
   value |= TGR3D_VAL(DRAW_PARAMS, PRIMITIVE_TYPE, TGR3D_PRIMITIVE_TYPE_TRIANGLES); /* TODO: derive from info */
   value |= TGR3D_VAL(DRAW_PARAMS, FIRST, info->start);
   value |= 0xC0000000;

   tegra_stream_push(stream, host1x_opcode_incr(TGR3D_DRAW_PARAMS, 1));
   tegra_stream_push(stream, value);

   assert(info->count < (1 << 11));
   value  = TGR3D_VAL(DRAW_PRIMITIVES, INDEX_COUNT, info->count);
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
