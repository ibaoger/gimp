/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * file-jpegxl - JPEG XL file format plug-in for the GIMP
 * Copyright (C) 2022  Daniel Novomesky
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <glib/gstdio.h>

#include <jxl/decode.h>
#include <jxl/encode.h>
#include <jxl/thread_parallel_runner.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "libgimp/stdplugins-intl.h"

#define LOAD_PROC      "file-jpegxl-load"
#define PLUG_IN_BINARY "file-jpegxl"

static void query (void);
static void run (const gchar     *name,
                 gint             nparams,
                 const GimpParam *param,
                 gint            *nreturn_vals,
                 GimpParam      **return_vals);

GimpPlugInInfo PLUG_IN_INFO = {
  NULL,  /* init_proc  */
  NULL,  /* quit_proc  */
  query, /* query_proc */
  run,   /* run_proc   */
};

MAIN ()

static void
query (void)
{
  static const GimpParamDef load_args[] = {
    { GIMP_PDB_INT32,  "run-mode",     "The run mode { RUN-INTERACTIVE (0), RUN-NONINTERACTIVE (1) }"},
    { GIMP_PDB_STRING, "filename",     "The name of the file to load"                                },
    { GIMP_PDB_STRING, "raw-filename", "The name of the file to load"                                }
  };
  static const GimpParamDef load_return_vals[] = {
    {GIMP_PDB_IMAGE, "image", "Output image"}
  };

  gimp_install_procedure (LOAD_PROC,
                          "Loads files in the JPEG XL file format",
                          "Loads files in the JPEG XL file format",
                          "Daniel Novomesky",
                          "(C) 2022 Daniel Novomesky",
                          "2022",
                          N_("JPEG XL image"),
                          NULL,
                          GIMP_PLUGIN,
                          G_N_ELEMENTS (load_args),
                          G_N_ELEMENTS (load_return_vals),
                          load_args, load_return_vals);

  gimp_register_file_handler_mime (LOAD_PROC, "image/jxl");
  gimp_register_magic_load_handler (LOAD_PROC, "jxl", "", "0,string,\xFF\x0A,0,string,\\000\\000\\000\x0CJXL\\040\\015\\012\x87\\012");
  gimp_register_file_handler_priority (LOAD_PROC, 100);
}

static gint32
load_image (const gchar *filename,
            GError     **error)
{
  FILE *inputFile = g_fopen (filename, "rb");

  gsize    inputFileSize;
  gpointer memory;

  JxlSignature      signature;
  JxlDecoder       *decoder;
  void             *runner;
  JxlBasicInfo      basicinfo;
  JxlDecoderStatus  status;
  JxlPixelFormat    pixel_format;
  JxlColorEncoding  color_encoding;
  size_t            icc_size   = 0;
  GimpColorProfile *profile    = NULL;
  gboolean          loadlinear = FALSE;
  size_t            channel_depth;
  size_t            result_size;
  gpointer          picture_buffer;
  gint32            image = -1;
  gint32            layer;
  GeglBuffer       *buffer;
  GimpPrecision     precision_linear;
  GimpPrecision     precision_non_linear;
  size_t            num_worker_threads = 1;

  if (! inputFile)
    {
      g_set_error (error, G_FILE_ERROR, 0, "Cannot open file for read: %s\n", filename);
      return -1;
    }

  fseek (inputFile, 0, SEEK_END);
  inputFileSize = ftell (inputFile);
  fseek (inputFile, 0, SEEK_SET);

  if (inputFileSize < 1)
    {
      g_set_error (error, G_FILE_ERROR, 0, "File too small: %s\n", filename);
      fclose (inputFile);
      return -1;
    }

  memory = g_malloc (inputFileSize);
  if (fread (memory, 1, inputFileSize, inputFile) != inputFileSize)
    {
      g_set_error (error, G_FILE_ERROR, 0, "Failed to read %zu bytes: %s\n",
                   inputFileSize, filename);
      fclose (inputFile);
      g_free (memory);
      return -1;
    }

  fclose (inputFile);

  signature = JxlSignatureCheck (memory, inputFileSize);
  if (signature != JXL_SIG_CODESTREAM && signature != JXL_SIG_CONTAINER)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   "File %s is probably not in JXL format!\n", filename);
      g_free (memory);
      return -1;
    }

  decoder = JxlDecoderCreate (NULL);
  if (! decoder)
    {
      g_set_error (error, G_FILE_ERROR, 0, "ERROR: JxlDecoderCreate failed");
      g_free (memory);
      return -1;
    }

  num_worker_threads = g_get_num_processors ();
  if (num_worker_threads > 16)
    {
      num_worker_threads = 16;
    }
  runner = JxlThreadParallelRunnerCreate (NULL, num_worker_threads);
  if (JxlDecoderSetParallelRunner (decoder, JxlThreadParallelRunner, runner) != JXL_DEC_SUCCESS)
    {
      g_set_error (error, G_FILE_ERROR, 0, "ERROR: JxlDecoderSetParallelRunner failed");
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return -1;
    }

  if (JxlDecoderSetInput (decoder, memory, inputFileSize) != JXL_DEC_SUCCESS)
    {
      g_set_error (error, G_FILE_ERROR, 0, "ERROR: JxlDecoderSetInput failed");
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return -1;
    }

  if (JxlDecoderSubscribeEvents (decoder, JXL_DEC_BASIC_INFO | JXL_DEC_COLOR_ENCODING | JXL_DEC_FULL_IMAGE)
      != JXL_DEC_SUCCESS)
    {
      g_set_error (error, G_FILE_ERROR, 0, "ERROR: JxlDecoderSubscribeEvents failed");
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return -1;
    }

  status = JxlDecoderProcessInput (decoder);
  if (status == JXL_DEC_ERROR)
    {
      g_set_error (error, G_FILE_ERROR, 0, "ERROR: JXL decoding failed");
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return -1;
    }

  if (status == JXL_DEC_NEED_MORE_INPUT)
    {
      g_set_error (error, G_FILE_ERROR, 0, "ERROR: JXL data incomplete");
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return -1;
    }

  status = JxlDecoderGetBasicInfo (decoder, &basicinfo);
  if (status != JXL_DEC_SUCCESS)
    {
      g_set_error (error, G_FILE_ERROR, 0, "ERROR: JXL basic info not available");
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return -1;
    }

  if (basicinfo.xsize == 0 || basicinfo.ysize == 0)
    {
      g_set_error (error, G_FILE_ERROR, 0, "ERROR: JXL image has zero dimensions");
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return -1;
    }

  status = JxlDecoderProcessInput (decoder);
  if (status != JXL_DEC_COLOR_ENCODING)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   "Unexpected event %d instead of JXL_DEC_COLOR_ENCODING", status);
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return -1;
    }

  if (basicinfo.uses_original_profile == JXL_FALSE)
    {
      if (basicinfo.num_color_channels == 3)
        {
          JxlColorEncodingSetToSRGB (&color_encoding, JXL_FALSE);
          JxlDecoderSetPreferredColorProfile (decoder, &color_encoding);
        }
      else if (basicinfo.num_color_channels == 1)
        {
          JxlColorEncodingSetToSRGB (&color_encoding, JXL_TRUE);
          JxlDecoderSetPreferredColorProfile (decoder, &color_encoding);
        }
    }

  pixel_format.endianness = JXL_NATIVE_ENDIAN;
  pixel_format.align      = 0;

  if (basicinfo.uses_original_profile == JXL_FALSE || basicinfo.bits_per_sample > 16)
    {
      pixel_format.data_type = JXL_TYPE_FLOAT;
      channel_depth          = 4;
      precision_linear       = GIMP_PRECISION_FLOAT_LINEAR;
      precision_non_linear   = GIMP_PRECISION_FLOAT_GAMMA;
    }
  else if (basicinfo.bits_per_sample <= 8)
    {
      pixel_format.data_type = JXL_TYPE_UINT8;
      channel_depth          = 1;
      precision_linear       = GIMP_PRECISION_U8_LINEAR;
      precision_non_linear   = GIMP_PRECISION_U8_GAMMA;
    }
  else
    {
      pixel_format.data_type = JXL_TYPE_UINT16;
      channel_depth          = 2;
      precision_linear       = GIMP_PRECISION_U16_LINEAR;
      precision_non_linear   = GIMP_PRECISION_U16_GAMMA;
    }

  if (basicinfo.num_color_channels == 1) /* grayscale */
    {
      if (basicinfo.alpha_bits > 0)
        {
          pixel_format.num_channels = 2;
        }
      else
        {
          pixel_format.num_channels = 1;
        }
    }
  else /* RGB */
    {

      if (basicinfo.alpha_bits > 0) /* RGB with alpha */
        {
          pixel_format.num_channels = 4;
        }
      else /* RGB no alpha */
        {
          pixel_format.num_channels = 3;
        }
    }

  result_size = channel_depth * pixel_format.num_channels
                * (size_t) basicinfo.xsize * (size_t) basicinfo.ysize;

  if (JxlDecoderGetColorAsEncodedProfile (decoder, &pixel_format,
                                          JXL_COLOR_PROFILE_TARGET_DATA,
                                          &color_encoding) == JXL_DEC_SUCCESS)
    {
      if (color_encoding.white_point == JXL_WHITE_POINT_D65)
        {
          switch (color_encoding.transfer_function)
            {
            case JXL_TRANSFER_FUNCTION_LINEAR:
              loadlinear = TRUE;

              switch (color_encoding.color_space)
                {
                case JXL_COLOR_SPACE_RGB:
                  profile = gimp_color_profile_new_rgb_srgb_linear ();
                  break;
                case JXL_COLOR_SPACE_GRAY:
                  profile = gimp_color_profile_new_d65_gray_linear ();
                  break;
                default:
                  break;
                }
              break;
            case JXL_TRANSFER_FUNCTION_SRGB:
              switch (color_encoding.color_space)
                {
                case JXL_COLOR_SPACE_RGB:
                  profile = gimp_color_profile_new_rgb_srgb ();
                  break;
                case JXL_COLOR_SPACE_GRAY:
                  profile = gimp_color_profile_new_d65_gray_srgb_trc ();
                  break;
                default:
                  break;
                }
              break;
            default:
              break;
            }
        }
    }

  if (! profile)
    {
      if (JxlDecoderGetICCProfileSize (decoder, &pixel_format,
                                       JXL_COLOR_PROFILE_TARGET_DATA,
                                       &icc_size) == JXL_DEC_SUCCESS)
        {
          if (icc_size > 0)
            {
              gpointer raw_icc_profile = g_malloc (icc_size);

              if (JxlDecoderGetColorAsICCProfile (decoder, &pixel_format, JXL_COLOR_PROFILE_TARGET_DATA,
                                                  raw_icc_profile, icc_size)
                  == JXL_DEC_SUCCESS)
                {
                  profile = gimp_color_profile_new_from_icc_profile (raw_icc_profile,
                                                                     icc_size, error);
                  if (profile)
                    {
                      loadlinear = gimp_color_profile_is_linear (profile);
                    }
                  else
                    {
                      g_printerr ("%s: Failed to read ICC profile: %s\n",
                                  G_STRFUNC, (*error)->message);
                      g_clear_error (error);
                    }
                }
              else
                {
                  g_printerr ("Failed to obtain data from JPEG XL decoder");
                }

              g_free (raw_icc_profile);
            }
          else
            {
              g_printerr ("Empty ICC data");
            }
        }
      else
        {
          g_message ("no ICC, other color profile");
        }
    }

  status = JxlDecoderProcessInput (decoder);
  if (status != JXL_DEC_NEED_IMAGE_OUT_BUFFER)
    {
      g_set_error (error, G_FILE_ERROR,
                   0, "Unexpected event %d instead of JXL_DEC_NEED_IMAGE_OUT_BUFFER", status);
      if (profile)
        {
          g_object_unref (profile);
        }
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return -1;
    }

  picture_buffer = g_try_malloc (result_size);
  if (! picture_buffer)
    {
      g_set_error (error, G_FILE_ERROR, 0, "Memory could not be allocated.");
      if (profile)
        {
          g_object_unref (profile);
        }
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return -1;
    }

  if (JxlDecoderSetImageOutBuffer (decoder, &pixel_format, picture_buffer, result_size) != JXL_DEC_SUCCESS)
    {
      g_set_error (error, G_FILE_ERROR, 0, "ERROR: JxlDecoderSetImageOutBuffer failed");
      if (profile)
        {
          g_object_unref (profile);
        }
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return -1;
    }

  status = JxlDecoderProcessInput (decoder);
  if (status != JXL_DEC_FULL_IMAGE)
    {
      g_set_error (error, G_FILE_ERROR, 0,
                   "Unexpected event %d instead of JXL_DEC_FULL_IMAGE", status);
      g_free (picture_buffer);
      if (profile)
        {
          g_object_unref (profile);
        }
      JxlThreadParallelRunnerDestroy (runner);
      JxlDecoderDestroy (decoder);
      g_free (memory);
      return -1;
    }

  if (basicinfo.num_color_channels == 1) /* grayscale */
    {
      image = gimp_image_new_with_precision (basicinfo.xsize, basicinfo.ysize, GIMP_GRAY,
                                             loadlinear ? precision_linear : precision_non_linear);

      if (profile)
        {
          if (gimp_color_profile_is_gray (profile))
            {
              gimp_image_set_color_profile (image, profile);
            }
        }

      layer = gimp_layer_new (image, "Background", basicinfo.xsize, basicinfo.ysize,
                              (basicinfo.alpha_bits > 0) ? GIMP_GRAYA_IMAGE : GIMP_GRAY_IMAGE,
                              100, gimp_image_get_default_new_layer_mode (image));
    }
  else /* RGB */
    {
      image = gimp_image_new_with_precision (basicinfo.xsize, basicinfo.ysize, GIMP_RGB,
                                             loadlinear ? precision_linear : precision_non_linear);

      if (profile)
        {
          if (gimp_color_profile_is_rgb (profile))
            {
              gimp_image_set_color_profile (image, profile);
            }
        }

      layer = gimp_layer_new (image, "Background", basicinfo.xsize, basicinfo.ysize,
                              (basicinfo.alpha_bits > 0) ? GIMP_RGBA_IMAGE : GIMP_RGB_IMAGE,
                              100, gimp_image_get_default_new_layer_mode (image));
    }

  gimp_image_insert_layer (image, layer, -1, 0);

  buffer = gimp_drawable_get_buffer (layer);

  gegl_buffer_set (buffer, GEGL_RECTANGLE (0, 0, basicinfo.xsize, basicinfo.ysize),
                   0, NULL, picture_buffer, GEGL_AUTO_ROWSTRIDE);

  g_object_unref (buffer);

  g_free (picture_buffer);
  if (profile)
    {
      g_object_unref (profile);
    }
  JxlThreadParallelRunnerDestroy (runner);
  JxlDecoderDestroy (decoder);
  g_free (memory);
  return image;
}

static void
run (const gchar     *name,
     gint             nparams,
     const GimpParam *param,
     gint            *nreturn_vals,
     GimpParam      **return_vals)
{
  static GimpParam  values[6];
  GimpRunMode       run_mode;
  GimpPDBStatusType status = GIMP_PDB_SUCCESS;
  gint32            image_ID;
  GError           *error = NULL;

  run_mode = param[0].data.d_int32;

  INIT_I18N ();
  gegl_init (NULL, NULL);

  *nreturn_vals           = 1;
  *return_vals            = values;
  values[0].type          = GIMP_PDB_STATUS;
  values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;

  if (strcmp (name, LOAD_PROC) == 0)
    {
      switch (run_mode)
        {
        case GIMP_RUN_INTERACTIVE:
        case GIMP_RUN_WITH_LAST_VALS:
          gimp_ui_init (PLUG_IN_BINARY, FALSE);
          break;
        default:
          break;
        }

      image_ID = load_image (param[1].data.d_string, &error);

      if (image_ID != -1)
        {
          *nreturn_vals          = 2;
          values[1].type         = GIMP_PDB_IMAGE;
          values[1].data.d_image = image_ID;
        }
      else
        {
          status = GIMP_PDB_EXECUTION_ERROR;
        }
    }
  else
    {
      status = GIMP_PDB_CALLING_ERROR;
    }

  if (status != GIMP_PDB_SUCCESS && error)
    {
      *nreturn_vals           = 2;
      values[1].type          = GIMP_PDB_STRING;
      values[1].data.d_string = error->message;
    }

  values[0].data.d_status = status;
}