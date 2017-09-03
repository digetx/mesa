#include <assert.h>

#include "util/macros.h"

#include "vpe_ir.h"
#include "vpe_vliw.h"

void
tegra_vpe_pack(uint32_t *dst, struct vpe_instr instr, bool end_of_program)
{
   /* we can only handle one output register per instruction */
   assert(instr.vec.dst.file != VPE_DST_FILE_OUTPUT ||
          instr.scalar.dst.file != VPE_DST_FILE_OUTPUT);

   union vpe_instruction_u tmp = {
      .predicate_lt = 1,
      .predicate_eq = 1,
      .predicate_gt = 1,

      .predicate_swizzle_x = VPE_SWZ_X,
      .predicate_swizzle_y = VPE_SWZ_Y,
      .predicate_swizzle_z = VPE_SWZ_Z,
      .predicate_swizzle_w = VPE_SWZ_W,
   };

   /* find the attribute/uniform fetch-values, and zero out the index
    * for these registers.
    */
   int attr_fetch = -1, uniform_fetch = -1;
   for (int i = 0; i < 3; ++i) {
      switch (instr.vec.src[i].file) {
      case VPE_SRC_FILE_ATTRIB:
         assert(attr_fetch < 0 ||
                attr_fetch == instr.vec.src[i].index);
         attr_fetch = instr.vec.src[i].index;
         instr.vec.src[i].index = 0;
         break;

      case VPE_SRC_FILE_UNIFORM:
         assert(uniform_fetch < 0 ||
                uniform_fetch == instr.vec.src[i].index);
         uniform_fetch = instr.vec.src[i].index;
         instr.vec.src[i].index = 0;
         break;

      default: /* nothing */
         break;
      }
   }

   switch (instr.scalar.src.file) {
   case VPE_SRC_FILE_ATTRIB:
      assert(attr_fetch < 0 ||
             attr_fetch == instr.scalar.src.index);
      attr_fetch = instr.scalar.src.index;
      instr.scalar.src.index = 0;
      break;

      case VPE_SRC_FILE_UNIFORM:
         assert(uniform_fetch < 0 ||
                uniform_fetch == instr.scalar.src.index);
         uniform_fetch = instr.scalar.src.index;
         instr.scalar.src.index = 0;
         break;

      default: /* nothing */
         break;

   }

   tmp.attribute_fetch_index = attr_fetch >= 0 ? attr_fetch : 0;
   tmp.uniform_fetch_index = uniform_fetch >= 0 ? uniform_fetch : 0;

   tmp.vector_opcode = instr.vec.op;
   switch (instr.vec.dst.file) {
   case VPE_DST_FILE_TEMP:
      tmp.vector_rD_index = instr.vec.dst.index;
      tmp.export_write_index = 31;
      break;

   case VPE_DST_FILE_OUTPUT:
      tmp.vector_rD_index = 63; // disable register-write
      tmp.export_vector_write_enable = 1;
      tmp.export_write_index = instr.vec.dst.index;
      break;

   case VPE_DST_FILE_UNDEF:
      // assert(0);  // TODO: consult NOP
      break;

   default:
      unreachable("illegal enum vpe_dst_file value");
   }
   tmp.vector_op_write_x_enable = (instr.vec.dst.write_mask >> 0) & 1;
   tmp.vector_op_write_y_enable = (instr.vec.dst.write_mask >> 1) & 1;
   tmp.vector_op_write_z_enable = (instr.vec.dst.write_mask >> 2) & 1;
   tmp.vector_op_write_w_enable = (instr.vec.dst.write_mask >> 3) & 1;

   tmp.scalar_opcode = instr.scalar.op;
   switch (instr.scalar.dst.file) {
   case VPE_DST_FILE_TEMP:
      tmp.scalar_rD_index = instr.scalar.dst.index;
      tmp.export_write_index = 31;
      break;

   case VPE_DST_FILE_OUTPUT:
      tmp.scalar_rD_index = 63; // disable register-write
      tmp.export_vector_write_enable = 0;
      tmp.export_write_index = instr.scalar.dst.index;
      break;

   case VPE_DST_FILE_UNDEF:
      // assert(0);  // TODO: consult NOP
      break;

   default:
      unreachable("illegal enum vpe_dst_file value");
   }
   tmp.scalar_op_write_x_enable = (instr.scalar.dst.write_mask >> 0) & 1;
   tmp.scalar_op_write_y_enable = (instr.scalar.dst.write_mask >> 1) & 1;
   tmp.scalar_op_write_z_enable = (instr.scalar.dst.write_mask >> 2) & 1;
   tmp.scalar_op_write_w_enable = (instr.scalar.dst.write_mask >> 3) & 1;

   tmp.rA_type = instr.vec.src[0].file;
   tmp.rA_index = instr.vec.src[0].index;
   tmp.rA_swizzle_x = instr.vec.src[0].swizzle[0];
   tmp.rA_swizzle_y = instr.vec.src[0].swizzle[1];
   tmp.rA_swizzle_z = instr.vec.src[0].swizzle[2];
   tmp.rA_swizzle_w = instr.vec.src[0].swizzle[3];
   tmp.rA_negate = instr.vec.src[0].negate;
   tmp.rA_absolute = instr.vec.src[0].absolute;

   tmp.rB_type = instr.vec.src[1].file;
   tmp.rB_index = instr.vec.src[1].index;
   tmp.rB_swizzle_x = instr.vec.src[1].swizzle[0];
   tmp.rB_swizzle_y = instr.vec.src[1].swizzle[1];
   tmp.rB_swizzle_z = instr.vec.src[1].swizzle[2];
   tmp.rB_swizzle_w = instr.vec.src[1].swizzle[3];
   tmp.rB_negate = instr.vec.src[1].negate;
   tmp.rB_absolute = instr.vec.src[1].absolute;

   if (instr.vec.src[2].file != VPE_SRC_FILE_UNDEF) {
      tmp.rC_type = instr.vec.src[2].file;
      tmp.rC_index = instr.vec.src[2].index;
      tmp.rC_swizzle_x = instr.vec.src[2].swizzle[0];
      tmp.rC_swizzle_y = instr.vec.src[2].swizzle[1];
      tmp.rC_swizzle_z = instr.vec.src[2].swizzle[2];
      tmp.rC_swizzle_w = instr.vec.src[2].swizzle[3];
      tmp.rC_negate = instr.vec.src[2].negate;
      tmp.rC_absolute = instr.vec.src[2].absolute;
   } else if (instr.scalar.src.file != VPE_SRC_FILE_UNDEF) {
      tmp.rC_type = instr.scalar.src.file;
      tmp.rC_index = instr.scalar.src.index;
      tmp.rC_swizzle_x = instr.scalar.src.swizzle[0];
      tmp.rC_swizzle_y = instr.scalar.src.swizzle[1];
      tmp.rC_swizzle_z = instr.scalar.src.swizzle[2];
      tmp.rC_swizzle_w = instr.scalar.src.swizzle[3];
      tmp.rC_negate = instr.scalar.src.negate;
      tmp.rC_absolute = instr.scalar.src.absolute;
   }

   tmp.end_of_program = end_of_program;

   /* copy packed instruction into destination */
   dst[0] = tmp.part3;
   dst[1] = tmp.part2;
   dst[2] = tmp.part1;
   dst[3] = tmp.part0;
}
