/* shared.c: global vars + some callback functions
 *
 * Copyright (C) 2006 Lars Hjemli
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"

struct repolist cgit_repolist;
struct repoinfo *cgit_repo;
int cgit_cmd;

const char *cgit_version = CGIT_VERSION;

char *cgit_root_title   = "Git repository browser";
char *cgit_css          = "/cgit.css";
char *cgit_logo         = "/git-logo.png";
char *cgit_index_header = NULL;
char *cgit_index_info   = NULL;
char *cgit_logo_link    = "http://www.kernel.org/pub/software/scm/git/docs/";
char *cgit_module_link  = "./?repo=%s&page=commit&id=%s";
char *cgit_agefile      = "info/web/last-modified";
char *cgit_virtual_root = NULL;
char *cgit_script_name  = CGIT_SCRIPT_NAME;
char *cgit_cache_root   = CGIT_CACHE_ROOT;
char *cgit_repo_group   = NULL;
char *cgit_robots       = "index, nofollow";
char *cgit_clone_prefix = NULL;

int cgit_nocache               =  0;
int cgit_snapshots             =  0;
int cgit_enable_index_links    =  0;
int cgit_enable_log_filecount  =  0;
int cgit_enable_log_linecount  =  0;
int cgit_max_lock_attempts     =  5;
int cgit_cache_root_ttl        =  5;
int cgit_cache_repo_ttl        =  5;
int cgit_cache_dynamic_ttl     =  5;
int cgit_cache_static_ttl      = -1;
int cgit_cache_max_create_time =  5;
int cgit_summary_log           =  0;
int cgit_summary_tags          =  0;
int cgit_summary_branches      =  0;
int cgit_renamelimit           = -1;

int cgit_max_msg_len = 60;
int cgit_max_repodesc_len = 60;
int cgit_max_commit_count = 50;

int cgit_query_has_symref = 0;
int cgit_query_has_sha1   = 0;

char *cgit_querystring  = NULL;
char *cgit_query_repo   = NULL;
char *cgit_query_page   = NULL;
char *cgit_query_head   = NULL;
char *cgit_query_search = NULL;
char *cgit_query_grep   = NULL;
char *cgit_query_sha1   = NULL;
char *cgit_query_sha2   = NULL;
char *cgit_query_path   = NULL;
char *cgit_query_name   = NULL;
int   cgit_query_ofs    = 0;

int htmlfd = 0;


int cgit_get_cmd_index(const char *cmd)
{
	static char *cmds[] = {"log", "commit", "diff", "tree", "blob",
			       "snapshot", "tag", "refs", "patch", NULL};
	int i;

	for(i = 0; cmds[i]; i++)
		if (!strcmp(cmd, cmds[i]))
			return i + 1;
	return 0;
}

int chk_zero(int result, char *msg)
{
	if (result != 0)
		die("%s: %s", msg, strerror(errno));
	return result;
}

int chk_positive(int result, char *msg)
{
	if (result <= 0)
		die("%s: %s", msg, strerror(errno));
	return result;
}

int chk_non_negative(int result, char *msg)
{
    	if (result < 0)
	    	die("%s: %s",msg, strerror(errno));
	return result;
}

struct repoinfo *add_repo(const char *url)
{
	struct repoinfo *ret;

	if (++cgit_repolist.count > cgit_repolist.length) {
		if (cgit_repolist.length == 0)
			cgit_repolist.length = 8;
		else
			cgit_repolist.length *= 2;
		cgit_repolist.repos = xrealloc(cgit_repolist.repos,
					       cgit_repolist.length *
					       sizeof(struct repoinfo));
	}

	ret = &cgit_repolist.repos[cgit_repolist.count-1];
	ret->url = trim_end(url, '/');
	ret->name = ret->url;
	ret->path = NULL;
	ret->desc = "[no description]";
	ret->owner = NULL;
	ret->group = cgit_repo_group;
	ret->defbranch = "master";
	ret->snapshots = cgit_snapshots;
	ret->enable_log_filecount = cgit_enable_log_filecount;
	ret->enable_log_linecount = cgit_enable_log_linecount;
	ret->module_link = cgit_module_link;
	ret->readme = NULL;
	return ret;
}

struct repoinfo *cgit_get_repoinfo(const char *url)
{
	int i;
	struct repoinfo *repo;

	for (i=0; i<cgit_repolist.count; i++) {
		repo = &cgit_repolist.repos[i];
		if (!strcmp(repo->url, url))
			return repo;
	}
	return NULL;
}

void cgit_global_config_cb(const char *name, const char *value)
{
	if (!strcmp(name, "root-title"))
		cgit_root_title = xstrdup(value);
	else if (!strcmp(name, "css"))
		cgit_css = xstrdup(value);
	else if (!strcmp(name, "logo"))
		cgit_logo = xstrdup(value);
	else if (!strcmp(name, "index-header"))
		cgit_index_header = xstrdup(value);
	else if (!strcmp(name, "index-info"))
		cgit_index_info = xstrdup(value);
	else if (!strcmp(name, "logo-link"))
		cgit_logo_link = xstrdup(value);
	else if (!strcmp(name, "module-link"))
		cgit_module_link = xstrdup(value);
	else if (!strcmp(name, "virtual-root")) {
		cgit_virtual_root = trim_end(value, '/');
		if (!cgit_virtual_root && (!strcmp(value, "/")))
			cgit_virtual_root = "";
	} else if (!strcmp(name, "nocache"))
		cgit_nocache = atoi(value);
	else if (!strcmp(name, "snapshots"))
		cgit_snapshots = cgit_parse_snapshots_mask(value);
	else if (!strcmp(name, "enable-index-links"))
		cgit_enable_index_links = atoi(value);
	else if (!strcmp(name, "enable-log-filecount"))
		cgit_enable_log_filecount = atoi(value);
	else if (!strcmp(name, "enable-log-linecount"))
		cgit_enable_log_linecount = atoi(value);
	else if (!strcmp(name, "cache-root"))
		cgit_cache_root = xstrdup(value);
	else if (!strcmp(name, "cache-root-ttl"))
		cgit_cache_root_ttl = atoi(value);
	else if (!strcmp(name, "cache-repo-ttl"))
		cgit_cache_repo_ttl = atoi(value);
	else if (!strcmp(name, "cache-static-ttl"))
		cgit_cache_static_ttl = atoi(value);
	else if (!strcmp(name, "cache-dynamic-ttl"))
		cgit_cache_dynamic_ttl = atoi(value);
	else if (!strcmp(name, "max-message-length"))
		cgit_max_msg_len = atoi(value);
	else if (!strcmp(name, "max-repodesc-length"))
		cgit_max_repodesc_len = atoi(value);
	else if (!strcmp(name, "max-commit-count"))
		cgit_max_commit_count = atoi(value);
	else if (!strcmp(name, "summary-log"))
		cgit_summary_log = atoi(value);
	else if (!strcmp(name, "summary-branches"))
		cgit_summary_branches = atoi(value);
	else if (!strcmp(name, "summary-tags"))
		cgit_summary_tags = atoi(value);
	else if (!strcmp(name, "agefile"))
		cgit_agefile = xstrdup(value);
	else if (!strcmp(name, "renamelimit"))
		cgit_renamelimit = atoi(value);
	else if (!strcmp(name, "robots"))
		cgit_robots = xstrdup(value);
	else if (!strcmp(name, "clone-prefix"))
		cgit_clone_prefix = xstrdup(value);
	else if (!strcmp(name, "repo.group"))
		cgit_repo_group = xstrdup(value);
	else if (!strcmp(name, "repo.url"))
		cgit_repo = add_repo(value);
	else if (!strcmp(name, "repo.name"))
		cgit_repo->name = xstrdup(value);
	else if (cgit_repo && !strcmp(name, "repo.path"))
		cgit_repo->path = trim_end(value, '/');
	else if (cgit_repo && !strcmp(name, "repo.clone-url"))
		cgit_repo->clone_url = xstrdup(value);
	else if (cgit_repo && !strcmp(name, "repo.desc"))
		cgit_repo->desc = xstrdup(value);
	else if (cgit_repo && !strcmp(name, "repo.owner"))
		cgit_repo->owner = xstrdup(value);
	else if (cgit_repo && !strcmp(name, "repo.defbranch"))
		cgit_repo->defbranch = xstrdup(value);
	else if (cgit_repo && !strcmp(name, "repo.snapshots"))
		cgit_repo->snapshots = cgit_snapshots & cgit_parse_snapshots_mask(value); /* XXX: &? */
	else if (cgit_repo && !strcmp(name, "repo.enable-log-filecount"))
		cgit_repo->enable_log_filecount = cgit_enable_log_filecount * atoi(value);
	else if (cgit_repo && !strcmp(name, "repo.enable-log-linecount"))
		cgit_repo->enable_log_linecount = cgit_enable_log_linecount * atoi(value);
	else if (cgit_repo && !strcmp(name, "repo.module-link"))
		cgit_repo->module_link= xstrdup(value);
	else if (cgit_repo && !strcmp(name, "repo.readme") && value != NULL) {
		if (*value == '/')
			cgit_repo->readme = xstrdup(value);
		else
			cgit_repo->readme = xstrdup(fmt("%s/%s", cgit_repo->path, value));
	} else if (!strcmp(name, "include"))
		cgit_read_config(value, cgit_global_config_cb);
}

void cgit_querystring_cb(const char *name, const char *value)
{
	if (!strcmp(name,"r")) {
		cgit_query_repo = xstrdup(value);
		cgit_repo = cgit_get_repoinfo(value);
	} else if (!strcmp(name, "p")) {
		cgit_query_page = xstrdup(value);
		cgit_cmd = cgit_get_cmd_index(value);
	} else if (!strcmp(name, "url")) {
		cgit_parse_url(value);
	} else if (!strcmp(name, "qt")) {
		cgit_query_grep = xstrdup(value);
	} else if (!strcmp(name, "q")) {
		cgit_query_search = xstrdup(value);
	} else if (!strcmp(name, "h")) {
		cgit_query_head = xstrdup(value);
		cgit_query_has_symref = 1;
	} else if (!strcmp(name, "id")) {
		cgit_query_sha1 = xstrdup(value);
		cgit_query_has_sha1 = 1;
	} else if (!strcmp(name, "id2")) {
		cgit_query_sha2 = xstrdup(value);
		cgit_query_has_sha1 = 1;
	} else if (!strcmp(name, "ofs")) {
		cgit_query_ofs = atoi(value);
	} else if (!strcmp(name, "path")) {
		cgit_query_path = trim_end(value, '/');
	} else if (!strcmp(name, "name")) {
		cgit_query_name = xstrdup(value);
	}
}

void *cgit_free_commitinfo(struct commitinfo *info)
{
	free(info->author);
	free(info->author_email);
	free(info->committer);
	free(info->committer_email);
	free(info->subject);
	free(info->msg);
	free(info->msg_encoding);
	free(info);
	return NULL;
}

int hextoint(char c)
{
	if (c >= 'a' && c <= 'f')
		return 10 + c - 'a';
	else if (c >= 'A' && c <= 'F')
		return 10 + c - 'A';
	else if (c >= '0' && c <= '9')
		return c - '0';
	else
		return -1;
}

char *trim_end(const char *str, char c)
{
	int len;
	char *s, *t;

	if (str == NULL)
		return NULL;
	t = (char *)str;
	len = strlen(t);
	while(len > 0 && t[len - 1] == c)
		len--;

	if (len == 0)
		return NULL;

	c = t[len];
	t[len] = '\0';
	s = xstrdup(t);
	t[len] = c;
	return s;
}

char *strlpart(char *txt, int maxlen)
{
	char *result;

	if (!txt)
		return txt;

	if (strlen(txt) <= maxlen)
		return txt;
	result = xmalloc(maxlen + 1);
	memcpy(result, txt, maxlen - 3);
	result[maxlen-1] = result[maxlen-2] = result[maxlen-3] = '.';
	result[maxlen] = '\0';
	return result;
}

char *strrpart(char *txt, int maxlen)
{
	char *result;

	if (!txt)
		return txt;

	if (strlen(txt) <= maxlen)
		return txt;
	result = xmalloc(maxlen + 1);
	memcpy(result + 3, txt + strlen(txt) - maxlen + 4, maxlen - 3);
	result[0] = result[1] = result[2] = '.';
	return result;
}

void cgit_add_ref(struct reflist *list, struct refinfo *ref)
{
	size_t size;

	if (list->count >= list->alloc) {
		list->alloc += (list->alloc ? list->alloc : 4);
		size = list->alloc * sizeof(struct refinfo *);
		list->refs = xrealloc(list->refs, size);
	}
	list->refs[list->count++] = ref;
}

struct refinfo *cgit_mk_refinfo(const char *refname, const unsigned char *sha1)
{
	struct refinfo *ref;

	ref = xmalloc(sizeof (struct refinfo));
	ref->refname = xstrdup(refname);
	ref->object = parse_object(sha1);
	switch (ref->object->type) {
	case OBJ_TAG:
		ref->tag = cgit_parse_tag((struct tag *)ref->object);
		break;
	case OBJ_COMMIT:
		ref->commit = cgit_parse_commit((struct commit *)ref->object);
		break;
	}
	return ref;
}

int cgit_refs_cb(const char *refname, const unsigned char *sha1, int flags,
		  void *cb_data)
{
	struct reflist *list = (struct reflist *)cb_data;
	struct refinfo *info = cgit_mk_refinfo(refname, sha1);

	if (info)
		cgit_add_ref(list, info);
	return 0;
}

void cgit_diff_tree_cb(struct diff_queue_struct *q,
		       struct diff_options *options, void *data)
{
	int i;

	for (i = 0; i < q->nr; i++) {
		if (q->queue[i]->status == 'U')
			continue;
		((filepair_fn)data)(q->queue[i]);
	}
}

static int load_mmfile(mmfile_t *file, const unsigned char *sha1)
{
	enum object_type type;

	if (is_null_sha1(sha1)) {
		file->ptr = (char *)"";
		file->size = 0;
	} else {
		file->ptr = read_sha1_file(sha1, &type, 
		                           (unsigned long *)&file->size);
	}
	return 1;
}

/*
 * Receive diff-buffers from xdiff and concatenate them as
 * needed across multiple callbacks.
 *
 * This is basically a copy of xdiff-interface.c/xdiff_outf(),
 * ripped from git and modified to use globals instead of
 * a special callback-struct.
 */
char *diffbuf = NULL;
int buflen = 0;

int filediff_cb(void *priv, mmbuffer_t *mb, int nbuf)
{
	int i;

	for (i = 0; i < nbuf; i++) {
		if (mb[i].ptr[mb[i].size-1] != '\n') {
			/* Incomplete line */
			diffbuf = xrealloc(diffbuf, buflen + mb[i].size);
			memcpy(diffbuf + buflen, mb[i].ptr, mb[i].size);
			buflen += mb[i].size;
			continue;
		}

		/* we have a complete line */
		if (!diffbuf) {
			((linediff_fn)priv)(mb[i].ptr, mb[i].size);
			continue;
		}
		diffbuf = xrealloc(diffbuf, buflen + mb[i].size);
		memcpy(diffbuf + buflen, mb[i].ptr, mb[i].size);
		((linediff_fn)priv)(diffbuf, buflen + mb[i].size);
		free(diffbuf);
		diffbuf = NULL;
		buflen = 0;
	}
	if (diffbuf) {
		((linediff_fn)priv)(diffbuf, buflen);
		free(diffbuf);
		diffbuf = NULL;
		buflen = 0;
	}
	return 0;
}

int cgit_diff_files(const unsigned char *old_sha1,
		     const unsigned char *new_sha1,
		     linediff_fn fn)
{
	mmfile_t file1, file2;
	xpparam_t diff_params;
	xdemitconf_t emit_params;
	xdemitcb_t emit_cb;

	if (!load_mmfile(&file1, old_sha1) || !load_mmfile(&file2, new_sha1))
		return 1;

	diff_params.flags = XDF_NEED_MINIMAL;
	emit_params.ctxlen = 3;
	emit_params.flags = XDL_EMIT_FUNCNAMES;
	emit_params.find_func = NULL;
	emit_cb.outf = filediff_cb;
	emit_cb.priv = fn;
	xdl_diff(&file1, &file2, &diff_params, &emit_params, &emit_cb);
	return 0;
}

void cgit_diff_tree(const unsigned char *old_sha1,
		    const unsigned char *new_sha1,
		    filepair_fn fn, const char *prefix)
{
	struct diff_options opt;
	int ret;
	int prefixlen;

	diff_setup(&opt);
	opt.output_format = DIFF_FORMAT_CALLBACK;
	opt.detect_rename = 1;
	opt.rename_limit = cgit_renamelimit;
	DIFF_OPT_SET(&opt, RECURSIVE);
	opt.format_callback = cgit_diff_tree_cb;
	opt.format_callback_data = fn;
	if (prefix) {
		opt.nr_paths = 1;
		opt.paths = &prefix;
		prefixlen = strlen(prefix);
		opt.pathlens = &prefixlen;
	}
	diff_setup_done(&opt);

	if (old_sha1 && !is_null_sha1(old_sha1))
		ret = diff_tree_sha1(old_sha1, new_sha1, "", &opt);
	else
		ret = diff_root_tree_sha1(new_sha1, "", &opt);
	diffcore_std(&opt);
	diff_flush(&opt);
}

void cgit_diff_commit(struct commit *commit, filepair_fn fn)
{
	unsigned char *old_sha1 = NULL;

	if (commit->parents)
		old_sha1 = commit->parents->item->object.sha1;
	cgit_diff_tree(old_sha1, commit->object.sha1, fn, NULL);
}
