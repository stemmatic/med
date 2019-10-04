
/*
	lemma.c - Routines for manipulating the lemma and columns
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "med.h"
#include "norm.h"
#include "lemma.h"

struct VarList {
	int nStates;
	int ocol, ncol;
	struct VarList *next;
};

struct Lemma {
	enum LemmaOp vop;
	int olen, nlen;
	int ocol, ncol;
	struct VarList *lemvars, **plv;
	char swapping[WITSIZ];
};

static struct VarList *
	vlFree(struct VarList *vl)
{
	struct VarList *next;
	
	if (!vl)
		return NULL;
	next = vl->next;
	free(vl);
	vlFree(next);
	return NULL;
}

void
	lemmaScan(struct Lemma *lm, int c)
{
	struct VarList *vl;
	int lastc;
	char *sw = lm->swapping;

	if (lm->vop != vDrop && lm->vop != vSwapB)
		putchar(c);
	
	// Set up variation unit
	vl = malloc(sizeof *vl);
	vl->nStates = 1;
	if (lm->vop == vSwapA) {
		vl->ocol = lm->ocol+1;
		vl->ncol = lm->ncol++;
		lm->olen++;
		lm->nlen++;
	}
	if (lm->vop == vSwapB) {
		vl->ocol = lm->ocol;
		lm->ocol += 2;
		vl->ncol = lm->ncol++;
		lm->olen++;
		lm->nlen++;
	}
	if (lm->vop == vDrop) {
		vl->ocol = lm->ocol++;
		vl->ncol = -1;
		lm->olen++;
	}
	if (lm->vop == vAdd) {
		vl->ocol = -1;
		vl->ncol = lm->ncol++;
		lm->nlen++;
	}
	if (lm->vop == vKeep) {
		vl->ocol = lm->ocol++;
		vl->ncol = lm->ncol++;
		lm->olen++;
		lm->nlen++;
	}
	vl->next = NULL;

	// Count the different states for a vunit
	for (lastc = ' '; (c = getchar()) != EOF; lastc = c) {
		if (c == '|' || c == ']' || c == '{') {
			ungetc(c, stdin);
			break;
		}

		if (!isspace(c) && isspace(lastc))
			vl->nStates++;

		if (lm->vop == vSwapA)
			*sw++ = c;
		else if (lm->vop != vDrop)
			putchar(c);
	}
	if (lm->vop == vSwapB) {
		int swap_nSt;

		putchar('|');
		while ((c = *sw++))
			putchar(c);

		swap_nSt = vl->nStates;
		vl->nStates = (*lm->plv)->nStates;
		(*lm->plv)->nStates = swap_nSt;

		lm->plv = &(*lm->plv)->next;
	}
	if (lm->vop == vSwapA) {
		*sw = EOS;
		lm->vop = vSwapB;
		*lm->plv = vl;
	} else {
		lm->vop = vKeep;
		*lm->plv = vl;
		lm->plv = &vl->next;
	}
}

struct Lemma *
	lemmaBegin()
{
	struct Lemma *lm = malloc(sizeof *lm);
	
	lm->vop = vKeep;
	lm->ocol = lm->ncol = 0;
	lm->olen = lm->nlen = 0;
	lm->lemvars = 0;
	lm->plv = &lm->lemvars;

	return lm;
}

void
	lemmaEnd(struct Lemma **lm)
{
	if (!lm || !*lm)
		return;
	vlFree(lm[0]->lemvars);
	free(lm[0]);
	lm[0] = NULL;
}

void
	lemmaApply(struct WitGrp *wg, struct Lemma *lm)
{
	static char src[CODSIZ];
	int c, o, n;

	for (; wg; wg = wgNext(wg)) {
		struct VarList *vl;
		char *dst = wgCodes(wg);

		append(src, dst, endof(src));
		for (n = 0; n < lm->nlen; n++)
			dst[n] = '_';
		dst[n] = EOS;

		// Check for right number of columns (on fail, just leave with _)
		n = lm->olen;
		if (n > 0 && strlen(src) != n)
			continue;

		// Check for right values in the column
		for (vl = lm->lemvars; vl; vl = vl->next) {
			o = vl->ocol;
			n = vl->ncol;

			// No corresponding column, so drop it.
			if (n == -1)
				continue;

			c = (o == -1) ? '^' : src[o];
			if (c == '?' || c == '-' || c == '%' || c == '^' || c == '.')
				;
			else if (c < '0' || c >= '0'+vl->nStates)
				c = 'x';
			dst[n] = c;
		}
	}
}


void
	lemmaEdit(struct Special *sp, int ch)
{
	struct Lemma *lm = spLemma(sp);
	enum LemmaOp was, is;

	ch = getchar();

	was = lm->vop;
	switch (ch) {
	default:
		break;
	case '+':
		lm->vop = vAdd;
		break;
	case '_':
		lm->vop = vDrop;
		break;
	case '~':
		lm->vop = vSwapA;
		break;
	}
	is = lm->vop;
	if (was != vKeep || is == vKeep) {
		putchar('{');
		putchar(ch);
		putchar('}');
		lm->vop = was;
	}

	while ((ch = getchar()) != EOF && ch != '}')
		putchar(ch);
}

