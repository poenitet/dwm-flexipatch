/* Bartabgroups properties, you can override these in your config.h if you want. */
#ifndef BARTAB_BORDERS
#define BARTAB_BORDERS 1       // 0 = off, 1 = on
#endif
#ifndef BARTAB_SHOWFLOATING
#define BARTAB_SHOWFLOATING 0  // whether to show titles for floating windows, hidden clients are always shown
#endif
#ifndef BARTAB_STACKWEIGHT
#define BARTAB_STACKWEIGHT 1   // stack weight compared to hidden and floating window titles
#endif
#ifndef BARTAB_HIDDENWEIGHT
#define BARTAB_HIDDENWEIGHT 1  // hidden window title weight
#endif
#ifndef BARTAB_FLOATWEIGHT
#define BARTAB_FLOATWEIGHT 1   // floating window title weight, set to 0 to not show floating windows
#endif

int
width_bartabgroups(Bar *bar, BarWidthArg *a)
{
	return a->max_width;
}

int
draw_bartabgroups(Bar *bar, BarDrawArg *a)
{
	drw_rect(drw, a->x, 0, a->w, bh, 1, 1);
	return bartabcalculate(bar->mon, a->x, a->w, -1, bartabdraw, NULL);
}

int
click_bartabgroups(Bar *bar, Arg *arg, BarClickArg *a)
{
	bartabcalculate(bar->mon, 0, a->rel_w, a->rel_x, bartabclick, arg);
	return ClkWinTitle;
}

void
bartabdraw(Monitor *m, Client *c, int unused, int x, int w, int groupactive, Arg *arg)
{
	if (!c)
		return;
	int i, nclienttags = 0, nviewtags = 0, pad = lrpad / 2;
	drw_setscheme(drw, scheme[
		m->sel == c
		? SchemeSel
		#ifdef HIDDEN
		: HIDDEN(c)
		? SchemeHid
		#endif
		: groupactive
		? SchemeTitleSel
		: SchemeTitleNorm
	]);
	if (w <= TEXTW("A") - lrpad + pad) // reduce text padding if wintitle is too small
		pad = (w - TEXTW("A") + lrpad < 0 ? 0 : (w - TEXTW("A") + lrpad) / 2);
	#if BAR_CENTEREDWINDOWNAME_PATCH
	else if (TEXTW(c->name) < w)
		pad = (w - TEXTW(c->name) + lrpad) / 2;
	#endif // BAR_CENTEREDWINDOWNAME_PATCH

	drw_text(drw, x, 0, w, bh, pad, c->name, 0);
	if (c->isfloating)
		drawindicator(m, c, 1, x, w, 0, 0, c->isfixed, floatindicatortype);

	if (BARTAB_BORDERS) {
		XSetForeground(drw->dpy, drw->gc, scheme[SchemeSel][ColBorder].pixel);
		XFillRectangle(drw->dpy, drw->drawable, drw->gc, x, 0, 1, bh);
		XFillRectangle(drw->dpy, drw->drawable, drw->gc, x + w, 0, 1, bh);
	}
	/* Optional tags icons */
	for (i = 0; i < LENGTH(tags); i++) {
		if ((m->tagset[m->seltags] >> i) & 1)
			nviewtags++;
		if ((c->tags >> i) & 1)
			nclienttags++;
	}

	if (TAGSINDICATOR == 2 || nclienttags > 1 || nviewtags > 1)
		drawindicator(m, c, 1, x, w, 0, 0, 0, INDICATOR_RIGHT_TAGS);
}

#ifndef HIDDEN
#define HIDDEN(C) 0
#endif

void
bartabclick(Monitor *m, Client *c, int passx, int x, int w, int unused, Arg *arg)
{
	if (passx >= x && passx <= x + w)
		arg->v = c;
}

int
bartabcalculate(
	Monitor *m, int offx, int tabw, int passx,
	void(*tabfn)(Monitor *, Client *, int, int, int, int, Arg *arg), Arg *arg
) {
	Client *c;
	int
		i, clientsnmaster = 0, clientsnstack = 0, clientsnfloating = 0, clientsnhidden = 0,
		masteractive = 0, fulllayout = 0,
		x = offx, w, r, num = 0, den, tgactive;

	for (i = 0; i < LENGTH(bartabmonfns); i++)
		if (m ->lt[m->sellt]->arrange == bartabmonfns[i]) {
			fulllayout = 1;
			break;
		}

	for (i = 0, c = m->clients; c; c = c->next) {
		if (!ISVISIBLE(c))
			continue;
		if (HIDDEN(c)) {
			clientsnhidden++;
			continue;
		}
		if (c->isfloating) {
			clientsnfloating++;
			continue;
		}
		if (m->sel == c)
			masteractive = i < m->nmaster;
		if (i < m->nmaster)
			clientsnmaster++;
		else
			clientsnstack++;
		i++;
	}

	if (clientsnmaster + clientsnstack + clientsnfloating + clientsnhidden == 0)
	 	return 0;

	tgactive = 1;
	num = tabw;
	/* floating mode */
	if ((fulllayout && BARTAB_FLOATWEIGHT) || clientsnmaster + clientsnstack == 0 || !m->lt[m->sellt]->arrange) {
		den = clientsnmaster + clientsnstack + clientsnfloating + clientsnhidden;
		r = num % den;
		w = num / den;
		for (c = m->clients, i = 0; c; c = c->next) {
			if (!ISVISIBLE(c))
				continue;
			tabfn(m, c, passx, x, w + (i < r ? 1 : 0), tgactive, arg);
			x += w + (i < r ? 1 : 0);
			i++;
		}
	/* no master and stack mode, e.g. monocole, grid layouts, fibonacci */
	} else if (fulllayout) {
		den = clientsnmaster + clientsnstack + clientsnhidden;
		r = num % den;
		w = num / den;
		for (c = m->clients, i = 0; c; c = c->next) {
			if (!ISVISIBLE(c) || (c->isfloating && !HIDDEN(c)))
				continue;
			tabfn(m, c, passx, x, w + (i < r ? 1 : 0), tgactive, arg);
			x += w + (i < r ? 1 : 0);
			i++;
		}
	/* tiled mode */
	} else {
		den = clientsnmaster;
		c = m->clients;
		i = 0;
		if (den) {
			if (clientsnstack + clientsnfloating * BARTAB_FLOATWEIGHT + clientsnhidden) {
				tgactive = masteractive;
				num = tabw * m->mfact;
			}
			r = num % den;
			w = num / den;
			for (; c && i < m->nmaster; c = c->next) { // tiled master
				if (!ISVISIBLE(c) || c->isfloating || HIDDEN(c))
					continue;
				tabfn(m, c, passx, x, w + (i < r ? 1 : 0), tgactive, arg);
				x += w + (i < r ? 1 : 0);
				i++;
			}
			tgactive = !tgactive;
			num = tabw - num;
		}

		den = clientsnstack * BARTAB_STACKWEIGHT + clientsnfloating * BARTAB_FLOATWEIGHT + clientsnhidden * BARTAB_HIDDENWEIGHT;
		if (!den)
			return 1;

		r = num % den;
		w = num / den;
		#if BARTAB_STACKWEIGHT
		for (; c; c = c->next) { // tiled stack
			if (!ISVISIBLE(c) || HIDDEN(c) || c->isfloating)
				continue;
			tabfn(m, c, passx, x, w * BARTAB_STACKWEIGHT + (i - m->nmaster < r ? 1 : 0), tgactive, arg);
			x += w * BARTAB_STACKWEIGHT + (i - m->nmaster < r ? 1 : 0);
			i++;
		}
		#endif // BARTAB_STACKWEIGHT

		#if BARTAB_HIDDENWEIGHT
		for (c = m->clients; c; c = c->next) { // hidden windows
			if (!ISVISIBLE(c) || !HIDDEN(c))
				continue;
			tabfn(m, c, passx, x, w * BARTAB_HIDDENWEIGHT + (i - m->nmaster < r ? 1 : 0), tgactive, arg);
			x += w * BARTAB_HIDDENWEIGHT + (i - m->nmaster < r ? 1 : 0);
			i++;
		}
		#endif // BARTAB_HIDDENWEIGHT

		#if BARTAB_FLOATWEIGHT
		for (c = m->clients; c; c = c->next) { // floating windows
			if (!ISVISIBLE(c) || HIDDEN(c) || !c->isfloating)
				continue;
			tabfn(m, c, passx, x, w * BARTAB_FLOATWEIGHT + (i - m->nmaster < r ? 1 : 0), tgactive, arg);
			x += w * BARTAB_FLOATWEIGHT + (i - m->nmaster < r ? 1 : 0);
			i++;
		}
		#endif // BARTAB_FLOATWEIGHT
	}
	return 1;
}