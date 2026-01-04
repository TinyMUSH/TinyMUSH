/**
 * @file netcommon.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Shared networking helpers for telnet negotiation, buffers, and descriptor bookkeeping
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 * @note This file contains routines used by the networking code that do not
 * depend on the implementation of the networking code.  The network-specific
 * portions of the descriptor data structure are not used.
 */

#include "config.h"

#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"
#include "ansi.h"

#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <arpa/inet.h>

/* ---------------------------------------------------------------------------
 * timeval_sub: return difference between two times as a timeval
 */

struct timeval timeval_sub(struct timeval now, struct timeval then)
{
	now.tv_sec -= then.tv_sec;
	now.tv_usec -= then.tv_usec;

	if (now.tv_usec < 0)
	{
		now.tv_usec += 1000000;
		now.tv_sec--;
	}

	return now;
}

/* ---------------------------------------------------------------------------
 * msec_diff: return difference between two times in milliseconds
 */

int msec_diff(struct timeval now, struct timeval then)
{
	return ((now.tv_sec - then.tv_sec) * 1000 + (now.tv_usec - then.tv_usec) / 1000);
}

/* ---------------------------------------------------------------------------
 * msec_add: add milliseconds to a timeval
 */

struct timeval msec_add(struct timeval t, int x)
{
	t.tv_sec += x / 1000;
	t.tv_usec += (x % 1000) * 1000;

	if (t.tv_usec >= 1000000)
	{
		t.tv_sec += t.tv_usec / 1000000;
		t.tv_usec = t.tv_usec % 1000000;
	}

	return t;
}

/* ---------------------------------------------------------------------------
 * update_quotas: Update timeslice quotas
 */

struct timeval update_quotas(struct timeval last, struct timeval current)
{
	int nslices;
	DESC *d, *dnext;
	nslices = msec_diff(current, last) / mushconf.timeslice;

	if (nslices > 0)
	{
		for (d = descriptor_list, dnext = ((d != NULL) ? d->next : NULL); d; d = dnext, dnext = ((dnext != NULL) ? dnext->next : NULL))
		{
			d->quota += mushconf.cmd_quota_incr * nslices;

			if (d->quota > mushconf.cmd_quota_max)
			{
				d->quota = mushconf.cmd_quota_max;
			}
		}
	}

	return msec_add(last, nslices * mushconf.timeslice);
}

/* raw_notify_html() -- raw_notify() without the newline */
void raw_notify_html(dbref player, const char *format, ...)
{
	DESC *d;
	char *msg = XMALLOC(LBUF_SIZE, "msg");
	char *s;
	va_list ap;
	va_start(ap, format);

	if ((!format || !*format))
	{
		if ((s = va_arg(ap, char *)) != NULL)
		{
			XSTRNCPY(msg, s, LBUF_SIZE);
		}
		else
		{
			XFREE(msg);
			return;
		}
	}
	else
	{
		XVSNPRINTF(msg, LBUF_SIZE, format, ap);
	}

	va_end(ap);

	if (mushstate.inpipe && (player == mushstate.poutobj))
	{
		XSAFELBSTR(msg, mushstate.poutnew, &mushstate.poutbufc);
		XFREE(msg);
		return;
	}

	if (!Connected(player))
	{
		XFREE(msg);
		return;
	}

	if (!Html(player))
	{ /* Don't splooge HTML at a non-HTML player. */
		XFREE(msg);
		return;
	}

	for (d = (DESC *)nhashfind((int)player, &mushstate.desc_htab); d; d = d->hashnext)
	{
		queue_string(d, NULL, msg);
	}
	XFREE(msg);
}

/* ---------------------------------------------------------------------------
 * raw_notify: write a message to a player
 */

void raw_notify(dbref player, const char *format, ...)
{
	DESC *d;
	char *msg = XMALLOC(LBUF_SIZE, "msg");
	char *s;
	va_list ap;
	va_start(ap, format);

	if ((!format || !*format))
	{
		if ((s = va_arg(ap, char *)) != NULL)
		{
			XSTRNCPY(msg, s, LBUF_SIZE);
		}
		else
		{
			XFREE(msg);
			return;
		}
	}
	else
	{
		XVSNPRINTF(msg, LBUF_SIZE, format, ap);
	}

	va_end(ap);

	if (mushstate.inpipe && (player == mushstate.poutobj))
	{
		XSAFELBSTR(msg, mushstate.poutnew, &mushstate.poutbufc);
		XSAFECRLF(mushstate.poutnew, &mushstate.poutbufc);
		XFREE(msg);
		return;
	}

	if (!Connected(player))
	{
		XFREE(msg);
		return;
	}

	for (d = (DESC *)nhashfind((int)player, &mushstate.desc_htab); d; d = d->hashnext)
	{
		queue_string(d, NULL, msg);
		queue_write(d, "\r\n", 2);
	}
	XFREE(msg);
}

void raw_notify_newline(dbref player)
{
	DESC *d;

	if (mushstate.inpipe && (player == mushstate.poutobj))
	{
		XSAFECRLF(mushstate.poutnew, &mushstate.poutbufc);
		return;
	}

	if (!Connected(player))
	{
		return;
	}

	for (d = (DESC *)nhashfind((int)player, &mushstate.desc_htab); d; d = d->hashnext)
	{
		queue_write(d, "\r\n", 2);
	}
}

/* ---------------------------------------------------------------------------
 * raw_broadcast: Send message to players who have indicated flags
 */

void raw_broadcast(int inflags, char *template, ...)
{
	char *buff = XMALLOC(LBUF_SIZE, "buff");
	DESC *d;
	int test_flag, which_flag, p_flag;
	va_list ap;
	va_start(ap, template);

	if (!template || !*template)
	{
		XFREE(buff);
		return;
	}

	XVSNPRINTF(buff, LBUF_SIZE, template, ap);
	/*
	 * Note that this use of the flagwords precludes testing for
	 * * type in this function. (Not that this matters, since we
	 * * look at connected descriptors, which must be players.)
	 */
	test_flag = inflags & ~(FLAG_WORD2 | FLAG_WORD3);

	if (inflags & FLAG_WORD2)
	{
		which_flag = 2;
	}
	else if (inflags & FLAG_WORD3)
	{
		which_flag = 3;
	}
	else
	{
		which_flag = 1;
	}

	DESC *dnext;
	for (d = descriptor_list, dnext = ((d != NULL) ? d->next : NULL); d; d = dnext, dnext = ((dnext != NULL) ? dnext->next : NULL))
		if (d->flags & DS_CONNECTED)
		{
			switch (which_flag)
			{
			case 1:
				p_flag = Flags(d->player);
				break;

			case 2:
				p_flag = Flags2(d->player);
				break;

			case 3:
				p_flag = Flags3(d->player);
				break;

			default:
				p_flag = Flags(d->player);
			}

			/*
			 * If inflags is 0, broadcast to everyone
			 */

			if ((p_flag & test_flag) || (!test_flag))
			{
				queue_string(d, NULL, buff);
				queue_write(d, "\r\n", 2);
				process_output(d);
			}
		}
	va_end(ap);
	XFREE(buff);
}

/* ---------------------------------------------------------------------------
 * clearstrings: clear out prefix and suffix strings
 */

void clearstrings(DESC *d)
{
	if (!d)
	{
		return;
	}

	if (d->output_prefix)
	{
		XFREE(d->output_prefix);
		d->output_prefix = NULL;
	}

	if (d->output_suffix)
	{
		XFREE(d->output_suffix);
		d->output_suffix = NULL;
	}
}

/* ---------------------------------------------------------------------------
 * queue_write: Add text to the output queue for the indicated descriptor.
 */

void queue_write(DESC *d, const char *b, int n)
{
	int left;
	char *name;
	TBLOCK *tp;

	if (n <= 0)
	{
		return;
	}

	if (d->output_size + n > mushconf.output_limit)
	{
		process_output(d);
	}

	left = mushconf.output_limit - d->output_size - n;

	if (left < 0)
	{
		tp = d->output_head;

		if (tp == NULL)
		{
			log_write(LOG_PROBLEMS, "QUE", "WRITE", "Flushing when output_head is null!");
		}
		else
		{
			name = log_getname(d->player);
			log_write(LOG_NET, "NET", "WRITE", "[%d/%s] Output buffer overflow, %d chars discarded by %s", d->descriptor, d->addr, tp->hdr.nchars, name);
			XFREE(name);
			d->output_size -= tp->hdr.nchars;
			d->output_head = tp->hdr.nxt;
			d->output_lost += tp->hdr.nchars;

			if (d->output_head == NULL)
			{
				d->output_tail = NULL;
			}

			XFREE(tp->data);
			XFREE(tp);
		}
	}

	/*
	 * Allocate an output buffer if needed
	 */

	if (d->output_head == NULL)
	{
		tp = (TBLOCK *)XMALLOC(sizeof(TBLOCK), "tp");
		tp->data = XMALLOC(mushconf.output_block_size - sizeof(TBLKHDR), "tp->data");
		tp->hdr.nxt = NULL;
		tp->hdr.start = tp->data;
		tp->hdr.end = tp->data;
		tp->hdr.nchars = 0;
		d->output_head = tp;
		d->output_tail = tp;
	}
	else
	{
		tp = d->output_tail;
	}

	/*
	 * Now tp points to the last buffer in the chain
	 */
	d->output_size += n;
	d->output_tot += n;

	do
	{
		/*
		 * See if there is enough space in the buffer to hold the
		 * * string.  If so, copy it and update the pointers..
		 */
		left = mushconf.output_block_size - (tp->hdr.end - (char *)tp + 1);

		if (n <= left)
		{
			XSTRNCPY(tp->hdr.end, b, n);
			tp->hdr.end += n;
			tp->hdr.nchars += n;
			n = 0;
		}
		else
		{
			/*
			 * It didn't fit.  Copy what will fit and then
			 * * allocate * another buffer and retry.
			 */
			if (left > 0)
			{
				XSTRNCPY(tp->hdr.end, b, left);
				tp->hdr.end += left;
				tp->hdr.nchars += left;
				b += left;
				n -= left;
			}

			tp = (TBLOCK *)XMALLOC(sizeof(TBLOCK), "tp");
			tp->data = XMALLOC(mushconf.output_block_size - sizeof(TBLKHDR), "tp->data");
			tp->hdr.nxt = NULL;
			tp->hdr.start = tp->data;
			tp->hdr.end = tp->data;
			tp->hdr.nchars = 0;
			d->output_tail->hdr.nxt = tp;
			d->output_tail = tp;
		}
	} while (n > 0);
}

/**
 * Callback helper for level_ansi_stream - writes chunks directly to queue
 */
static void queue_write_callback(const char *data, size_t len, void *context)
{
	DESC *d = (DESC *)context;
	queue_write(d, data, len);
}

typedef struct PostprocessStreamContext
{
	bool apply_nobleed;
	bool apply_colormap;
	int *cmap;

	char seq_buf[128];
	size_t seq_len;

	char out_buf[8192];
	char *out_ptr;
	char *out_end;
	size_t flush_threshold;

	void (*flush_fn)(const char *data, size_t len, void *ctx);
	void *flush_ctx;
} PostprocessStreamContext;

static inline int postprocess_fg_sgr_from_state(const ColorState *state)
{
	if (!state || state->foreground.is_set != ColorStatusSet)
	{
		return -1;
	}

	int idx = state->foreground.ansi_index;
	if (idx >= 0 && idx < 8)
	{
		return 30 + idx;
	}

	if (idx >= 8 && idx < 16)
	{
		return 90 + (idx - 8);
	}

	return -1;
}

static inline int postprocess_bg_sgr_from_state(const ColorState *state)
{
	if (!state || state->background.is_set != ColorStatusSet)
	{
		return -1;
	}

	int idx = state->background.ansi_index;
	if (idx >= 0 && idx < 8)
	{
		return 40 + idx;
	}

	if (idx >= 8 && idx < 16)
	{
		return 100 + (idx - 8);
	}

	return -1;
}

static inline void postprocess_apply_sgr_to_state(ColorState *state, int sgr)
{
	if (!state)
	{
		return;
	}

	if (sgr >= 30 && sgr <= 37)
	{
		int idx = sgr - 30;
		state->foreground.is_set = ColorStatusSet;
		state->foreground.ansi_index = idx;
		state->foreground.xterm_index = idx;
	}
	else if (sgr >= 40 && sgr <= 47)
	{
		int idx = sgr - 40;
		state->background.is_set = ColorStatusSet;
		state->background.ansi_index = idx;
		state->background.xterm_index = idx;
	}
}

static inline void postprocess_flush(PostprocessStreamContext *ctx)
{
	size_t used = (size_t)(ctx->out_ptr - ctx->out_buf);
	if (used > 0)
	{
		ctx->flush_fn(ctx->out_buf, used, ctx->flush_ctx);
		ctx->out_ptr = ctx->out_buf;
	}
}

static inline void postprocess_emit_block(PostprocessStreamContext *ctx, const char *data, size_t len)
{
	while (len > 0)
	{
		size_t space = (size_t)(ctx->out_end - ctx->out_ptr);
		if (space == 0)
		{
			postprocess_flush(ctx);
			space = (size_t)(ctx->out_end - ctx->out_ptr);
		}

		size_t copy_len = (len < space) ? len : space;
		XMEMCPY(ctx->out_ptr, data, copy_len);
		ctx->out_ptr += copy_len;
		data += copy_len;
		len -= copy_len;

		if ((size_t)(ctx->out_ptr - ctx->out_buf) >= ctx->flush_threshold)
		{
			postprocess_flush(ctx);
		}
	}
}

static void postprocess_emit_sequence(PostprocessStreamContext *ctx)
{
	ctx->seq_buf[ctx->seq_len] = '\0';
	const char *seq_ptr = ctx->seq_buf;
	const char *seq_start = seq_ptr;
	ColorState state = ansi_parse_sequence(&seq_ptr);

	if (seq_ptr == seq_start)
	{
		postprocess_emit_block(ctx, ctx->seq_buf, ctx->seq_len);
		ctx->seq_len = 0;
		return;
	}

	ColorState final_state = state;

	if (ctx->apply_nobleed && state.reset == ColorStatusReset)
	{
		ColorState white_state = {0};
		white_state.foreground.is_set = ColorStatusSet;
		white_state.foreground.ansi_index = 7;
		white_state.foreground.xterm_index = 7;
		white_state.foreground.truecolor = (ColorRGB){255, 255, 255};

		white_state.background = state.background;
		white_state.highlight = state.highlight;
		white_state.underline = state.underline;
		white_state.flash = state.flash;
		white_state.inverse = state.inverse;

		final_state = white_state;
	}

	if (ctx->apply_colormap)
	{
		int n = postprocess_fg_sgr_from_state(&final_state);
		if (n >= I_ANSI_BLACK && n < I_ANSI_NUM && ctx->cmap[n - I_ANSI_BLACK] != 0)
		{
			postprocess_apply_sgr_to_state(&final_state, ctx->cmap[n - I_ANSI_BLACK]);
		}

		n = postprocess_bg_sgr_from_state(&final_state);
		if (n >= I_ANSI_BLACK && n < I_ANSI_NUM && ctx->cmap[n - I_ANSI_BLACK] != 0)
		{
			postprocess_apply_sgr_to_state(&final_state, ctx->cmap[n - I_ANSI_BLACK]);
		}
	}

	char seq_out[128];
	size_t offset = 0;
	ColorStatus status = to_ansi_escape_sequence(seq_out, sizeof(seq_out), &offset, &final_state, ColorTypeAnsi);
	if (status == ColorStatusNone || offset == 0)
	{
		postprocess_emit_block(ctx, ctx->seq_buf, ctx->seq_len);
	}
	else
	{
		postprocess_emit_block(ctx, seq_out, offset);
	}

	ctx->seq_len = 0;
}

static void postprocess_stream_write(const char *data, size_t len, void *context)
{
	PostprocessStreamContext *ctx = (PostprocessStreamContext *)context;

	for (size_t i = 0; i < len; i++)
	{
		char ch = data[i];

		if (ctx->seq_len > 0 || ch == ESC_CHAR)
		{
			if (ctx->seq_len < sizeof(ctx->seq_buf) - 1)
			{
				ctx->seq_buf[ctx->seq_len++] = ch;
			}

			if (ch == 'm')
			{
				postprocess_emit_sequence(ctx);
			}
			else if (ctx->seq_len >= sizeof(ctx->seq_buf) - 1)
			{
				postprocess_emit_block(ctx, ctx->seq_buf, ctx->seq_len);
				ctx->seq_len = 0;
			}

			continue;
		}

		postprocess_emit_block(ctx, &ch, 1);
	}
}

static void postprocess_stream_finish(PostprocessStreamContext *ctx)
{
	if (ctx->seq_len > 0)
	{
		postprocess_emit_block(ctx, ctx->seq_buf, ctx->seq_len);
		ctx->seq_len = 0;
	}

	postprocess_flush(ctx);
}

void queue_string(DESC *d, const char *format, ...)
{
	va_list ap;
	char *s = NULL, *msg = XMALLOC(LBUF_SIZE, "msg");

	va_start(ap, format);

	if ((!format || !*format))
	{
		if ((s = va_arg(ap, char *)) != NULL)
		{
			XSTRNCPY(msg, s, LBUF_SIZE);
		}
		else
		{
			XFREE(msg);
			return;
		}
	}
	else
	{
		XVSNPRINTF(msg, LBUF_SIZE, format, ap);
	}

	va_end(ap);

	if (msg)
	{
		if (!mushconf.ansi_colors)
		{
			queue_write(d, msg, strlen(msg));
		}
		else
		{
			bool apply_nobleed = NoBleed(d->player) && (Ansi(d->player) || Color256(d->player) || Color24Bit(d->player));
			bool apply_colormap = (d->colormap != NULL);
			bool needs_postprocessing = apply_nobleed || apply_colormap;

			if (!needs_postprocessing)
			{
				level_ansi_stream(msg, Ansi(d->player), Color256(d->player), Color24Bit(d->player), queue_write_callback, d);
			}
			else
			{
				PostprocessStreamContext ctx = {
					.apply_nobleed = apply_nobleed,
					.apply_colormap = apply_colormap,
					.cmap = d->colormap,
					.seq_len = 0,
					.out_ptr = NULL,
					.out_end = NULL,
					.flush_threshold = 0,
					.flush_fn = queue_write_callback,
					.flush_ctx = d};

				ctx.out_ptr = ctx.out_buf;
				ctx.out_end = ctx.out_buf + sizeof(ctx.out_buf);
				ctx.flush_threshold = sizeof(ctx.out_buf) * 80 / 100;

				level_ansi_stream(msg, Ansi(d->player), Color256(d->player), Color24Bit(d->player), postprocess_stream_write, &ctx);
				postprocess_stream_finish(&ctx);
			}
		}
	}

	XFREE(msg);
}

void queue_rawstring(DESC *d, const char *format, ...)
{
	char *msg = XMALLOC(LBUF_SIZE, "msg");
	char *s;
	va_list ap;
	va_start(ap, format);

	if ((!format || !*format))
	{
		if ((s = va_arg(ap, char *)) != NULL)
		{
			XSTRNCPY(msg, s, LBUF_SIZE);
		}
		else
		{
			XFREE(msg);
			return;
		}
	}
	else
	{
		XVSNPRINTF(msg, LBUF_SIZE, format, ap);
	}

	va_end(ap);
	queue_write(d, msg, strlen(msg));
	XFREE(msg);
}

void freeqs(DESC *d)
{
	TBLOCK *tb, *tnext;
	CBLK *cb, *cnext;

	if (!d)
	{
		return;
	}

	tb = d->output_head;

	while (tb)
	{
		tnext = tb->hdr.nxt;
		if (tb->data)
		{
			XFREE(tb->data);
		}
		XFREE(tb);
		tb = tnext;
	}

	d->output_head = NULL;
	d->output_tail = NULL;
	cb = d->input_head;

	while (cb)
	{
		cnext = (CBLK *)cb->hdr.nxt;
		XFREE(cb);
		cb = cnext;
	}

	d->input_head = NULL;
	d->input_tail = NULL;

	if (d->raw_input)
	{
		XFREE(d->raw_input);
	}

	d->raw_input = NULL;
	d->raw_input_at = NULL;
}

/* ---------------------------------------------------------------------------
 * desc_addhash: Add a net descriptor to its player hash list.
 */

void desc_addhash(DESC *d)
{
	dbref player;
	DESC *hdesc;
	player = d->player;
	hdesc = (DESC *)nhashfind((int)player, &mushstate.desc_htab);

	if (hdesc == NULL)
	{
		d->hashnext = NULL;
		nhashadd((int)player, (int *)d, &mushstate.desc_htab);
	}
	else
	{
		d->hashnext = hdesc;
		nhashrepl((int)player, (int *)d, &mushstate.desc_htab);
	}
}

/* ---------------------------------------------------------------------------
 * desc_delhash: Remove a net descriptor from its player hash list.
 */

void desc_delhash(DESC *d)
{
	DESC *hdesc, *last;
	dbref player;
	player = d->player;
	last = NULL;
	hdesc = (DESC *)nhashfind((int)player, &mushstate.desc_htab);

	while (hdesc != NULL)
	{
		if (d == hdesc)
		{
			if (last == NULL)
			{
				if (d->hashnext == NULL)
				{
					nhashdelete((int)player, &mushstate.desc_htab);
				}
				else
				{
					nhashrepl((int)player, (int *)d->hashnext, &mushstate.desc_htab);
				}
			}
			else
			{
				last->hashnext = d->hashnext;
			}

			break;
		}

		last = hdesc;
		hdesc = hdesc->hashnext;
	}

	d->hashnext = NULL;
}

void welcome_user(DESC *d)
{
	/*
	 * Send TELNET options to inform client about server capabilities
	 * This helps clients like TinyFugue properly handle ANSI codes without
	 * requiring manual emulation mode changes.
	 * 
	 * Standard MUD telnet negotiation sequence:
	 * - WILL SUPPRESS_GO_AHEAD: Server won't send GA after each line
	 * - WONT ECHO: Client handles local echo (standard MUD behavior)
	 * 
	 * This combination triggers automatic ANSI mode in TinyFugue and similar clients.
	 */
	unsigned char telnet_opts[3];
	
	/* Send WILL SUPPRESS_GO_AHEAD (option 3) */
	telnet_opts[0] = 0xFF;  /* IAC */
	telnet_opts[1] = 0xFB;  /* WILL */
	telnet_opts[2] = 0x03;  /* SUPPRESS_GO_AHEAD */
	queue_write(d, (char *)telnet_opts, 3);
	
	/* Send WONT ECHO (option 1) - client does local echo (MUD standard) */
	telnet_opts[0] = 0xFF;  /* IAC */
	telnet_opts[1] = 0xFC;  /* WONT */
	telnet_opts[2] = 0x01;  /* ECHO */
	queue_write(d, (char *)telnet_opts, 3);
	
	if (mushconf.have_pueblo == 1)
	{
		queue_rawstring(d, NULL, mushconf.pueblo_version);
		queue_rawstring(d, NULL, "\r\n\r\n");
	}

	if (d->host_info & H_REGISTRATION)
	{
		fcache_dump(d, FC_CONN_REG);
	}
	else
	{
		fcache_dump(d, FC_CONN);
	}
}

void save_command(DESC *d, CBLK *command)
{
	command->hdr.nxt = NULL;

	if (d->input_tail == NULL)
	{
		d->input_head = command;
	}
	else
	{
		d->input_tail->hdr.nxt = command;
	}

	d->input_tail = command;
}

void set_userstring(char **userstring, const char *command)
{
	while (*command && isascii(*command) && isspace(*command))
	{
		command++;
	}

	if (!*command)
	{
		if (*userstring != NULL)
		{
			XFREE(*userstring);
			*userstring = NULL;
		}
	}
	else
	{
		if (*userstring == NULL)
		{
			*userstring = XMALLOC(LBUF_SIZE, "userstring");
		}

		XSTRCPY(*userstring, command);
	}
}

void parse_connect(const char *msg, char *command, char *user, char *pass)
{
	char *p;

	if (strlen(msg) > MBUF_SIZE)
	{
		*command = '\0';
		*user = '\0';
		*pass = '\0';
		return;
	}

	while (*msg && isascii(*msg) && isspace(*msg))
	{
		msg++;
	}

	p = command;

	while (*msg && isascii(*msg) && !isspace(*msg))
	{
		*p++ = *msg++;
	}

	*p = '\0';

	while (*msg && isascii(*msg) && isspace(*msg))
	{
		msg++;
	}

	p = user;
	char *user_end = user + LBUF_SIZE - 1;

	if (mushconf.name_spaces && (*msg == '\"'))
	{
		for (; *msg && (*msg == '\"' || isspace(*msg)); msg++)
			;

		while (*msg && *msg != '\"' && p < user_end)
		{
			while (*msg && !isspace(*msg) && (*msg != '\"') && p < user_end)
			{
				*p++ = *msg++;
			}

			if (*msg == '\"')
			{
				break;
			}

			while (*msg && isspace(*msg))
			{
				msg++;
			}

			if (*msg && (*msg != '\"') && p < user_end)
			{
				*p++ = ' ';
			}
		}

		for (; *msg && *msg == '\"'; msg++)
			;
	}
	else
		while (*msg && isascii(*msg) && !isspace(*msg) && p < user_end)
		{
			*p++ = *msg++;
		}

	*p = '\0';

	while (*msg && isascii(*msg) && isspace(*msg))
	{
		msg++;
	}

	p = pass;
	char *pass_end = pass + LBUF_SIZE - 1;

	while (*msg && isascii(*msg) && !isspace(*msg) && p < pass_end)
	{
		*p++ = *msg++;
	}

	*p = '\0';
}

char *time_format_1(time_t dt)
{
	// Removed: register struct tm *delta;
	char *buf = XMALLOC(MBUF_SIZE, "buf");

	if (dt < 0)
	{
		dt = 0;
	}

	struct tm delta;
	gmtime_r(&dt, &delta);

	if (delta.tm_yday > 0)
	{
		XSPRINTF(buf, "%dd %02d:%02d", delta.tm_yday, delta.tm_hour, delta.tm_min);
	}
	else
	{
		XSPRINTF(buf, "%02d:%02d", delta.tm_hour, delta.tm_min);
	}

	return buf;
}

char *time_format_2(time_t dt)
{
	char *buf = XMALLOC(MBUF_SIZE, "buf");

	if (dt < 0)
	{
		dt = 0;
	}

	struct tm delta;
	gmtime_r(&dt, &delta);

	if (delta.tm_yday > 0)
	{
		XSPRINTF(buf, "%dd", delta.tm_yday);
	}
	else if (delta.tm_hour > 0)
	{
		XSPRINTF(buf, "%dh", delta.tm_hour);
	}
	else if (delta.tm_min > 0)
	{
		XSPRINTF(buf, "%dm", delta.tm_min);
	}
	else
	{
		XSPRINTF(buf, "%ds", delta.tm_sec);
	}

	return buf;
}

void announce_connattr(DESC *d, dbref player, dbref loc, const char *reason, int num, int attr)
{
	dbref aowner, obj, zone;
	int aflags, alen, argn;
	char *buf;
	char *argv[7];
	/*
	 * Pass session information on the stack:
	 * *   %0 - reason message
	 * *   %1 - current number of connections
	 * *   %2 - connect time
	 * *   %3 - last input
	 * *   %4 - number of commands entered
	 * *   %5 - bytes input
	 * *   %6 - bytes output
	 */
	argv[0] = (char *)reason;
	argv[1] = XMALLOC(SBUF_SIZE, "argv[1]");
	XSPRINTF(argv[1], "%d", num);

	if (attr == A_ADISCONNECT)
	{
		argn = 7;
		argv[2] = XMALLOC(SBUF_SIZE, "argv[2]");
		argv[3] = XMALLOC(SBUF_SIZE, "argv[3]");
		argv[4] = XMALLOC(SBUF_SIZE, "argv[4]");
		argv[5] = XMALLOC(SBUF_SIZE, "argv[5]");
		argv[6] = XMALLOC(SBUF_SIZE, "argv[6]");
		XSPRINTF(argv[2], "%ld", (long)d->connected_at);
		XSPRINTF(argv[3], "%ld", (long)d->last_time);
		XSPRINTF(argv[4], "%d", d->command_count);
		XSPRINTF(argv[5], "%d", d->input_tot);
		XSPRINTF(argv[6], "%d", d->output_tot);
	}
	else
	{
		argn = 2;
	}

	buf = atr_pget(player, attr, &aowner, &aflags, &alen);

	if (*buf)
	{
		wait_que(player, player, 0, NOTHING, 0, buf, argv, argn, NULL);
	}

	XFREE(buf);

	if (Good_obj(mushconf.master_room) && mushconf.use_global_aconn)
	{
		buf = atr_pget(mushconf.master_room, attr, &aowner, &aflags, &alen);

		if (*buf)
			wait_que(mushconf.master_room, player, 0, NOTHING, 0, buf, argv, argn, NULL);

		XFREE(buf);
		dbref master_contents = Contents(mushconf.master_room);
		if (!Good_obj(master_contents))
		{
			master_contents = NOTHING;
		}
		for (obj = master_contents; (obj != NOTHING) && (Next(obj) != obj); obj = Next(obj))
		{
			if (!mushconf.global_aconn_uselocks || could_doit(player, obj, A_LUSE))
			{
				buf = atr_pget(obj, attr, &aowner, &aflags, &alen);

				if (*buf)
				{
					wait_que(obj, player, 0, NOTHING, 0, buf, argv, argn, NULL);
				}

				XFREE(buf);
			}
		}
	}

	/*
	 * do the zone of the player's location's possible a(dis)connect
	 */
	if (mushconf.have_zones && ((zone = Zone(loc)) != NOTHING) && Good_obj(zone))
	{
		dbref zone_contents = NOTHING;
		switch (Typeof(zone))
		{
		case TYPE_THING:
			buf = atr_pget(zone, attr, &aowner, &aflags, &alen);

			if (*buf)
			{
				wait_que(zone, player, 0, NOTHING, 0, buf, argv, argn, NULL);
			}

			XFREE(buf);
			break;

		case TYPE_ROOM:
			/*
			 * check every object in the room for a (dis)connect
			 * * action
			 */
			zone_contents = Contents(zone);

			if (!Good_obj(zone_contents))
			{
				zone_contents = NOTHING;
			}

			for (obj = zone_contents; (obj != NOTHING) && (Next(obj) != obj); obj = Next(obj))
			{
				buf = atr_pget(obj, attr, &aowner, &aflags, &alen);

				if (*buf)
				{
					wait_que(obj, player, 0, NOTHING, 0, buf, argv, argn, NULL);
				}

				XFREE(buf);
			}
			break;

		default:
			buf = log_getname(player);
			log_write(LOG_PROBLEMS, "OBJ", "DAMAG", "Invalid zone #%d for %s has bad type %d", zone, buf, Typeof(zone));
			XFREE(buf);
		}
	}

	XFREE(argv[1]);

	if (attr == A_ADISCONNECT)
	{
		XFREE(argv[2]);
		XFREE(argv[3]);
		XFREE(argv[4]);
		XFREE(argv[5]);
		XFREE(argv[6]);
	}
}

void announce_connect(dbref player, DESC *d, const char *reason)
{
	dbref loc = NOTHING, aowner = NOTHING, temp = NOTHING;
	int aflags = 0, alen = 0, num = 0, key = 0, count = 0;
	char *buf = NULL, *time_str = NULL;
	DESC *dtemp = NULL, *dtemp_next = NULL;
	desc_addhash(d);

	for (dtemp = descriptor_list, dtemp_next = ((dtemp != NULL) ? dtemp->next : NULL); dtemp; dtemp = dtemp_next, dtemp_next = ((dtemp_next != NULL) ? dtemp_next->next : NULL))
		if (dtemp->flags & DS_CONNECTED)
		{
			count++;
		}

	if (mushstate.record_players < count)
	{
		mushstate.record_players = count;
	}

	buf = atr_pget(player, A_TIMEOUT, &aowner, &aflags, &alen);
	d->timeout = (int)strtol(buf, (char **)NULL, 10);

	if (d->timeout <= 0)
	{
		d->timeout = mushconf.idle_timeout;
	}

	XFREE(buf);
	loc = Location(player);
	s_Connected(player);

	if (mushconf.have_pueblo == 1)
	{
		if (d->flags & DS_PUEBLOCLIENT)
		{
			s_Html(player);
		}
	}

	if (mushconf.motd_msg)
	{
		if (mushconf.ansi_colors)
		{
			raw_notify(player, "\r\n%sMOTD:%s %s\r\n", ANSI_HILITE, ANSI_NORMAL, mushconf.motd_msg);
		}
		else
		{
			raw_notify(player, "\r\nMOTD: %s\r\n", mushconf.motd_msg);
		}
	}

	if (Wizard(player))
	{
		if (mushconf.wizmotd_msg)
		{
			if (mushconf.ansi_colors)
			{
				raw_notify(player, "\r\n%sWIZMOTD:%s %s\r\n", ANSI_HILITE, ANSI_NORMAL, mushconf.wizmotd_msg);
			}
			else
			{
				raw_notify(player, "\r\nWIZMOTD: %s\r\n", mushconf.wizmotd_msg);
			}
		}

		if (!(mushconf.control_flags & CF_LOGIN))
		{
			raw_notify(player, NULL, "*** Logins are disabled.");
		}
	}

	buf = atr_get(player, A_LPAGE, &aowner, &aflags, &alen);

	if (*buf)
	{
		raw_notify(player, NULL, "REMINDER: Your PAGE LOCK is set. You may be unable to receive some pages.");
	}

	if (Dark(player))
	{
		raw_notify(player, NULL, "REMINDER: You are set DARK.");
	}

	num = 0;
	for (dtemp = (DESC *)nhashfind((int)player, &mushstate.desc_htab); dtemp; dtemp = dtemp->hashnext)
	{
		num++;
	}
	/*
	 * Reset vacation flag
	 */
	s_Flags2(player, Flags2(player) & ~VACATION);

	if (num < 2)
	{
		XSPRINTF(buf, "%s has connected.", Name(player));

		if (Hidden(player))
		{
			raw_broadcast(WATCHER | FLAG_WORD2, (char *)"GAME: %s has DARK-connected.", Name(player));
		}
		else
		{
			raw_broadcast(WATCHER | FLAG_WORD2, (char *)"GAME: %s has connected.", Name(player));
		}
	}
	else
	{
		XSPRINTF(buf, "%s has reconnected.", Name(player));
		raw_broadcast(WATCHER | FLAG_WORD2, (char *)"GAME: %s has reconnected.", Name(player));
	}

	key = MSG_INV;

	if ((loc != NOTHING) && !(Hidden(player) && Can_Hide(player)))
	{
		key |= (MSG_NBR | MSG_NBR_EXITS | MSG_LOC | MSG_FWDLIST);
	}

	temp = mushstate.curr_enactor;
	mushstate.curr_enactor = player;
	notify_check(player, player, key, NULL, buf);
	XFREE(buf);

	for (MODULE *cam__mp = mushstate.modules_list; cam__mp != NULL; cam__mp = cam__mp->next)
	{
		if (cam__mp->announce_connect)
		{
			(*(cam__mp->announce_connect))(player, reason, num);
		}
	}

	if (Suspect(player))
	{
		raw_broadcast(WIZARD, (char *)"[Suspect] %s has connected.", Name(player));
	}

	if (d->host_info & H_SUSPECT)
	{
		raw_broadcast(WIZARD, (char *)"[Suspect site: %s] %s has connected.", d->addr, Name(player));
	}

	announce_connattr(d, player, loc, reason, num, A_ACONNECT);
	char conn_time_buf[26];
	struct tm conn_time_tm;
	localtime_r(&mushstate.now, &conn_time_tm);
	strftime(conn_time_buf, sizeof(conn_time_buf), "%a %b %d %H:%M:%S %Y", &conn_time_tm);
	record_login(player, 1, conn_time_buf, d->addr, d->username);

	dbref player_loc = Location(player);
	if (!Good_obj(player_loc))
	{
		player_loc = mushconf.start_room;
	}

	if (mushconf.have_pueblo == 1)
	{
		look_in(player, player_loc, (LK_SHOWEXIT | LK_OBEYTERSE | LK_SHOWVRML));
	}
	else
	{
		look_in(player, player_loc, (LK_SHOWEXIT | LK_OBEYTERSE));
	}

	mushstate.curr_enactor = temp;
}

void announce_disconnect(dbref player, DESC *d, const char *reason)
{
	dbref loc, temp;
	int num, key;
	char *buf;
	DESC *dtemp;

	if (Suspect(player))
	{
		raw_broadcast(WIZARD, (char *)"[Suspect] %s has disconnected.", Name(player));
	}

	if (d->host_info & H_SUSPECT)
	{
		raw_broadcast(WIZARD, (char *)"[Suspect site: %s] %s has disconnected.", d->addr, Name(d->player));
	}

	loc = Location(player);
	num = -1;
	for (dtemp = (DESC *)nhashfind((int)player, &mushstate.desc_htab); dtemp; dtemp = dtemp->hashnext)
	{
		num++;
	}
	temp = mushstate.curr_enactor;
	mushstate.curr_enactor = player;

	if (num < 1)
	{
		buf = XMALLOC(MBUF_SIZE, "buf");
		XSPRINTF(buf, "%s has disconnected.", Name(player));
		key = MSG_INV;

		if ((loc != NOTHING) && !(Hidden(player) && Can_Hide(player)))
			key |= (MSG_NBR | MSG_NBR_EXITS | MSG_LOC | MSG_FWDLIST);

		notify_check(player, player, key, NULL, buf);
		XFREE(buf);
		raw_broadcast(WATCHER | FLAG_WORD2, (char *)"GAME: %s has disconnected.", Name(player));
		/*
		 * Must reset flags before we do module stuff.
		 */
		c_Connected(player);

		if (mushconf.have_pueblo == 1)
		{
			c_Html(player);
		}
	}
	else
	{
		buf = XMALLOC(MBUF_SIZE, "buf");
		XSPRINTF(buf, "%s has partially disconnected.", Name(player));
		key = MSG_INV;

		if ((loc != NOTHING) && !(Hidden(player) && Can_Hide(player)))
			key |= (MSG_NBR | MSG_NBR_EXITS | MSG_LOC | MSG_FWDLIST);

		notify_check(player, player, key, NULL, buf);
		raw_broadcast(WATCHER | FLAG_WORD2, (char *)"GAME: %s has partially disconnected.", Name(player));
		XFREE(buf);
	}

	for (MODULE *cam__mp = mushstate.modules_list; cam__mp != NULL; cam__mp = cam__mp->next)
	{
		if (cam__mp->announce_disconnect)
		{
			(*(cam__mp->announce_disconnect))(player, reason, num);
		}
	}

	announce_connattr(d, player, loc, reason, num, A_ADISCONNECT);

	if (num < 1)
	{
		if (d->flags & DS_AUTODARK)
		{
			s_Flags(d->player, Flags(d->player) & ~DARK);
			d->flags &= ~DS_AUTODARK;
		}

		if (Guest(player))
		{
			s_Flags(player, Flags(player) | DARK);
		}
	}

	mushstate.curr_enactor = temp;
	desc_delhash(d);
}

int boot_off(dbref player, char *message)
{
	DESC *d = NULL, *dnext = NULL;
	int count = 0;

	if (!Good_obj(player))
	{
		return 0;
	}

	for (d = (DESC *)nhashfind((int)player, &mushstate.desc_htab), dnext = ((d != NULL) ? d->hashnext : NULL); d; d = dnext, dnext = ((dnext != NULL) ? dnext->hashnext : NULL))
	{
		if (message && *message)
		{
			queue_rawstring(d, NULL, message);
			queue_write(d, "\r\n", 2);
		}

		shutdownsock(d, R_BOOT);
		count++;
	}
	return count;
}

int boot_by_port(int port, int no_god, char *message)
{
	DESC *d, *dnext;
	int count;
	count = 0;

	for (d = descriptor_list, dnext = ((d != NULL) ? d->next : NULL); d; d = dnext, dnext = ((dnext != NULL) ? dnext->next : NULL))
	{
		if ((d->descriptor == port) && (!no_god || !God(d->player)))
		{
			if (message && *message)
			{
				queue_rawstring(d, NULL, message);
				queue_write(d, "\r\n", 2);
			}

			shutdownsock(d, R_BOOT);
			count++;
		}
	}
	return count;
}

/* ---------------------------------------------------------------------------
 * desc_reload: Reload parts of net descriptor that are based on db info.
 */

void desc_reload(dbref player)
{
	DESC *d;
	char *buf;
	dbref aowner;
	int aflags, alen;

	if (!Good_obj(player))
	{
		return;
	}

	for (d = (DESC *)nhashfind((int)player, &mushstate.desc_htab); d; d = d->hashnext)
	{
		buf = atr_pget(player, A_TIMEOUT, &aowner, &aflags, &alen);
		d->timeout = (int)strtol(buf, (char **)NULL, 10);

		if (d->timeout <= 0)
		{
			d->timeout = mushconf.idle_timeout;
		}

		XFREE(buf);
	}
}

/* ---------------------------------------------------------------------------
 * fetch_idle, fetch_connect: Return smallest idle time/largest connect time
 * for a player (or -1 if not logged in)
 */

int fetch_idle(dbref target, int port_num)
{
	DESC *d;
	int result, idletime;
	result = -1;

	if (port_num < 0)
	{
		for (d = (DESC *)nhashfind((int)target, &mushstate.desc_htab); d; d = d->hashnext)
		{
			idletime = (mushstate.now - d->last_time);

			if ((result == -1) || (idletime < result))
			{
				result = idletime;
			}
		}
	}
	else
	{
		DESC *dnext;
		for (d = descriptor_list, dnext = ((d != NULL) ? d->next : NULL); d; d = dnext, dnext = ((dnext != NULL) ? dnext->next : NULL))
			if (d->flags & DS_CONNECTED)
			{
				if (d->descriptor == port_num)
				{
					idletime = (mushstate.now - d->last_time);

					if ((result == -1) || (idletime < result))
					{
						result = idletime;
					}

					return result;
				}
			}
	}

	return result;
}

int fetch_connect(dbref target, int port_num)
{
	DESC *d;
	int result, conntime;
	result = -1;

	if (port_num < 0)
	{
		for (d = (DESC *)nhashfind((int)target, &mushstate.desc_htab); d; d = d->hashnext)
		{
			conntime = (mushstate.now - d->connected_at);

			if (conntime > result)
			{
				result = conntime;
			}
		}
	}
	else
	{
		DESC *dnext;
		for (d = descriptor_list, dnext = ((d != NULL) ? d->next : NULL); d; d = dnext, dnext = ((dnext != NULL) ? dnext->next : NULL))
			if (d->flags & DS_CONNECTED)
			{
				if (d->descriptor == port_num)
				{
					conntime = (mushstate.now - d->connected_at);

					if (conntime > result)
					{
						result = conntime;
					}

					return result;
				}
			}
	}

	return result;
}

void check_idle(void)
{
	DESC *d, *dnext;
	time_t idletime;

	for (d = descriptor_list, dnext = ((d != NULL) ? d->next : NULL); d; d = dnext, dnext = ((dnext != NULL) ? dnext->next : NULL))
	{
		if (d->flags & DS_CONNECTED)
		{
			idletime = mushstate.now - d->last_time;

			if ((idletime > d->timeout) && !Can_Idle(d->player))
			{
				queue_rawstring(d, NULL, "*** Inactivity Timeout ***\r\n");
				shutdownsock(d, R_TIMEOUT);
			}
			else if (mushconf.idle_wiz_dark && (idletime > mushconf.idle_timeout) && Can_Idle(d->player) && Can_Hide(d->player) && !Hidden(d->player))
			{
				raw_notify(d->player, NULL, "*** Inactivity AutoDark ***");
				s_Flags(d->player, Flags(d->player) | DARK);
				d->flags |= DS_AUTODARK;
			}
		}
		else
		{
			idletime = mushstate.now - d->connected_at;

			if (idletime > mushconf.conn_timeout)
			{
				queue_rawstring(d, NULL, "*** Login Timeout ***\r\n");
				shutdownsock(d, R_TIMEOUT);
			}
		}
	}
}

char *trimmed_name(dbref player)
{
	const int NAME_TRIM_LEN = 16;
	char *cbuff = XMALLOC(MBUF_SIZE, "cbuff");
	XSTRNCPY(cbuff, Name(player), NAME_TRIM_LEN);
	cbuff[NAME_TRIM_LEN] = '\0';
	return cbuff;
}

char *trimmed_site(char *name)
{
	char *buff = XMALLOC(MBUF_SIZE, "buff");
	int max_chars = (mushconf.site_chars < MBUF_SIZE - 1) ? mushconf.site_chars : MBUF_SIZE - 1;
	XSTRNCPY(buff, name, max_chars);
	buff[max_chars] = '\0';
	return buff;
}

void dump_users(DESC *e, char *match, int key)
{
	DESC *d;
	int count;
	char *buf, *fp, *sp;
	char *flist = XMALLOC(4, "flist");
	char *slist = XMALLOC(4, "slist");
	char *s;
	dbref room_it;

	while (match && *match && isspace(*match))
	{
		match++;
	}

	if (!match || !*match)
	{
		match = NULL;
	}

	if (mushconf.have_pueblo == 1)
	{
		if ((e->flags & DS_PUEBLOCLIENT) && (Html(e->player)))
		{
			queue_string(e, NULL, "<pre>");
		}
	}

	buf = XMALLOC(MBUF_SIZE, "buf");

	if (key == CMD_SESSION)
	{
		queue_rawstring(e, NULL, "                               ");
		queue_rawstring(e, NULL, "     Characters Input----  Characters Output---\r\n");
	}

	queue_rawstring(e, NULL, "Player Name        On For Idle ");

	if (key == CMD_SESSION)
	{
		queue_rawstring(e, NULL, "Port Pend  Lost     Total  Pend  Lost     Total\r\n");
	}
	else if ((e->flags & DS_CONNECTED) && (Wizard_Who(e->player)) && (key == CMD_WHO))
	{
		queue_rawstring(e, NULL, "  Room    Cmds   Host\r\n");
	}
	else
	{
		if (Wizard_Who(e->player) || See_Hidden(e->player))
		{
			queue_string(e, NULL, "  ");
		}
		else
		{
			queue_string(e, NULL, " ");
		}

		queue_string(e, NULL, mushstate.doing_hdr);
		queue_string(e, NULL, "\r\n");
	}

	count = 0;
	DESC *dnext;

	for (d = descriptor_list, dnext = ((d != NULL) ? d->next : NULL); d; d = dnext, dnext = ((dnext != NULL) ? dnext->next : NULL))
		if (d->flags & DS_CONNECTED)
		{
			if (!Hidden(d->player) || ((e->flags & DS_CONNECTED) && See_Hidden(e->player)))
			{
				count++;

				if (match && !(string_prefix(Name(d->player), match)))
				{
					continue;
				}

				if ((key == CMD_SESSION) && !(Wizard_Who(e->player) && (e->flags & DS_CONNECTED)) && (d->player != e->player))
				{
					continue;
				}

				/*
				 * Get choice flags for wizards
				 */
				fp = flist;
				sp = slist;

				if ((e->flags & DS_CONNECTED) && Wizard_Who(e->player))
				{
					if (Hidden(d->player))
					{
						if (d->flags & DS_AUTODARK)
						{
							*fp++ = 'd';
						}
						else
						{
							*fp++ = 'D';
						}
					}

					if (!Findable(d->player))
					{
						*fp++ = 'U';
					}
					else
					{
						room_it = where_room(d->player);

						if (Good_obj(room_it))
						{
							if (Hideout(room_it))
							{
								*fp++ = 'u';
							}
						}
						else
						{
							*fp++ = 'u';
						}
					}

					if (Suspect(d->player))
					{
						*fp++ = '+';
					}

					if (d->host_info & H_FORBIDDEN)
					{
						*sp++ = 'F';
					}

					if (d->host_info & H_REGISTRATION)
					{
						*sp++ = 'R';
					}

					if (d->host_info & H_SUSPECT)
					{
						*sp++ = '+';
					}

					if (d->host_info & H_GUEST)
					{
						*sp++ = 'G';
					}
				}
				else if ((e->flags & DS_CONNECTED) && See_Hidden(e->player))
				{
					if (Hidden(d->player))
					{
						if (d->flags & DS_AUTODARK)
						{
							*fp++ = 'd';
						}
						else
						{
							*fp++ = 'D';
						}
					}
				}

				*fp = '\0';
				*sp = '\0';

				if ((e->flags & DS_CONNECTED) && Wizard_Who(e->player) && (key == CMD_WHO))
				{
					s = XASPRINTF("s", "%s@%s", d->username, d->addr);
					char *trs = trimmed_site(((d->username[0] != '\0') ? s : d->addr));
					char *trn = trimmed_name(d->player);
					char *tf1 = time_format_1(mushstate.now - d->connected_at);
					char *tf2 = time_format_2(mushstate.now - d->last_time);
					XSPRINTF(buf, "%-16s%9s %4s%-3s#%-6d%5d%3s%-25s\r\n", trn, tf1, tf2, flist, Location(d->player), d->command_count, slist, trs);
					XFREE(s);
					XFREE(tf2);
					XFREE(tf1);
					XFREE(trn);
					XFREE(trs);
				}
				else if (key == CMD_SESSION)
				{
					char *trn = trimmed_name(d->player);
					char *tf1 = time_format_1(mushstate.now - d->connected_at);
					char *tf2 = time_format_2(mushstate.now - d->last_time);
					XSPRINTF(buf, "%-16s%9s %4s%5d%5d%6d%10d%6d%6d%10d\r\n", trn, tf1, tf2, d->descriptor, d->input_size, d->input_lost, d->input_tot, d->output_size, d->output_lost, d->output_tot);
					XFREE(tf1);
					XFREE(tf2);
					XFREE(trn);
				}
				else if (Wizard_Who(e->player) || See_Hidden(e->player))
				{
					char *trn = trimmed_name(d->player);
					char *tf1 = time_format_1(mushstate.now - d->connected_at);
					char *tf2 = time_format_2(mushstate.now - d->last_time);
					char *doing_str = (d->doing == NULL ? XSTRDUP("", "doing") : (resolve_color_type(e->player, e->player) == ColorTypeNone ? ansi_strip_ansi(d->doing) : XSTRDUP(d->doing, "doing")));
					XSPRINTF(buf, "%-16s%9s %4s%-3s%s\r\n", trn, tf1, tf2, flist, doing_str);
					XFREE(tf1);
					XFREE(tf2);
					XFREE(trn);
					XFREE(doing_str);
				}
				else
				{
					char *trn = trimmed_name(d->player);
					char *tf1 = time_format_1(mushstate.now - d->connected_at);
					char *tf2 = time_format_2(mushstate.now - d->last_time);
					char *doing_str = (d->doing == NULL ? XSTRDUP("", "doing") : (resolve_color_type(e->player, e->player) == ColorTypeNone ? ansi_strip_ansi(d->doing) : XSTRDUP(d->doing, "doing")));
					XSPRINTF(buf, "%-16s%9s %4s  %s\r\n", trn, tf1, tf2, doing_str);
					XFREE(tf1);
					XFREE(tf2);
					XFREE(trn);
					XFREE(doing_str);
				}

				queue_string(e, NULL, buf);
			}
		}
	/*
	 * sometimes I like the ternary operator....
	 */
	s = XASPRINTF("s", "%d", mushconf.max_players);
	XSPRINTF(buf, "%d Player%slogged in, %d record, %s maximum.\r\n", count, (count == 1) ? " " : "s ", mushstate.record_players, (mushconf.max_players == -1) ? "no" : s);
	XFREE(s);
	queue_rawstring(e, NULL, buf);

	if (mushconf.have_pueblo == 1)
	{
		if ((e->flags & DS_PUEBLOCLIENT) && (Html(e->player)))
		{
			queue_string(e, NULL, "</pre>");
		}
	}

	XFREE(slist);
	XFREE(flist);
	XFREE(buf);
}

void dump_info(DESC *call_by)
{
	DESC *d;
	char *temp;
	LINKEDLIST *llp;
	int count = 0;
	queue_rawstring(call_by, NULL, "### Begin INFO 1\r\n");
	queue_rawstring(call_by, "Name: %s\r\n", mushconf.mush_name);
	char uptime_buf[26];
	struct tm tm_buf;
	localtime_r(&mushstate.start_time, &tm_buf);
	strftime(uptime_buf, sizeof(uptime_buf), "%a %b %d %H:%M:%S %Y", &tm_buf);
	queue_rawstring(call_by, "Uptime: %s\r\n", uptime_buf);

	DESC *dnext;
	for (d = descriptor_list, dnext = ((d != NULL) ? d->next : NULL); d; d = dnext, dnext = ((dnext != NULL) ? dnext->next : NULL))
		if (d->flags & DS_CONNECTED)
		{
			if (!Hidden(d->player) || ((call_by->flags & DS_CONNECTED) && See_Hidden(call_by->player)))
			{
				count++;
			}
		}
	queue_rawstring(call_by, "Connected: %d\r\n", count);
	queue_rawstring(call_by, "Size: %d\r\n", mushstate.db_top);
	queue_rawstring(call_by, "Version: %d.%d.%d.%d-%d\r\n", mushstate.version.major, mushstate.version.minor, mushstate.version.patch, mushstate.version.tweak, mushstate.version.status);

	for (llp = mushconf.infotext_list; llp != NULL; llp = llp->next)
	{
		queue_rawstring(call_by, "%s: %s\r\n", llp->name, llp->value);
	}

	queue_rawstring(call_by, NULL, "### End INFO\r\n");
}

/* ---------------------------------------------------------------------------
 * do_colormap: Remap ANSI colors in output.
 */

void do_colormap(dbref player, dbref cause, int key, char *fstr, char *tstr)
{
	DESC *d;
	int from_color, to_color, i, x;
	from_color = mushcode_to_sgr((unsigned char)*fstr);
	to_color = mushcode_to_sgr((unsigned char)*tstr);

	if ((from_color < I_ANSI_BLACK) || (from_color >= I_ANSI_NUM))
	{
		notify(player, "That's not a valid color to change.");
		return;
	}

	if ((to_color < I_ANSI_BLACK) || (to_color >= I_ANSI_NUM))
	{
		notify(player, "That's not a valid color to remap to.");
		return;
	}

	for (d = (DESC *)nhashfind((int)player, &mushstate.desc_htab); d; d = d->hashnext)
	{
		if (d->colormap)
		{
			if (from_color == to_color)
			{
				d->colormap[from_color - I_ANSI_BLACK] = 0;
				/*
				 * If no changes, clear colormap
				 */
				x = 0;

				for (i = 0; i < I_ANSI_NUM - I_ANSI_BLACK; i++)
					if (d->colormap[i] != 0)
					{
						x++;
					}

				if (!x)
				{
					XFREE(d->colormap);
					notify(player, "Colors restored to standard.");
				}
				else
				{
					notify(player, "Color restored to standard.");
				}
			}
			else
			{
				d->colormap[from_color - I_ANSI_BLACK] = to_color;
				notify(player, "Color remapped.");
			}
		}
		else
		{
			if (from_color == to_color)
			{
				notify(player, "No color change.");
			}
			else
			{
				d->colormap = (int *)XCALLOC(I_ANSI_NUM - I_ANSI_BLACK, sizeof(int), "d->colormap");
				d->colormap[from_color - I_ANSI_BLACK] = to_color;
				notify(player, "Color remapped.");
			}
		}
	}
}

/* ---------------------------------------------------------------------------
 * do_doing: Set the doing string that appears in the WHO report.
 * Idea from R'nice@TinyTIM.
 */

char *sane_doing(char *arg, char *name)
{
	char *p, *bp;

	if (arg != NULL)
	{
		for (p = arg; *p; p++)
		{
			if ((*p == '\t') || (*p == '\r') || (*p == '\n'))
			{
				*p = ' ';
			}
			else if (!isprint(*p) && !isspace(*p) && *p != ESC_CHAR)
			{
				*p = '?';
			}
		}

		bp = XSTRDUP(arg, "bp");
	}
	else
	{
		return (XSTRDUP("", "name"));
	}

	return (bp);
}

void do_doing(dbref player, dbref cause, int key, char *arg)
{
	DESC *d;
	int foundany, over;
	over = 0;

	if (key & DOING_HEADER)
	{
		if (!(Can_Poll(player)))
		{
			notify(player, NOPERM_MESSAGE);
			return;
		}

		if (mushstate.doing_hdr != NULL)
		{
			XFREE(mushstate.doing_hdr);
		}

		if (!arg || !*arg)
		{
			mushstate.doing_hdr = sane_doing("Doing", "mushstate.doing_hdr");
		}
		else
		{
			mushstate.doing_hdr = sane_doing(arg, "mushstate.doing_hdr");
		}

		if (over)
		{
			notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Warning: %d characters lost.", over);
		}

		if (!Quiet(player) && !(key & DOING_QUIET))
		{
			notify(player, "Set.");
		}
	}
	else if (key & DOING_POLL)
	{
		notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Poll: %s", mushstate.doing_hdr);
	}
	else
	{
		foundany = 0;
		for (d = (DESC *)nhashfind((int)player, &mushstate.desc_htab); d; d = d->hashnext)
		{
			if (d->doing != NULL)
			{
				XFREE(d->doing);
			}

			d->doing = sane_doing(arg, "doing");
			foundany = 1;
		}

		if (foundany)
		{
			if (over)
			{
				notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME_ALL | MSG_F_DOWN, "Warning: %d characters lost.", over);
			}

			if (!Quiet(player) && !(key & DOING_QUIET))
			{
				notify(player, "Set.");
			}
		}
		else
		{
			notify(player, "Not connected.");
		}
	}
}

void init_logout_cmdtab(void)
{
	NAMETAB *cp;
	/*
	 * Make the htab bigger than the number of entries so that we find
	 * * things on the first check.  Remember that the admin can add
	 * * aliases.
	 */
	hashinit(&mushstate.logout_cmd_htab, 3 * mushconf.hash_factor, HT_STR);

	for (cp = logout_cmdtable; cp->flag; cp++)
	{
		hashadd(cp->name, (int *)cp, &mushstate.logout_cmd_htab, 0);
	}
}

void failconn(const char *logcode, const char *logtype, const char *logreason, DESC *d, int disconnect_reason, dbref player, int filecache, char *motd_msg, char *command, char *user, char *password)
{
	char *name;

	if (player != NOTHING)
	{
		name = log_getname(player);
		log_write(LOG_LOGIN | LOG_SECURITY, logcode, "RJCT", "[%d/%s] %s rejected to %s (%s)", d->descriptor, d->addr, logtype, name, logreason);
		XFREE(name);
	}
	else
	{
		log_write(LOG_LOGIN | LOG_SECURITY, logcode, "RJCT", "[%d/%s] %s rejected to %s (%s)", d->descriptor, d->addr, logtype, user, logreason);
	}

	fcache_dump(d, filecache);

	if (*motd_msg)
	{
		queue_string(d, NULL, motd_msg);
		queue_write(d, "\r\n", 2);
	}

	XFREE(command);
	XFREE(user);
	XFREE(password);
	shutdownsock(d, disconnect_reason);
}

const char *connect_fail = "Either that player does not exist, or has a different password.\r\n";
const char *create_fail = "Either there is already a player with that name, or that name is illegal.\r\n";

int check_connect(DESC *d, char *msg)
{
	char *command, *user, *password, *buff, *cmdsave, *name;
	dbref player, aowner;
	int aflags, alen, nplayers, reason;
	DESC *d2;
	char *p, *lname, *pname;
	cmdsave = mushstate.debug_cmd;
	mushstate.debug_cmd = (char *)"< check_connect >";
	/*
	 * Hide the password length from SESSION
	 */
	d->input_tot -= (strlen(msg) + 1);
	/*
	 * Crack the command apart
	 */
	command = XMALLOC(LBUF_SIZE, "command");
	user = XMALLOC(LBUF_SIZE, "user");
	password = XMALLOC(LBUF_SIZE, "password");
	parse_connect(msg, command, user, password);

	if (!strncmp(command, "co", 2) || !strncmp(command, "cd", 2))
	{
		if ((string_prefix(user, mushconf.guest_basename)) && Good_obj(mushconf.guest_char) && (mushconf.control_flags & CF_LOGIN))
		{
			if ((p = make_guest(d)) == NULL)
			{
				queue_string(d, NULL, "All guests are tied up, please try again later.\r\n");
				XFREE(command);
				XFREE(user);
				XFREE(password);
				XFREE(mushstate.debug_cmd);
				mushstate.debug_cmd = cmdsave;
				return 0;
			}

			XSTRCPY(user, p);
			XSTRCPY(password, mushconf.guest_password);
		}

		/*
		 * See if this connection would exceed the max #players
		 */

		if (mushconf.max_players < 0)
		{
			nplayers = mushconf.max_players - 1;
		}
		else
		{
			nplayers = 0;
			DESC *d2next;

			for (d2 = descriptor_list, d2next = ((d2 != NULL) ? d2->next : NULL); d2; d2 = d2next, d2next = ((d2next != NULL) ? d2next->next : NULL))
				if (d2->flags & DS_CONNECTED)
					nplayers++;
		}

		char login_addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(d->address).sin_addr, login_addr, sizeof(login_addr));
		player = connect_player(user, password, d->addr, d->username, login_addr);

		if (player == NOTHING)
		{
			/*
			 * Not a player, or wrong password
			 */
			queue_rawstring(d, NULL, connect_fail);
			log_write(LOG_LOGIN | LOG_SECURITY, "CON", "BAD", "[%d/%s] Failed connect to '%s'", d->descriptor, d->addr, user);
			user[3800] = '\0';

			if (--(d->retries_left) <= 0)
			{
				XFREE(command);
				XFREE(user);
				XFREE(password);
				shutdownsock(d, R_BADLOGIN);
				XFREE(mushstate.debug_cmd);
				mushstate.debug_cmd = cmdsave;
				return 0;
			}
		}
		else if (((mushconf.control_flags & CF_LOGIN) && (nplayers < mushconf.max_players)) || WizRoy(player) || God(player))
		{
			if (Guest(player))
			{
				reason = R_GUEST;
			}
			else if (!strncmp(command, "cd", 2) && (Wizard(player) || God(player)))
			{
				s_Flags(player, Flags(player) | DARK);
				reason = R_DARK;
			}
			else
			{
				reason = R_CONNECT;
			}

			/*
			 * First make sure we don't have a guest from a bad host.
			 */

			if (Guest(player) && (d->host_info & H_GUEST))
			{
				failconn("CON", "Connect", "Guest Site Forbidden", d, R_GAMEDOWN, player, FC_CONN_SITE, mushconf.downmotd_msg, command, user, password);
				XFREE(mushstate.debug_cmd);
				mushstate.debug_cmd = cmdsave;
				return 0;
			}

			/*
			 * Logins are enabled, or wiz or god
			 */
			pname = log_getname(player);

			if ((mushconf.log_info & LOGOPT_LOC) && Has_location(player))
			{
				lname = log_getname(Location(player));
				log_write(LOG_LOGIN, "CON", "LOGIN", "[%d/%s] %s in %s %s %s", d->descriptor, d->addr, pname, lname, connReasons(reason), user);
				XFREE(lname);
			}
			else
			{
				log_write(LOG_LOGIN, "CON", "LOGIN", "[%d/%s] %s %s %s", d->descriptor, d->addr, pname, connReasons(reason), user);
			}

			XFREE(pname);
			d->flags |= DS_CONNECTED;
			d->connected_at = time(NULL);
			d->player = player;
			/*
			 * Check to see if the player is currently running
			 * * an @program. If so, drop the new descriptor into
			 * * it.
			 */
			for (d2 = (DESC *)nhashfind((int)player, &mushstate.desc_htab); d2; d2 = d2->hashnext)
			{
				if (d2->program_data != NULL)
				{
					d->program_data = d2->program_data;
					break;
				}
			}

			/*
			 * Give the player the MOTD file and the settable
			 * * MOTD message(s). Use raw notifies so the
			 * * player doesn't try to match on the text.
			 */

			if (Guest(player))
			{
				fcache_dump(d, FC_CONN_GUEST);
			}
			else
			{
				buff = atr_get(player, A_LAST, &aowner, &aflags, &alen);

				if (!*buff)
				{
					fcache_dump(d, FC_CREA_NEW);
				}
				else
				{
					fcache_dump(d, FC_MOTD);
				}

				if (Wizard(player))
				{
					fcache_dump(d, FC_WIZMOTD);
				}

				XFREE(buff);
			}

			announce_connect(player, d, connMessages(reason));

			/*
			 * If stuck in an @prog, show the prompt
			 */

			if (d->program_data != NULL)
			{
				queue_rawstring(d, NULL, "> \377\371");
			}
		}
		else if (!(mushconf.control_flags & CF_LOGIN))
		{
			failconn("CON", "Connect", "Logins Disabled", d, R_GAMEDOWN, player, FC_CONN_DOWN, mushconf.downmotd_msg, command, user, password);
			XFREE(mushstate.debug_cmd);
			mushstate.debug_cmd = cmdsave;
			return 0;
		}
		else
		{
			failconn("CON", "Connect", "Game Full", d, R_GAMEFULL, player, FC_CONN_FULL, mushconf.fullmotd_msg, command, user, password);
			XFREE(mushstate.debug_cmd);
			mushstate.debug_cmd = cmdsave;
			return 0;
		}
	}
	else if (!strncmp(command, "cr", 2))
	{
		reason = R_CREATE;

		/*
		 * Enforce game down
		 */

		if (!(mushconf.control_flags & CF_LOGIN))
		{
			failconn("CRE", "Create", "Logins Disabled", d, R_GAMEDOWN, NOTHING, FC_CONN_DOWN, mushconf.downmotd_msg, command, user, password);
			XFREE(mushstate.debug_cmd);
			mushstate.debug_cmd = cmdsave;
			return 0;
		}

		/*
		 * Enforce max #players
		 */

		if (mushconf.max_players < 0)
		{
			nplayers = mushconf.max_players;
		}
		else
		{
			nplayers = 0;
			DESC *d2next;

			for (d2 = descriptor_list, d2next = ((d2 != NULL) ? d2->next : NULL); d2; d2 = d2next, d2next = ((d2next != NULL) ? d2next->next : NULL))
				if (d2->flags & DS_CONNECTED)
					nplayers++;
		}

		if (nplayers > mushconf.max_players)
		{
			/*
			 * Too many players on, reject the attempt
			 */
			failconn("CRE", "Create", "Game Full", d, R_GAMEFULL, NOTHING, FC_CONN_FULL, mushconf.fullmotd_msg, command, user, password);
			XFREE(mushstate.debug_cmd);
			mushstate.debug_cmd = cmdsave;
			return 0;
		}

		if (d->host_info & H_REGISTRATION)
		{
			fcache_dump(d, FC_CREA_REG);
		}
		else
		{
			player = create_player(user, password, NOTHING, 0, 0);

			if (player == NOTHING)
			{
				queue_rawstring(d, NULL, create_fail);
				log_write(LOG_SECURITY | LOG_PCREATES, "CON", "BAD", "[%d/%s] Create of '%s' failed", d->descriptor, d->addr, user);
			}
			else
			{
				name = log_getname(player);
				log_write(LOG_LOGIN | LOG_PCREATES, "CON", "CREA", "[%d/%s] %s %s", d->descriptor, d->addr, connReasons(reason), name);
				XFREE(name);
				move_object(player, (Good_loc(mushconf.start_room) ? mushconf.start_room : 0));
				d->flags |= DS_CONNECTED;
				d->connected_at = time(NULL);
				d->player = player;
				fcache_dump(d, FC_CREA_NEW);
				announce_connect(player, d, connMessages(R_CREATE));
			}
		}
	}
	else
	{
		welcome_user(d);
		log_write(LOG_LOGIN | LOG_SECURITY, "CON", "BAD", "[%d/%s] Failed connect: '%s'", d->descriptor, d->addr, msg);
		msg[150] = '\0';
	}

	XFREE(command);
	XFREE(user);
	XFREE(password);
	XFREE(mushstate.debug_cmd);
	mushstate.debug_cmd = cmdsave;
	return 1;
}

void logged_out_internal(DESC *d, int key, char *arg)
{
	switch (key)
	{
	case CMD_QUIT:
		shutdownsock(d, R_QUIT);
		break;

	case CMD_LOGOUT:
		shutdownsock(d, R_LOGOUT);
		break;

	case CMD_WHO:
	case CMD_DOING:
	case CMD_SESSION:
		dump_users(d, arg, key);
		break;

	case CMD_PREFIX:
		set_userstring(&d->output_prefix, arg);
		break;

	case CMD_SUFFIX:
		set_userstring(&d->output_suffix, arg);
		break;

	case CMD_INFO:
		dump_info(d);
		break;

	case CMD_PUEBLOCLIENT:
		if (mushconf.have_pueblo == 1)
		{
			/*
			 * Set the descriptor's flag
			 */
			d->flags |= DS_PUEBLOCLIENT;

			/*
			 * If we're already connected, set the player's flag
			 */
			if (d->flags & DS_CONNECTED)
			{
				s_Html(d->player);
			}

			queue_rawstring(d, NULL, mushconf.pueblo_msg);
			queue_write(d, "\r\n", 2);
			fcache_dump(d, FC_CONN_HTML);
			log_write(LOG_LOGIN, "CON", "HTML", "[%d/%s] PuebloClient enabled.", d->descriptor, d->addr);
		}
		else
		{
			queue_rawstring(d, NULL, "Sorry. This MUSH does not have Pueblo support enabled.\r\n");
		}

		break;

	default:
		log_write(LOG_BUGS, "BUG", "PARSE", "Logged-out command with no handler: '%s'", mushstate.debug_cmd);
	}
}

void do_command(DESC *d, char *command, int first)
{
	NAMETAB *cp = NULL;
	long begin_time = 0L, used_time = 0L;
	char *arg = NULL, *cmdsave = NULL, *log_cmdbuf = NULL, *pname = NULL, *lname = NULL;

	cmdsave = mushstate.debug_cmd;
	mushstate.debug_cmd = XSTRDUP("< do_command >", "mushstate.debug_cmd");

	if (d->flags & DS_CONNECTED)
	{
		/*
		 * Normal logged-in command processing
		 */
		d->command_count++;

		if (d->output_prefix)
		{
			queue_string(d, NULL, d->output_prefix);
			queue_write(d, "\r\n", 2);
		}

		mushstate.curr_player = d->player;
		mushstate.curr_enactor = d->player;

		if (mushstate.rdata)
		{
			for (int z = 0; z < mushstate.rdata->q_alloc; z++)
			{
				if (mushstate.rdata->q_regs[z])
					XFREE(mushstate.rdata->q_regs[z]);
			}

			for (int z = 0; z < mushstate.rdata->xr_alloc; z++)
			{
				if (mushstate.rdata->x_names[z])
					XFREE(mushstate.rdata->x_names[z]);

				if (mushstate.rdata->x_regs[z])
					XFREE(mushstate.rdata->x_regs[z]);
			}

			if (mushstate.rdata->q_regs)
			{
				XFREE(mushstate.rdata->q_regs);
			}

			if (mushstate.rdata->q_lens)
			{
				XFREE(mushstate.rdata->q_lens);
			}

			if (mushstate.rdata->x_names)
			{
				XFREE(mushstate.rdata->x_names);
			}

			if (mushstate.rdata->x_regs)
			{
				XFREE(mushstate.rdata->x_regs);
			}

			if (mushstate.rdata->x_lens)
			{
				XFREE(mushstate.rdata->x_lens);
			}

			XFREE(mushstate.rdata);
		}

		mushstate.rdata = NULL;

		if (mushconf.lag_check)
		{
			begin_time = time(NULL);
		}

		mushstate.cmd_invk_ctr = 0;
		log_cmdbuf = process_command(d->player, d->player, 1, command, (char **)NULL, 0);

		if (mushconf.lag_check)
		{
			used_time = time(NULL) - begin_time;

			if (used_time >= mushconf.max_cmdsecs)
			{
				pname = log_getname(d->player);

				if ((mushconf.log_info & LOGOPT_LOC) && Has_location(d->player))
				{
					lname = log_getname(Location(d->player));
					log_write(LOG_PROBLEMS, "CMD", "CPU", "%s in %s entered command taking %ld secs: %s", pname, lname, used_time, log_cmdbuf);
					XFREE(lname);
				}
				else
				{
					log_write(LOG_PROBLEMS, "CMD", "CPU", "%s entered command taking %ld secs: %s", pname, used_time, log_cmdbuf);
				}

				XFREE(pname);
			}
		}
		XFREE(log_cmdbuf);

		mushstate.curr_cmd = (char *)"";

		if (d->output_suffix)
		{
			queue_string(d, NULL, d->output_suffix);
			queue_write(d, "\r\n", 2);
		}

		XFREE(mushstate.debug_cmd);
		mushstate.debug_cmd = cmdsave;
		return;
	}

	/*
	 * Login screen (logged-out) command processing.
	 */
	/*
	 * Split off the command from the arguments
	 */
	arg = command;

	while (*arg && !isspace(*arg))
	{
		arg++;
	}

	if (*arg)
	{
		*arg++ = '\0';
	}

	/*
	 * Look up the command in the logged-out command table.
	 */
	cp = (NAMETAB *)hashfind(command, &mushstate.logout_cmd_htab);

	if (cp == NULL)
	{
		/*
		 * Not in the logged-out command table, so maybe a
		 * * connect attempt.
		 */
		if (*arg)
		{
			*--arg = ' '; /* restore nullified space */
		}

		XFREE(mushstate.debug_cmd);
		mushstate.debug_cmd = cmdsave;
		check_connect(d, command);
		return;
	}

	/*
	 * The command was in the logged-out command table.  Perform prefix
	 * * and suffix processing, and invoke the command handler.
	 */
	d->command_count++;

	if (!(cp->flag & CMD_NOxFIX))
	{
		if (d->output_prefix)
		{
			queue_string(d, NULL, d->output_prefix);
			queue_write(d, "\r\n", 2);
		}
	}

	if (cp->perm != CA_PUBLIC)
	{
		queue_rawstring(d, NULL, "Permission denied.\r\n");
	}
	else
	{
		XFREE(mushstate.debug_cmd);
		mushstate.debug_cmd = cp->name;
		logged_out_internal(d, cp->flag & CMD_MASK, arg);
	}

	/*
	 * QUIT or LOGOUT will close the connection and cause the
	 * * descriptor to be freed!
	 */
	if (((cp->flag & CMD_MASK) != CMD_QUIT) && ((cp->flag & CMD_MASK) != CMD_LOGOUT) && !(cp->flag & CMD_NOxFIX))
	{
		if (d->output_suffix)
		{
			queue_string(d, NULL, d->output_suffix);
			queue_write(d, "\r\n", 2);
		}
	}

	XFREE(mushstate.debug_cmd);
	mushstate.debug_cmd = cmdsave;
}

void logged_out(dbref player, dbref cause, int key, char *arg)
{
	DESC *d, *dlast;

	if (key == CMD_PUEBLOCLIENT)
	{
		/*
		 * PUEBLOCLIENT affects all the player's connections.
		 */
		for (d = (DESC *)nhashfind((int)player, &mushstate.desc_htab); d; d = d->hashnext)
		{
			logged_out_internal(d, key, arg);
		}
	}
	else
	{
		/*
		 * Other logged-out commands affect only the player's
		 * * most recently used connection.
		 */
		dlast = NULL;
		for (d = (DESC *)nhashfind((int)player, &mushstate.desc_htab); d; d = d->hashnext)
		{
			if (dlast == NULL || d->last_time > dlast->last_time)
			{
				dlast = d;
			}
		}

		if (dlast)
		{
			logged_out_internal(dlast, key, arg);
		}
	}
}

void process_commands(void)
{
	int nprocessed;
	DESC *d, *dnext;
	CBLK *t;
	char *cmdsave;
	cmdsave = mushstate.debug_cmd;
	mushstate.debug_cmd = XSTRDUP("process_commands", "mushstate.debug_cmd");

	do
	{
		nprocessed = 0;
		for (d = descriptor_list, dnext = ((d != NULL) ? d->next : NULL); d; d = dnext, dnext = ((dnext != NULL) ? dnext->next : NULL))
		{
			if (d->quota > 0 && (t = d->input_head))
			{
				d->quota--;
				nprocessed++;
				d->input_head = (CBLK *)t->hdr.nxt;

				if (!d->input_head)
				{
					d->input_tail = NULL;
				}

				d->input_size -= (strlen(t->cmd) + 1);
				log_write(LOG_KBCOMMANDS, "CMD", "KBRD", "[%d/%s] Cmd: %s", d->descriptor, d->addr, t->cmd);

				/*
				 * ignore the IDLE psuedo-command
				 */
				if (strcmp(t->cmd, (char *)"IDLE"))
				{
					d->last_time = mushstate.now;

					if (d->program_data != NULL)
					{
						handle_prog(d, t->cmd);
					}
					else
					{
						do_command(d, t->cmd, 1);
					}
				}

				XFREE(t);
			}
		}
	} while (nprocessed > 0);

	XFREE(mushstate.debug_cmd);
	mushstate.debug_cmd = cmdsave;
}

/* ---------------------------------------------------------------------------
 * site_check: Check for site flags in a site list.
 */

int site_check(struct in_addr host, SITE *site_list)
{
	SITE *this;
	int flag = 0;

	for (this = site_list; this; this = this->next)
	{
		if ((host.s_addr & this->mask.s_addr) == this->address.s_addr)
		{
			flag |= this->flag;
		}
	}

	return flag;
}

/* --------------------------------------------------------------------------
 * list_sites: Display information in a site list
 */

const char *stat_string(int strtype, int flag)
{
	const char *str;

	switch (strtype)
	{
	case S_SUSPECT:
		if (flag)
		{
			str = "Suspected";
		}
		else
		{
			str = "Trusted";
		}

		break;

	case S_ACCESS:
		switch (flag)
		{
		case H_FORBIDDEN:
			str = "Forbidden";
			break;

		case H_REGISTRATION:
			str = "Registration";
			break;

		case H_GUEST:
			str = "NoGuest";
			break;

		case 0:
			str = "Unrestricted";
			break;

		default:
			str = "Strange";
		}

		break;

	default:
		str = "Strange";
	}

	return str;
}

unsigned int mask_to_prefix(unsigned int mask_num)
{
	unsigned int i, result, tmp;

	/*
	 * The number of bits in the mask is equal to the number of left
	 * * shifts before it becomes zero. Binary search for that number.
	 */

	for (i = 16, result = 0; i && mask_num; i >>= 1)
		if ((tmp = (mask_num << i)))
		{
			result |= i;
			mask_num = tmp;
		}

	if (mask_num)
	{
		result++;
	}

	return result;
}

void list_sites(dbref player, SITE *site_list, const char *header_txt, int stat_type, bool header, bool footer)
{
	char *buff, *str, *maskaddr;
	SITE *this;
	unsigned int bits;
	buff = XMALLOC(MBUF_SIZE, "buff");

	if (header)
	{
		notify(player, "Type                IP Prefix           Mask                Status");
		notify(player, "------------------- ------------------- ------------------- -------------------");
	}

	for (this = site_list; this; this = this->next)
	{
		str = (char *)stat_string(stat_type, this->flag);
		bits = mask_to_prefix(ntohl(this->mask.s_addr));

		/*
		 * special-case 0, can't shift by 32
		 */
		if ((bits == 0 && htonl(0) == this->mask.s_addr) || htonl(0xFFFFFFFFU << (32 - bits)) == this->mask.s_addr)
		{
			char cidr_addr[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &this->address, cidr_addr, sizeof(cidr_addr));
			XSPRINTF(buff, "%-19.19s %-19.19s /%-19d %s", header_txt, cidr_addr, bits, str);
		}
		else
		{
			/*
			 * Deal with bizarre stuff not along CIDRized boundaries.
			 */
			char mask_addr[INET_ADDRSTRLEN];
			char site_addr[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &this->mask, mask_addr, sizeof(mask_addr));
			inet_ntop(AF_INET, &this->address, site_addr, sizeof(site_addr));
			XSPRINTF(buff, "%-17s %-17s %s", site_addr, mask_addr, str);
		}

		notify(player, buff);
	}

	if (footer)
	{
		notify(player, "-------------------------------------------------------------------------------");
	}

	XFREE(buff);
}

/* ---------------------------------------------------------------------------
 * list_siteinfo: List information about specially-marked sites.
 */

void list_siteinfo(dbref player)
{
	list_sites(player, mushstate.access_list, "Site Access", S_ACCESS, true, false);
	list_sites(player, mushstate.suspect_list, "Suspected Sites", S_SUSPECT, false, true);
}

/* ---------------------------------------------------------------------------
 * make_ulist: Make a list of connected user numbers for the LWHO function.
 */

void make_ulist(dbref player, char *buff, char **bufc)
{
	char *cp;
	DESC *d, *dnext;
	cp = *bufc;

	for (d = descriptor_list, dnext = ((d != NULL) ? d->next : NULL); d; d = dnext, dnext = ((dnext != NULL) ? dnext->next : NULL))
		if (d->flags & DS_CONNECTED)
		{
			if (!See_Hidden(player) && Hidden(d->player))
			{
				continue;
			}

			if (cp != *bufc)
			{
				XSAFELBCHR(' ', buff, bufc);
			}

			XSAFELBCHR('#', buff, bufc);
			XSAFELTOS(buff, bufc, d->player, LBUF_SIZE);
		}
}

/* ---------------------------------------------------------------------------
 * make_portlist: Make a list of ports for PORTS().
 */

void make_portlist(dbref player, dbref target, char *buff, char **bufc)
{
	DESC *d, *dnext;
	char *s = XMALLOC(MBUF_SIZE, "s");
	int i = 0;

	for (d = descriptor_list, dnext = ((d != NULL) ? d->next : NULL); d; d = dnext, dnext = ((dnext != NULL) ? dnext->next : NULL))
		if (d->flags & DS_CONNECTED)
		{
			if ((target == NOTHING) || (d->player == target))
			{
				XSNPRINTF(s, MBUF_SIZE, "%d ", d->descriptor);
				XSAFELBSTR(s, buff, bufc);
				i = 1;
			}
		}

	if (i)
	{
		(*bufc)--;
	}

	**bufc = '\0';
	XFREE(s);
}

/* ---------------------------------------------------------------------------
 * make_sessioninfo: Return information about a port, for SESSION().
 * List of numbers: command_count input_tot output_tot
 */

void make_sessioninfo(dbref player, dbref target, int port_num, char *buff, char **bufc)
{
	DESC *d, *dnext;
	char *s = XMALLOC(MBUF_SIZE, "s");

	for (d = descriptor_list, dnext = ((d != NULL) ? d->next : NULL); d; d = dnext, dnext = ((dnext != NULL) ? dnext->next : NULL))
		if (d->flags & DS_CONNECTED)
		{
			if ((d->descriptor == port_num) || (d->player == target))
			{
				if (Wizard_Who(player) || Controls(player, d->player))
				{
					XSNPRINTF(s, MBUF_SIZE, "%d %d %d", d->command_count, d->input_tot, d->output_tot);
					XSAFELBSTR(s, buff, bufc);
					XFREE(s);
					return;
				}
				else
				{
					notify_quiet(player, NOPERM_MESSAGE);
					XSAFELBSTR((char *)"-1 -1 -1", buff, bufc);
					XFREE(s);
					return;
				}
			}
		}
	/*
	 * Not found, return error.
	 */
	XSAFELBSTR((char *)"-1 -1 -1", buff, bufc);
	XFREE(s);
}

/* ---------------------------------------------------------------------------
 * get_doing: Return the DOING string of a player.
 */

char *get_doing(dbref target, int port_num)
{
	DESC *d;

	if (port_num < 0)
	{
		for (d = (DESC *)nhashfind((int)target, &mushstate.desc_htab); d; d = d->hashnext)
		{
			return d->doing;
		}
	}
	else
	{
		DESC *dnext;
		for (d = descriptor_list, dnext = ((d != NULL) ? d->next : NULL); d; d = dnext, dnext = ((dnext != NULL) ? dnext->next : NULL))
			if (d->flags & DS_CONNECTED)
			{
				if (d->descriptor == port_num)
				{
					return d->doing;
				}
			}
	}

	return NULL;
}

/* ---------------------------------------------------------------------------
 * get_programmer: Get the dbref of the controlling programmer, if any.
 */

dbref get_programmer(dbref target)
{
	DESC *d, *dnext;

	for (d = descriptor_list, dnext = ((d != NULL) ? d->next : NULL); d; d = dnext, dnext = ((dnext != NULL) ? dnext->next : NULL))
		if (d->flags & DS_CONNECTED)
		{
			if ((d->player == target) && (d->program_data != NULL))
			{
				return (d->program_data->wait_cause);
			}
		}
	return NOTHING;
}

/* ---------------------------------------------------------------------------
 * find_connected_name: Resolve a playername from the list of connected
 * players using prefix matching.  We only return a match if the prefix
 * was unique.
 */

dbref find_connected_name(dbref player, char *name)
{
	DESC *d, *dnext;
	dbref found;
	found = NOTHING;

	for (d = descriptor_list, dnext = ((d != NULL) ? d->next : NULL); d; d = dnext, dnext = ((dnext != NULL) ? dnext->next : NULL))
		if (d->flags & DS_CONNECTED)
		{
			if (Good_obj(player) && !See_Hidden(player) && Hidden(d->player))
			{
				continue;
			}

			if (!string_prefix(Name(d->player), name))
			{
				continue;
			}

			if ((found != NOTHING) && (found != d->player))
			{
				return NOTHING;
			}

			found = d->player;
		}
	return found;
}

/* ---------------------------------------------------------------------------
 * find_connected_ambiguous: Resolve a playername from the list of connected
 * players using prefix matching.  If the prefix is non-unique, we return
 * the AMBIGUOUS token; if it does not exist, we return the NOTHING token.
 * was unique.
 */

dbref find_connected_ambiguous(dbref player, char *name)
{
	DESC *d, *dnext;
	dbref found;
	found = NOTHING;

	for (d = descriptor_list, dnext = ((d != NULL) ? d->next : NULL); d; d = dnext, dnext = ((dnext != NULL) ? dnext->next : NULL))
		if (d->flags & DS_CONNECTED)
		{
			if (Good_obj(player) && !See_Hidden(player) && Hidden(d->player))
			{
				continue;
			}

			if (!string_prefix(Name(d->player), name))
			{
				continue;
			}

			if ((found != NOTHING) && (found != d->player))
			{
				return AMBIGUOUS;
			}

			found = d->player;
		}
	return found;
}
