/*
 * i2s_codec.c  --  Generic I2S ALSA SoC Audio driver
 *
 * Copyright 2010-2011 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/pm.h>
#include <linux/clk.h>
#include <linux/log2.h>
#include <linux/regmap.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/consumer.h>
#include <linux/of_device.h>
#include <sound/core.h>
#include <sound/tlv.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/initval.h>

static const struct snd_soc_dapm_widget i2s_codec_dapm_widgets[] = {
   SND_SOC_DAPM_INPUT("LINE_IN"),
   SND_SOC_DAPM_INPUT("MIC_IN"),

   SND_SOC_DAPM_OUTPUT("HP_OUT"),
   SND_SOC_DAPM_OUTPUT("LINE_OUT"),
};

/* routes for i2s_codec */
static const struct snd_soc_dapm_route i2s_codec_dapm_routes[] = {
   {"Capture", NULL, "LINE_IN"}, /* line_in --> adc_mux */
   {"Capture", NULL, "MIC_IN"},   /* mic_in --> adc_mux */

   {"LINE_OUT", NULL, "Playback"},
   {"HP_OUT", NULL, "Playback"},
};

/* custom function to fetch info of PCM playback volume */
static int dac_info_volsw(struct snd_kcontrol *kcontrol,
           struct snd_ctl_elem_info *uinfo)
{
   uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
   uinfo->count = 2;
   uinfo->value.integer.min = 0;
   uinfo->value.integer.max = 0xfc - 0x3c;
   return 0;
}

static int dac_get_volsw(struct snd_kcontrol *kcontrol,
          struct snd_ctl_elem_value *ucontrol)
{
   ucontrol->value.integer.value[0] = 0x3c;
   ucontrol->value.integer.value[1] = 0x3c;

   return 0;
}

static int dac_put_volsw(struct snd_kcontrol *kcontrol,
          struct snd_ctl_elem_value *ucontrol)
{
   return 0;
}

static const struct snd_kcontrol_new i2s_codec_snd_controls[] = {
   /* SOC_DOUBLE_S8_TLV with invert */
   {
      .iface = SNDRV_CTL_ELEM_IFACE_PCM,
      .name = "PCM Playback Volume",
      .access = SNDRV_CTL_ELEM_ACCESS_TLV_READ |
         SNDRV_CTL_ELEM_ACCESS_READWRITE,
      .info = dac_info_volsw,
      .get = dac_get_volsw,
      .put = dac_put_volsw,
   }
};

/* mute the codec used by alsa core */
static int i2s_codec_digital_mute(struct snd_soc_dai *codec_dai, int mute)
{
   return 0;
}

/* set codec format */
static int i2s_codec_set_dai_fmt(struct snd_soc_dai *codec_dai, unsigned int fmt)
{
   return 0;
}

static int i2s_codec_pcm_hw_params(struct snd_pcm_substream *substream,
              struct snd_pcm_hw_params *params,
              struct snd_soc_dai *dai)
{
   return 0;
}

#define I2S_CODEC_FORMATS (SNDRV_PCM_FMTBIT_S16_LE |\
         SNDRV_PCM_FMTBIT_S20_3LE |\
         SNDRV_PCM_FMTBIT_S24_LE |\
         SNDRV_PCM_FMTBIT_S32_LE)

static const struct snd_soc_dai_ops i2s_codec_ops = {
   .hw_params = i2s_codec_pcm_hw_params,
   .digital_mute = i2s_codec_digital_mute,
   .set_fmt = i2s_codec_set_dai_fmt,
};

static struct snd_soc_dai_driver i2s_codec_dai = {
   .name = "i2s_codec",
   .playback = {
      .stream_name = "Playback",
      .channels_min = 1,
      .channels_max = 2,
      /*
       * only support 8~48K + 96K,
       * TODO modify hw_param to support more
       */
      .rates = SNDRV_PCM_RATE_8000_48000 | SNDRV_PCM_RATE_96000,
      .formats = I2S_CODEC_FORMATS,
   },
   .capture = {
      .stream_name = "Capture",
      .channels_min = 1,
      .channels_max = 2,
      .rates = SNDRV_PCM_RATE_8000_48000 | SNDRV_PCM_RATE_96000,
      .formats = I2S_CODEC_FORMATS,
   },
   .ops = &i2s_codec_ops,
   .symmetric_rates = 1,
};

static struct snd_soc_codec_driver i2s_codec_driver = {
   .component_driver = {
      .controls      = i2s_codec_snd_controls,
      .num_controls     = ARRAY_SIZE(i2s_codec_snd_controls),
      .dapm_widgets     = i2s_codec_dapm_widgets,
      .num_dapm_widgets = ARRAY_SIZE(i2s_codec_dapm_widgets),
      .dapm_routes      = i2s_codec_dapm_routes,
      .num_dapm_routes  = ARRAY_SIZE(i2s_codec_dapm_routes),
   },
};

static int i2s_codec_codec_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
   int ret;

   dev_info(dev, "i2s_codec probe start\n");

   ret = snd_soc_register_codec(dev, &i2s_codec_driver, &i2s_codec_dai, 1);

   if (ret) {
      dev_err(dev, "%s: snd_soc_register_codec() failed (%d)\n",
         __func__, ret);
      return ret;
   }

   dev_info(dev, "i2s_codec probe exit %x\n", ret);
   return ret;
}

static int i2s_codec_codec_remove(struct platform_device *pdev)
{
   snd_soc_unregister_codec(&pdev->dev);
   return 0;
}

static const struct of_device_id i2s_codec_dt_ids[] = {
	{ .compatible = "fsl,i2s_codec", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, i2s_codec_dt_ids);

static struct platform_driver i2s_codec_codec_driver = {
   .driver = {
         .name = "i2s_codec",
		   .of_match_table = i2s_codec_dt_ids,
         },
   .probe = i2s_codec_codec_probe,
   .remove = i2s_codec_codec_remove,
};

module_platform_driver(i2s_codec_codec_driver);

MODULE_DESCRIPTION("Generic I2S ALSA SoC Codec Driver");
MODULE_AUTHOR("Zeng Zhaoming <zengzm.kernel@gmail.com>");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:i2s_codec");

