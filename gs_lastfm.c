/*
 * Copyright (c) 2014 Alyx Wolcott <alyx -at- malkier.net>
 * Rights to this code are as documented in doc/LICENSE.
 *
 * To use this module, add a lastfm_api value to the gameserv{}
 * config block containing your Last.FM API key.
 */

#include "../api/http.h"
#include "../gameserv/gameserv_common.h"

DECLARE_MODULE_V1(
	"contrib/gs_lastfm", false, _modinit, _moddeinit,
	PACKAGE_STRING,
	"Atheme Development Group <http://www.atheme.org>"
);

static inline void sfree(char *p)
{
	if (p != NULL) free(p);
}

static char *api_key;

struct np_track
{
	char *user;
	char *name;
	char *artist;
	char *album;
	bool nowplaying;
	bool loved;
};

static void command_np(sourceinfo_t *si, int parc, char *parv[]);

command_t cmd_lastfm = { "NP", N_("Find last played song from Last.FM"), AC_NONE, 2,
	command_np, {.path = "gameserv/lastfm"}};

void _modinit(module_t *m)
{
	service_t *gamesvs;
	MODULE_TRY_REQUEST_DEPENDENCY(m, "gameserv/main");
	gamesvs = service_find("gameserv");
	add_dupstr_conf_item("LASTFM_API", &gamesvs->conf_table, 0, &api_key, NULL);

	use_http_symbols(m);
	service_named_bind_command("gameserv", &cmd_lastfm);
	service_named_bind_command("chanserv", &cmd_lastfm);
}

void _moddeinit(module_unload_intent_t intent)
{
	service_t *gamesvs;
	gamesvs = service_find("gameserv");
	service_named_unbind_command("gameserv", &cmd_lastfm);
	service_named_unbind_command("chanserv", &cmd_lastfm);
	del_conf_item("LASTFM_API", &gamesvs->conf_table);
}

int get_first_track(const char *key, void *data, void *privdata)
{
	mowgli_json_t *n, *m;
	struct np_track *t;

	n = (mowgli_json_t *)data;
	t = (struct np_track *)privdata;

	if (strcmp(key, "name") == 0)
	{
		t->name = strdup(n->v.v_string->str);
	}
	else if (strcmp(key, "loved") == 0)
	{
		if (strcmp(n->v.v_string->str, "0") == 0)
			t->loved = false;
		else
			t->loved = true;
	}
	else if (strcmp(key, "@attr") == 0)
	{
		m = mowgli_patricia_retrieve(n->v.v_object, "nowplaying");
		if (m != NULL)
			if (strcmp(m->v.v_string->str, "0") == 0)
				t->nowplaying = false;
			else
				t->nowplaying = true;
	}
	else if (strcmp(key, "artist") == 0)
	{
		m = mowgli_patricia_retrieve(n->v.v_object, "name");
		if (m != NULL)
			t->artist = strdup(m->v.v_string->str);
	}
	else if (strcmp(key, "album") == 0)
	{
		m = mowgli_patricia_retrieve(n->v.v_object, "#text");
		if (m != NULL)
			t->album = strdup(m->v.v_string->str);
	}

	return 1;
}

void user_getrecenttracks_consumer(http_client_t *container, void *userdata)
{
	sourceinfo_t *si;
	mowgli_json_t *json, *curr;
	struct np_track *track;

	track = smalloc(sizeof(struct np_track));
	si = (sourceinfo_t *)userdata;

	track->name = track->artist = track->album = NULL;
	track->loved = track->nowplaying = false;

	printf("%s\n", container->data);

	json = mowgli_json_parse_string(container->data);

	if (json == NULL || json->tag != MOWGLI_JSON_TAG_OBJECT)
	{
		gs_command_report(si, "LastFM lookup failed (Missing everything)");
		goto cleanup;
	}
	if ((json = mowgli_json_object_retrieve(json, "recenttracks"))
			== NULL)
	{
		gs_command_report(si, "LastFM lookup failed (missing recenttracks)");
		goto cleanup;
	}
	if ((curr = mowgli_json_object_retrieve(json, "@attr")) == NULL)
	{
		gs_command_report(si, "LastFM lookup failed (missing @attr)");
		goto cleanup;
	}
	if ((curr = mowgli_json_object_retrieve(curr, "user")) == NULL)
	{
		gs_command_report(si, "LastFM lookup failed (missing user)");
		goto cleanup;
	} else {
		track->user = sstrdup(curr->v.v_string->str);
	}
	if ((json = mowgli_json_object_retrieve(json, "track")) == NULL)
	{
		gs_command_report(si, "LastFM lookup failed (Missing track)");
		goto cleanup;
	}
	/*json = mowgli_json_array_nth(json, 0);*/
	mowgli_patricia_foreach(json->v.v_object, get_first_track, track);

	if (track->album)
		gs_command_report(si, "%s %s %s by %s (%s) %s\n", track->user, 
				(track->nowplaying ? "is now playing" : "last played"),
				track->name, track->artist, track->album, 
				(track->loved ? "[Loved Track]" : ""));
	else
		gs_command_report(si, "%s %s %s by %s %s\n", track->user, 
				(track->nowplaying ? "is now playing" : "last played"),
				track->name, track->artist, (track->loved ? "[Loved Track]" : ""));

cleanup:
	sfree(track->user);
	sfree(track->name);
	sfree(track->album);
	sfree(track->artist);
	free(track);
	object_unref(si);

}

static void command_np(sourceinfo_t *si, int parc, char *parv[])
{
	http_client_t *container;
	mychan_t *mc;

	if (!gs_do_parameters(si, &parc, &parv, &mc))
		return;


	if (parc < 1)
	{
		command_fail(si, fault_needmoreparams, STR_INSUFFICIENT_PARAMS, "NP");
		command_fail(si, fault_needmoreparams, _("Usage: NP <username>"));
		return;
	}

	if (api_key == NULL)
	{
		gs_command_report(si, "LastFM lookup failed (Incorrect config)");
		slog(LG_INFO, "Missing LastFM API key");
		return;
	}

	container = http_client_new();

	http_parse_url(container, "http://ws.audioscrobbler.com/2.0/");
	http_add_param(container, "method", "user.getrecenttracks");
	http_add_param(container, "user", parv[0]);
	http_add_param(container, "extended", "1");
	http_add_param(container, "limit", "1");
	http_add_param(container, "format", "json");
	http_add_param(container, "api_key", api_key);

	object_ref(si);
	http_get(container, si, user_getrecenttracks_consumer);
}
