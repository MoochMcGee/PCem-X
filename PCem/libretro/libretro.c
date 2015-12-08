#include "libretro/libretro.h"
#include "ibm.h"
#include "cpu.h"
#include "model.h"
#include "nvr.h"
#include "video.h"

retro_log_printf_t log_cb = NULL;
retro_video_refresh_t video_cb = NULL;
retro_input_poll_t poll_cb = NULL;
retro_input_state_t input_cb = NULL;
retro_audio_sample_batch_t audio_batch_cb = NULL;
retro_environment_t environ_cb = NULL;

void retro_get_system_info(struct retro_system_info *info)
{
   info->library_name = "PCem-X";
   info->library_version = "v10";
   info->valid_extensions = "";
   info->need_fullpath = true;
   info->block_extract = false;
}

const char* retro_get_system_directory(void)
{
    const char* dir;
    environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &dir);

    return dir ? dir : ".";
}

void retro_set_environment(retro_environment_t cb)
{
   environ_cb = cb;

   setup_variables();
}

void retro_init()
{
        int frames = 0;
        int c, d;
        install_timer();
        install_int_ex(timer_rout, BPS_TO_TIMER(100));
	install_int_ex(onesec, BPS_TO_TIMER(1));
	midi_init();

        initpc();

        struct retro_log_callback log;
   unsigned colorMode = RETRO_PIXEL_FORMAT_XRGB8888;
   screen_pitch = 0;

   if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
      log_cb = log.log;
   else
      log_cb = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_PERF_INTERFACE, &perf_cb))
      perf_get_cpu_features_cb = perf_cb.get_cpu_features;
   else
      perf_get_cpu_features_cb = NULL;

   environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &colorMode);


   environ_cb(RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE, &rumble);
}

void retro_deinit()
{
  closepc();

  midi_close();
}

unsigned retro_api_version(void) { return RETRO_API_VERSION; }
