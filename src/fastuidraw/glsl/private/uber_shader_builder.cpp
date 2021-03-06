/*!
 * \file uber_shader_builder.cpp
 * \brief file uber_shader_builder.cpp
 *
 * Copyright 2016 by Intel.
 *
 * Contact: kevin.rogovin@intel.com
 *
 * This Source Code Form is subject to the
 * terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with
 * this file, You can obtain one at
 * http://mozilla.org/MPL/2.0/.
 *
 * \author Kevin Rogovin <kevin.rogovin@intel.com>
 *
 */

#include <sstream>

#include "uber_shader_builder.hpp"
#include "../../private/util_private.hpp"

namespace
{
  fastuidraw::c_string
  float_varying_label(unsigned int t)
  {
    using namespace fastuidraw::glsl;
    switch(t)
      {
      case varying_list::interpolation_smooth:
        return "fastuidraw_varying_float_smooth";
      case varying_list::interpolation_flat:
        return "fastuidraw_varying_float_flat";
      case varying_list::interpolation_noperspective:
        return "fastuidraw_varying_float_noperspective";
      }
    FASTUIDRAWassert(!"Invalid varying_list::interpolation_qualifier_t");
    return "";
  }

  fastuidraw::c_string
  int_varying_label(void)
  {
    return "fastuidraw_varying_int";
  }

  fastuidraw::c_string
  uint_varying_label(void)
  {
    return "fastuidraw_varying_uint";
  }

  void
  stream_varyings_as_local_variables_array(fastuidraw::glsl::ShaderSource &vert,
                                           fastuidraw::c_array<const fastuidraw::c_string> p,
                                           fastuidraw::c_string type)
  {
    std::ostringstream ostr;
    for(fastuidraw::c_string str : p)
      {
        ostr << type << " " << str << ";\n";
      }
    vert.add_source(ostr.str().c_str(), fastuidraw::glsl::ShaderSource::from_string);
  }

  unsigned int
  compute_special_index(size_t sz)
  {
    if (sz % 4 == 1)
      {
        return sz - 1;
      }
    else
      {
        return sz;
      }
  }

  void
  stream_alias_varyings_array(fastuidraw::c_string append_to_name,
                              fastuidraw::glsl::ShaderSource &vert,
                              fastuidraw::c_array<const fastuidraw::c_string> p,
                              const std::string &s, bool define,
                              unsigned int special_index)
  {
    fastuidraw::c_string ext = "xyzw";
    for(unsigned int i = 0; i < p.size(); ++i)
      {
        if (define)
          {
            std::ostringstream str;
            str << s << append_to_name << i / 4;
            if (i != special_index)
              {
                str << "." << ext[i % 4];
              }
            vert.add_macro(p[i], str.str().c_str());
          }
        else
          {
            vert.remove_macro(p[i]);
          }
      }
  }

  unsigned int
  stream_declare_varyings_type(fastuidraw::c_string append_to_name, unsigned int location,
                               std::ostream &str, unsigned int cnt,
                               fastuidraw::c_string qualifier,
                               fastuidraw::c_string types[],
                               fastuidraw::c_string name)
  {
    unsigned int extra_over_4;
    unsigned int return_value;

    /* pack the data into vec4 types an duse macros
     * to do the rest.
     */
    return_value = cnt / 4;
    for(unsigned int i = 0; i < return_value; ++i, ++location)
      {
        str << "FASTUIDRAW_LAYOUT_VARYING(" << location << ") "
            << qualifier << " fastuidraw_varying"
            << " " << types[3] << " "
            << name << append_to_name << i << ";\n\n";
      }

    extra_over_4 = cnt % 4;
    if (extra_over_4 > 0)
      {
        str << "FASTUIDRAW_LAYOUT_VARYING(" << location << ") "
            << qualifier << " fastuidraw_varying"
            << " " << types[extra_over_4 - 1] << " "
            << name << append_to_name << return_value << ";\n\n";
        ++return_value;
      }

    return return_value;
  }

  unsigned int
  stream_declare_varyings(fastuidraw::c_string append_to_name, std::ostream &str,
                          size_t uint_count, size_t int_count,
                          fastuidraw::c_array<const size_t> float_counts,
                          unsigned int start_slot)
  {
    unsigned int number_slots(0);
    fastuidraw::c_string uint_labels[]=
      {
        "uint",
        "uvec2",
        "uvec3",
        "uvec4",
      };
    fastuidraw::c_string int_labels[]=
      {
        "int",
        "ivec2",
        "ivec3",
        "ivec4",
      };
    fastuidraw::c_string float_labels[]=
      {
        "float",
        "vec2",
        "vec3",
        "vec4",
      };

    number_slots +=
      stream_declare_varyings_type(append_to_name, start_slot + number_slots, str,
                                   uint_count, "flat", uint_labels, uint_varying_label());

    number_slots +=
      stream_declare_varyings_type(append_to_name, start_slot + number_slots, str,
                                   int_count, "flat", int_labels, int_varying_label());

    number_slots +=
      stream_declare_varyings_type(append_to_name, start_slot + number_slots, str,
                                   float_counts[fastuidraw::glsl::varying_list::interpolation_smooth],
                                   "", float_labels,
                                   float_varying_label(fastuidraw::glsl::varying_list::interpolation_smooth));

    number_slots +=
      stream_declare_varyings_type(append_to_name, start_slot + number_slots, str,
                                   float_counts[fastuidraw::glsl::varying_list::interpolation_flat],
                                   "flat", float_labels,
                                   float_varying_label(fastuidraw::glsl::varying_list::interpolation_flat));

    number_slots +=
      stream_declare_varyings_type(append_to_name, start_slot + number_slots, str,
                                   float_counts[fastuidraw::glsl::varying_list::interpolation_noperspective],
                                   "noperspective", float_labels,
                                   float_varying_label(fastuidraw::glsl::varying_list::interpolation_noperspective));
    return number_slots;
  }


  void
  pre_stream_varyings(fastuidraw::glsl::ShaderSource &dst,
                      const fastuidraw::reference_counted_ptr<fastuidraw::glsl::PainterItemShaderGLSL> &sh,
                      const fastuidraw::glsl::detail::DeclareVaryingsStringDatum &datum)
  {
    fastuidraw::glsl::detail::stream_alias_varyings("_shader", dst, sh->varyings(), true, datum);
  }

  void
  post_stream_varyings(fastuidraw::glsl::ShaderSource &dst,
                       const fastuidraw::reference_counted_ptr<fastuidraw::glsl::PainterItemShaderGLSL> &sh,
                       const fastuidraw::glsl::detail::DeclareVaryingsStringDatum &datum)
  {
    fastuidraw::glsl::detail::stream_alias_varyings("_shader", dst, sh->varyings(), false, datum);
  }

  void
  add_macro_requirment(fastuidraw::glsl::ShaderSource &dst,
                       bool should_be_defined,
                       fastuidraw::c_string macro,
                       fastuidraw::c_string error_message)
  {
    std::ostringstream str;
    fastuidraw::c_string not_cnd, msg;

    not_cnd = (should_be_defined) ? "!defined" : "defined";
    msg = (should_be_defined) ? "" : "not ";
    str << "#if " << not_cnd << "(" << macro << ")\n"
        << "#error \"" << error_message << ": "
        << macro << " should " << msg << "be defined\"\n"
        << "#endif\n";
    dst.add_source(str.str().c_str(), fastuidraw::glsl::ShaderSource::from_string);
  }

  template<typename T>
  class UberShaderStreamer
  {
  public:
    typedef fastuidraw::glsl::ShaderSource ShaderSource;
    typedef fastuidraw::reference_counted_ptr<T> ref_type;
    typedef fastuidraw::c_array<const ref_type> array_type;
    typedef const ShaderSource& (T::*get_src_type)(void) const;
    typedef fastuidraw::glsl::detail::DeclareVaryingsStringDatum Datum;
    typedef void (*pre_post_stream_type)(ShaderSource &dst, const ref_type &sh, const Datum &datum);

    static
    void
    stream_nothing(ShaderSource &, const ref_type &,
                   const Datum &)
    {}

    static
    void
    stream_uber(bool use_switch, ShaderSource &dst, array_type shaders,
                get_src_type get_src,
                pre_post_stream_type pre_stream,
                pre_post_stream_type post_stream,
                const Datum &datum,
                const std::string &return_type,
                const std::string &uber_func_with_args,
                const std::string &shader_main,
                const std::string &shader_args, //of the form ", arg1, arg2,..,argN" or empty string
                const std::string &shader_id);

    static
    void
    stream_uber(bool use_switch, ShaderSource &dst, array_type shaders,
                get_src_type get_src,
                const std::string &return_type,
                const std::string &uber_func_with_args,
                const std::string &shader_main,
                const std::string &shader_args,
                const std::string &shader_id)
    {
      stream_uber(use_switch, dst, shaders, get_src,
                  &stream_nothing, &stream_nothing,
                  fastuidraw::glsl::detail::DeclareVaryingsStringDatum(),
                  return_type, uber_func_with_args,
                  shader_main, shader_args, shader_id);
    }
  private:
    static
    void
    stream_source(ShaderSource &dst, const std::string &prefix,
                  const ShaderSource &shader);
  };

}

template<typename T>
void
UberShaderStreamer<T>::
stream_source(ShaderSource &dst, const std::string &prefix,
              const ShaderSource &shader)
{
  /* This terribly hack is because GLES specfication mandates
   * for GLSL in GLES to not support token pasting (##) in the
   * pre-processor. Many GLES drivers support it anyways, but
   * Mesa does not. Sighs. So we emulate the token pasting
   * for the FASTUIDRAW_LOCAL() macro.
   */
  using namespace fastuidraw;
  std::string src, needle;
  std::string::size_type pos, last_pos;
  std::ostringstream ostr;

  needle = "FASTUIDRAW_LOCAL";
  src = shader.assembled_code(true);

  /* HUNT for FASTUIDRAW_LOCAL(X) anywhere in the code and expand
   * it. NOTE: this is NOT robust at all as it is not a real
   * pre-processor, just a hack. It will fail if the macro
   * invocation is spread across multiple lines or if the agument
   * was a macro itself that needs to expanded by the pre-processor.
   */
  for (last_pos = 0, pos = src.find(needle); pos != std::string::npos; last_pos = pos + 1, pos = src.find(needle, pos + 1))
    {
      std::string::size_type open_pos, close_pos;

      /* stream up to pos */
      if (pos > last_pos)
        {
          ostr << src.substr(last_pos, pos - last_pos);
        }

      /* find the first open and close-parentesis pair after pos. */
      open_pos = src.find('(', pos);
      close_pos = src.find(')', pos);

      if (open_pos != std::string::npos
          && close_pos != std::string::npos
          && open_pos < close_pos)
        {
          std::string tmp;
          tmp = src.substr(open_pos + 1, close_pos - open_pos - 1);
          // trim the string of white spaces.
          tmp.erase(0, tmp.find_first_not_of(" \t"));
          tmp.erase(tmp.find_last_not_of(" \t") + 1);
          ostr << prefix << tmp;
          pos = close_pos;
        }
    }
  ostr << src.substr(last_pos);

  dst.add_source(ostr.str().c_str(), ShaderSource::from_string);
}

template<typename T>
void
UberShaderStreamer<T>::
stream_uber(bool use_switch, ShaderSource &dst, array_type shaders,
            get_src_type get_src,
            pre_post_stream_type pre_stream, pre_post_stream_type post_stream,
            const Datum &datum,
            const std::string &return_type,
            const std::string &uber_func_with_args,
            const std::string &shader_main,
            const std::string &shader_args, //of the form ", arg1, arg2,..,argN" or empty string
            const std::string &shader_id)
{
  /* first stream all of the item_shaders with predefined macros. */
  for(const auto &sh : shaders)
    {
      std::ostringstream start_comment, str, prefix_ostr;
      std::string prefix;

      str << shader_main << sh->ID();
      prefix_ostr << shader_main << "_local_" << sh->ID() << "_";
      prefix = prefix_ostr.str();
      start_comment << "\n/////////////////////////////////////////\n"
                    << "// Start Shader #" << sh->ID() << "\n";

      dst.add_source(start_comment.str().c_str(), ShaderSource::from_string);
      pre_stream(dst, sh, datum);
      dst.add_macro(shader_main.c_str(), str.str().c_str());
      stream_source(dst, prefix, (sh.get()->*get_src)());
      dst.remove_macro(shader_main.c_str());
      post_stream(dst, sh, datum);
    }

  std::ostringstream str;
  bool has_sub_shaders(false), has_return_value(return_type != "void");
  std::string tab;

  str << return_type << "\n"
      << uber_func_with_args << "\n"
      << "{\n";

  if (has_return_value)
    {
      str << "    " << return_type << " p;\n";
    }

  for(const auto &sh : shaders)
    {
      if (sh->number_sub_shaders() > 1)
        {
          unsigned int start, end;
          start = sh->ID();
          end = start + sh->number_sub_shaders();
          if (has_sub_shaders)
            {
              str << "    else ";
            }
          else
            {
              str << "    ";
            }

          str << "if (" << shader_id << " >= uint(" << start
              << ") && " << shader_id << " < uint(" << end << "))\n"
              << "    {\n"
              << "        ";
          if (has_return_value)
            {
              str << "p = ";
            }
          str << shader_main << sh->ID()
              << "(" << shader_id << " - uint(" << start << ")" << shader_args << ");\n"
              << "    }\n";
          has_sub_shaders = true;
        }
    }

  if (has_sub_shaders && use_switch)
    {
      str << "    else\n"
          << "    {\n";
      tab = "        ";
    }
  else
    {
      tab = "    ";
    }

  if (use_switch)
    {
      str << tab << "switch(" << shader_id << ")\n"
          << tab << "{\n";
    }

  bool first_entry(true);
  for(const auto &sh : shaders)
    {
      if (sh->number_sub_shaders() == 1)
        {
          if (use_switch)
            {
              str << tab << "case uint(" << sh->ID() << "):\n";
              str << tab << "    {\n"
                  << tab << "        ";
            }
          else
            {
              if (!first_entry)
                {
                  str << tab << "else if";
                }
              else
                {
                  str << tab << "if";
                }
              str << "(" << shader_id << " == uint(" << sh->ID() << "))\n";
              str << tab << "{\n"
                  << tab << "    ";
            }

          if (has_return_value)
            {
              str << "p = ";
            }

          str << shader_main << sh->ID()
              << "(uint(0)" << shader_args << ");\n";

          if (use_switch)
            {
              str << tab << "    }\n"
                  << tab << "    break;\n\n";
            }
          else
            {
              str << tab << "}\n";
            }
          first_entry = false;
        }
    }

  if (use_switch)
    {
      str << tab << "}\n";
    }

  if (has_sub_shaders && use_switch)
    {
      str << "    }\n";
    }

  if (has_return_value)
    {
      str << "    return p;\n";
    }

  str << "}\n";
  dst.add_source(str.str().c_str(), ShaderSource::from_string);
}


namespace fastuidraw { namespace glsl { namespace detail {

void
stream_alias_varyings(fastuidraw::c_string append_to_name,
                      ShaderSource &shader,
                      const varying_list &p,
                      bool define,
                      const DeclareVaryingsStringDatum &datum)
{
  stream_alias_varyings_array(append_to_name, shader, p.uints(), uint_varying_label(),
                              define, datum.m_uint_special_index);
  stream_alias_varyings_array(append_to_name, shader, p.ints(), int_varying_label(),
                              define, datum.m_int_special_index);

  for(unsigned int i = 0; i < varying_list::interpolation_number_types; ++i)
    {
      enum varying_list::interpolation_qualifier_t q;
      q = static_cast<enum varying_list::interpolation_qualifier_t>(i);
      stream_alias_varyings_array(append_to_name, shader, p.floats(q), float_varying_label(q),
                                  define, datum.m_float_special_index[q]);
    }
}

void
stream_varyings_as_local_variables(ShaderSource &shader, const varying_list &p)
{
  stream_varyings_as_local_variables_array(shader, p.uints(), "uint");
  stream_varyings_as_local_variables_array(shader, p.ints(), "uint");

  for(unsigned int i = 0; i < varying_list::interpolation_number_types; ++i)
    {
      enum varying_list::interpolation_qualifier_t q;
      q = static_cast<enum varying_list::interpolation_qualifier_t>(i);
      stream_varyings_as_local_variables_array(shader, p.floats(q), "float");
    }
}

std::string
declare_varyings_string(fastuidraw::c_string append_to_name,
                        size_t uint_count, size_t int_count,
                        c_array<const size_t> float_counts,
                        unsigned int *slot,
                        DeclareVaryingsStringDatum *datum)
{
  std::ostringstream ostr;

  *slot += stream_declare_varyings(append_to_name, ostr, uint_count, int_count, float_counts, *slot);

  datum->m_uint_special_index = compute_special_index(uint_count);
  datum->m_int_special_index = compute_special_index(int_count);
  for(unsigned int i = 0; i < varying_list::interpolation_number_types; ++i)
    {
      datum->m_float_special_index[i] = compute_special_index(float_counts[i]);
    }

  return ostr.str();
}

void
stream_uber_vert_shader(bool use_switch,
                        ShaderSource &vert,
                        c_array<const reference_counted_ptr<PainterItemShaderGLSL> > item_shaders,
                        const DeclareVaryingsStringDatum &datum)
{
  UberShaderStreamer<PainterItemShaderGLSL>::stream_uber(use_switch, vert, item_shaders,
                                                         &PainterItemShaderGLSL::vertex_src,
                                                         &pre_stream_varyings, &post_stream_varyings, datum,
                                                         "vec4", "fastuidraw_run_vert_shader(in fastuidraw_shader_header h, out int add_z)",
                                                         "fastuidraw_gl_vert_main",
                                                         ", fastuidraw_primary_attribute, fastuidraw_secondary_attribute, "
                                                         "fastuidraw_uint_attribute, h.item_shader_data_location, add_z",
                                                         "h.item_shader");
}

void
stream_uber_frag_shader(bool use_switch,
                        ShaderSource &frag,
                        c_array<const reference_counted_ptr<PainterItemShaderGLSL> > item_shaders,
                        const DeclareVaryingsStringDatum &datum)
{
  UberShaderStreamer<PainterItemShaderGLSL>::stream_uber(use_switch, frag, item_shaders,
                                                         &PainterItemShaderGLSL::fragment_src,
                                                         &pre_stream_varyings, &post_stream_varyings, datum,
                                                         "vec4",
                                                         "fastuidraw_run_frag_shader(in uint frag_shader, "
                                                         "in uint frag_shader_data_location)",
                                                         "fastuidraw_gl_frag_main", ", frag_shader_data_location",
                                                         "frag_shader");
}

void
stream_uber_blend_shader(bool use_switch,
                         ShaderSource &frag,
                         c_array<const reference_counted_ptr<PainterBlendShaderGLSL> > shaders,
                         enum PainterBlendShader::shader_type tp)
{
  std::string sub_func_name, func_name, sub_func_args;
  std::ostringstream check_blend_macros_correct;

  switch(tp)
    {
    default:
      FASTUIDRAWassert("Unknown blend_code_type!");
      //fall through
    case PainterBlendShader::single_src:
      func_name = "fastuidraw_run_blend_shader(in uint blend_shader, in uint blend_shader_data_location, in vec4 in_src, out vec4 out_src)";
      sub_func_name = "fastuidraw_gl_compute_blend_value";
      sub_func_args = ", blend_shader_data_location, in_src, out_src";
      add_macro_requirment(frag, true, "FASTUIDRAW_PAINTER_BLEND_SINGLE_SRC_BLEND", "Mismatch macros determining blend shader type!");
      add_macro_requirment(frag, false, "FASTUIDRAW_PAINTER_BLEND_DUAL_SRC_BLEND", "Mismatch macros determining blend shader type!");
      add_macro_requirment(frag, false, "FASTUIDRAW_PAINTER_BLEND_FRAMEBUFFER_FETCH", "Mismatch macros determining blend shader type!");
      break;

    case PainterBlendShader::dual_src:
      func_name = "fastuidraw_run_blend_shader(in uint blend_shader, in uint blend_shader_data_location, in vec4 color0, out vec4 src0, out vec4 src1)";
      sub_func_name = "fastuidraw_gl_compute_blend_factors";
      sub_func_args = ", blend_shader_data_location, color0, src0, src1";
      add_macro_requirment(frag, false, "FASTUIDRAW_PAINTER_BLEND_SINGLE_SRC_BLEND", "Mismatch macros determining blend shader type");
      add_macro_requirment(frag, true, "FASTUIDRAW_PAINTER_BLEND_DUAL_SRC_BLEND", "Mismatch macros determining blend shader type");
      add_macro_requirment(frag, false, "FASTUIDRAW_PAINTER_BLEND_FRAMEBUFFER_FETCH", "Mismatch macros determining blend shader type");
      break;

    case PainterBlendShader::framebuffer_fetch:
      func_name = "fastuidraw_run_blend_shader(in uint blend_shader, in uint blend_shader_data_location, in vec4 in_src, in vec4 in_fb, out vec4 out_src)";
      sub_func_name = "fastuidraw_gl_compute_post_blended_value";
      sub_func_args = ", blend_shader_data_location, in_src, in_fb, out_src";
      add_macro_requirment(frag, false, "FASTUIDRAW_PAINTER_BLEND_SINGLE_SRC_BLEND", "Mismatch macros determining blend shader type");
      add_macro_requirment(frag, false, "FASTUIDRAW_PAINTER_BLEND_DUAL_SRC_BLEND", "Mismatch macros determining blend shader type");
      add_macro_requirment(frag, true, "FASTUIDRAW_PAINTER_BLEND_FRAMEBUFFER_FETCH", "Mismatch macros determining blend shader type");
      break;
    }
  UberShaderStreamer<PainterBlendShaderGLSL>::stream_uber(use_switch, frag, shaders,
                                                          &PainterBlendShaderGLSL::blend_src,
                                                          "void", func_name,
                                                          sub_func_name, sub_func_args, "blend_shader");
}

}}}
