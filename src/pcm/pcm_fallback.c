/**
 * \file pcm/pcm_fallback.c
 * \ingroup PCM_Plugins
 * \brief PCM Fallback Plugin Interface
 * \author Takashi Iwai <tiwai@suse.de>
 * \date 2016
 */

#include "pcm_local.h"

#ifndef PIC
/* entry for static linking */
const char *_snd_module_pcm_fallback = "";
#endif

/*! \page pcm_plugins

\section pcm_plugins_fallback Plugin: fallback

This plugin allows user to give multiple slave PCMs.
At opening the PCM, the plugin tries each slave PCM until the
open of the slave PCM succeeds or returns -EAGAIN.

\code
pcm.name {
	type fallback		# Fallback PCM
	slaves {		# Slaves definition
		ID STR		# Slave PCM name
		# or
		ID {
			pcm STR		# Slave PCM name
			# or
			pcm { } 	# Slave PCM definition
		}
	}
}
\endcode

\subsection pcm_plugins_fallback_funcref Function reference

<UL>
  <LI>_snd_pcm_fallback_open()
</UL>

*/

/**
 * \brief Creates a new fallback stream PCM
 * \param pcmp Returns created PCM handle
 * \param name Name of PCM
 * \param root Root configuration node
 * \param conf Configuration node with copy PCM description
 * \param stream Stream type
 * \param mode Stream mode
 * \retval zero on success otherwise a negative error code
 * \warning Using of this function might be dangerous in the sense
 *          of compatibility reasons. The prototype might be freely
 *          changed in future.
 */
int _snd_pcm_fallback_open(snd_pcm_t **pcmp, const char *name ATTRIBUTE_UNUSED,
			   snd_config_t *root, snd_config_t *conf,
			   snd_pcm_stream_t stream, int mode)
{
	snd_config_iterator_t i, next;
	snd_config_t *slaves = NULL, *sconf;
	snd_pcm_t *pcm;
	int err = -ENODEV;

	snd_config_for_each(i, next, conf) {
		snd_config_t *n = snd_config_iterator_entry(i);
		const char *id;
		if (snd_config_get_id(n, &id) < 0)
			continue;
		if (snd_pcm_conf_generic_id(id))
			continue;
		if (strcmp(id, "slaves") == 0) {
			if (snd_config_get_type(n) != SND_CONFIG_TYPE_COMPOUND) {
				SNDERR("Invalid type for %s", id);
				return -EINVAL;
			}
			slaves = n;
			continue;
		}
		SNDERR("Unknown field %s", id);
		return -EINVAL;
	}

	if (!slaves) {
		SNDERR("slaves is not defined");
		return -EINVAL;
	}

	snd_config_for_each(i, next, slaves) {
		snd_config_t *m = snd_config_iterator_entry(i);
		err = snd_pcm_slave_conf(root, m, &sconf, 0);
		if (err < 0)
			return err;
		err = snd_pcm_open_slave(&pcm, root, sconf, stream, mode, conf);
		snd_config_delete(sconf);
		if (err >= 0 || err == -EAGAIN) {
			*pcmp = pcm;
			return err;
		}
	}
	return err;
}
#ifndef DOC_HIDDEN
SND_DLSYM_BUILD_VERSION(_snd_pcm_fallback_open, SND_PCM_DLSYM_VERSION);
#endif
