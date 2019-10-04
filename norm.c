
/*
	norm.c - Normalize witness groups
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "med.h"
#include "norm.h"
#include "lemma.h"

struct WitGrp {
	struct WitGrp *next;	/* Next group of witnesses */
	char vcodes[CODSIZ];	/* Variant codes, e.g. 0000010 */
	char witbuf[WITSIZ];	/* List of witnesses, e.g. 01 B $L */
};

struct EditList {
	struct EditList *next;
	struct WitGrp *edit;
};

struct Pattern {
	char match[CODSIZ];		/* Pattern to match */
	char replc[CODSIZ];		/* Replacement pattern */
};

char *
	append(char *s, const char *t, char *end)
{
	do {
		if (s == end) {
			fprintf(stderr, "WARN: Max buffer size exceeded; "
				"token truncated.\n");
			s[-1] = EOS;
			return end;
		}
	} while ((*s++ = *t++) != EOS);

	return s-1;
}

static char *
	getToken(FILE *fp)
{
	static char token[TOKSIZ];
	char *s;
	int ch;

	// Skip initial white space
	while ((ch = getc(fp)) != EOF && isspace(ch))
		continue;
	if (ch == EOF)
		return NULL;

	// Collect chars for token
	s = token;
	*s++ = ch;
	if (isalnum(ch) || ch == '$' || ch == '?' || ch == '%' || ch == '*'
	|| ch == '^' || ch == '.') {
		while ((ch = getc(fp)) != EOF && !isspace(ch) && ch != '}') {
			if (s == &token[TOKSIZ-1]) {
				fprintf(stderr, "WARN: Max token size (%d) exceeded: %s\n",
					TOKSIZ, token);
				break;
			}
			*s++ = ch;
		}
		if (ch == '}')
			ungetc(ch, fp);
	}
	*s = EOS;

	return token;
}

static struct WitGrp *
	wgNew(const char *vc, const char *wb, struct WitGrp *next)
{
	struct WitGrp *wg = malloc(sizeof *wg);
	assert( wg != NULL );						// I know, I know

	append(wg->vcodes, vc, wg->vcodes + sizeof wg->vcodes);
	append(wg->witbuf, wb, wg->witbuf + sizeof wg->witbuf);
	wg->next = next;

	return wg;
}

struct WitGrp *
	wgNext(struct WitGrp *wg)
{
	return wg->next;
}

char *
	wgCodes(struct WitGrp *wg)
{
	return wg->vcodes;
}

char *
	wgWitns(struct WitGrp *wg)
{
	return wg->witbuf;
}

static struct WitGrp *
	wgInsert(struct WitGrp *wg, const char *vc, const char *wb)
{
	if (!wg || strcmp(vc, wg->vcodes) < 0)
		return wgNew(vc, wb, wg);

	if (strcmp(vc, wg->vcodes) == 0) {
		char *s = wg->witbuf;
		s += strlen(s);
		(void) append(s, wb, endof(wg->witbuf));
		return wg;
	}

	wg->next = wgInsert(wg->next, vc, wb);
	return wg;
}

static struct WitGrp *
	wgAppend(struct WitGrp *wg, const char *vc, const char *wb)
{
	if (!wg)
		return wgNew(vc, wb, wg);

	if (strcmp(vc, wg->vcodes) == 0) {
		char *s = wg->witbuf;
		s += strlen(s);
		(void) append(s, wb, endof(wg->witbuf));
		return wg;
	}

	wg->next = wgAppend(wg->next, vc, wb);
	return wg;
}

static char *
	wgRemove(struct WitGrp **pwg, const char *wb)
{
	char *s;
	char *vc;
	struct WitGrp *wg;
	int len;

	assert( pwg != NULL );
	wg = *pwg;
	if (!wg)
		return NULL;

	len = strlen(wb);
	s = strstr(wg->witbuf, wb);
	if (s && (s[len] == ' ' || s[len] == EOS)) {
		// Remove it
		strcpy(s, s+len);
		if (wg->witbuf == EOS) {
			*pwg = wg->next;
			free(wg);
		}
	} else
		s = NULL;
	vc = wgRemove(&(*pwg)->next, wb);
	return (s) ? wg->vcodes : vc;
}

static struct WitGrp *
	wgScan(FILE *fp)
{
	static char vc[CODSIZ];
	static char wb[WITSIZ];
	struct WitGrp *wg;
	char *token;

	wg = NULL;
	wb[0] = EOS;
	do {
		char *s = wb;
		if ((token = getToken(fp)) == NULL)
			return NULL;
		(void) append(vc, token, endof(vc));
		while ((token = getToken(fp)) != NULL) {
			if (*token == '>' || *token == '|')
				break;
			s = append(s, " ", endof(wb));
			s = append(s, token, endof(wb));
		}
		wg = wgAppend(wg, vc, wb);
	} while (token && *token != '>');

	return wg;
}


static char *
	glob(char *base, char *patt, char *deflt)
{
	static char vbuf[CODSIZ];
	int n;

	for (n = 0; patt[n]; n++) {
		int c = patt[n];

		if (c == '*') {
			if (deflt[0] != '$')
				c = deflt[n];
			else if (base)
				c = base[n];
			else
				c = '%';
		}
		if (c == '.' && base)
			c = base[n];
		vbuf[n] = c;
	}
	vbuf[n] = EOS;
	if (deflt[0] == '$')
		strcpy(deflt, vbuf);
	return vbuf;
}

static int
	match(char *base, char *patt)
{
	char *globbed = glob(base, patt, "...................");
	return (strcmp(base, globbed) == 0);
}

static struct WitGrp *
	wgEdit(struct WitGrp *wg, struct WitGrp *edit, char *first)
{
	char *s, *e;
	char *vr, *vc;
	static char wbuf[WITSIZ];

	if (!edit)
		return wg;

	s = edit->witbuf;
	if (!*s) {
		strcpy(wbuf, wg->witbuf);
		s = wbuf;
	}
	do {
		e = strchr(s+1, ' ');
		if (e)
			*e = EOS;
		vr = wgRemove(&wg, s);
		
		if (edit->vcodes[0] != '$') {
			vc = glob(vr, edit->vcodes, first);
			wg = wgInsert(wg, vc, s);
		}
		if (e) {
			*e = ' ';
			s = e;
		}
	} while (e);
	wg = wgEdit(wg, edit->next, first);
	return wg;
}

static void
	wgPrint(FILE *fp, struct WitGrp *wg)
{
	char *sep = "<";

	while (wg) {
		if (*wg->witbuf) {
			fprintf(fp, "%s %s%s", sep, wg->vcodes, wg->witbuf);
			sep = "\n\t|";
		}
		wg = wg->next;
	}
	fprintf(fp, " >");
}

static struct WitGrp *
	wgFree(struct WitGrp *wg)
{
	struct WitGrp *next;
	
	if (!wg)
		return NULL;
	next = wg->next;
	free(wg);
	wgFree(next);
	return NULL;
}

static void
	normalize(struct Ctx *ctx)
{
	struct WitGrp *wg;
	struct EditList *edits;
	struct Special *sp = ctx->active;
	char starbuf[CODSIZ];
	struct WitGrp *clone = NULL;

	assert( ctx->active != 0 );

	wg = wgScan(stdin);
	strcpy(starbuf, "$");

	// Modify the vcodes[] as appropriate
	if (sp->lemma)
		lemmaApply(wg, sp->lemma);

	// Sort (using insert) while cloning
	for (; wg; wg = wg->next) {
		char *vc = wg->vcodes;
		if (sp->pattn && match(vc, sp->pattn->match))
			vc = glob(vc, sp->pattn->replc, starbuf);
		clone = wgInsert(clone, vc, wg->witbuf);
	}
	wgFree(wg);
	wg = clone;

	// Add, replace, or insert new readings
	edits = ctx->active->edits;
	if (edits) {
		wg = wgEdit(wg, edits->edit, starbuf);
		wgFree(edits->edit);
		ctx->active->edits = edits->next;
	}

	wgPrint(stdout, wg);

	wgFree(wg);
	sp->cs = csOther;
}

static void
	variantEdit(struct Special *sp, int ch)
{
	putchar(ch);
	while ((ch = getchar()) != EOF && ch != '}')
		putchar(ch);
	if (ch != EOF)
		putchar(ch);
}

void foo() {}

static struct Pattern *
	patNew()
{
	struct Pattern *pat;

	pat = malloc(sizeof *pat);
	pat->match[0] = EOS;
	pat->replc[0] = EOS;
	return pat;
}

static void
	otherEdit(struct Special *sp, int ch)
{
	ch = getchar();
	switch (ch) {
	default:
		break;
	case '}':
		putchar('{');
		putchar('{');
		putchar('}');
		sp->keepBrace = YES;
		return;
	case '!':
		foo();
		break;
	case '+':
		ch = getchar();
		if (ch == '<') {
			struct EditList *edits = malloc(sizeof *edits);
			edits->edit = wgScan(stdin);
			edits->next = NULL;
			sp->edits = edits;
		}
		break;
	case '/':
		{
			static char wb[WITSIZ];
			char *token;
			struct EditList **prev;
			
			// Set up witness
			wb[0] = ' ';
			token = getToken(stdin);
			append(wb+1, token, endof(wb));
			
			// Scan each entry to edit for witness
			prev = &sp->edits;
			(*prev) = NULL;
			while ((token = getToken(stdin)) != NULL) {
				if (*token == '}')
					break;
				*prev = malloc(sizeof **prev);
				(*prev)->edit = wgNew(token, wb, NULL);
				(*prev)->next = NULL;
				prev = &(*prev)->next;
			}
		}
		return;
	case '%':
		{
			char *token;
			struct Pattern *pat;

			if (sp->pattn) {
				free(sp->pattn);
				sp->pattn = NULL;
			}

			pat = patNew();
			token = getToken(stdin);
			append(pat->match, token, endof(pat->match));
			token = getToken(stdin);
			append(pat->replc, token, endof(pat->replc));
			sp->pattn = pat;

			if (sp->keepBrace)
				fprintf(stdout, "{%% %s %s}", pat->match, pat->replc);
		}
		break;
	}
	while ((ch = getchar()) != EOF && ch != '}')
		putchar(ch);
}

struct Special *
	activate(struct Ctx *ctx)
{
	struct Special *sp = malloc(sizeof *sp);

	sp->keepBrace = NO;

	sp->cs = csOther;
	sp->edits = NULL;

	sp->lemma = NULL;
	
	sp->pattn = NULL;
	return sp;
}

void
	deactivate(struct Special **psp)
{
	if (!psp || !*psp)
		return;
	lemmaEnd(&psp[0]->lemma);
	if (psp[0]->pattn)
		free(psp[0]->pattn);
	free(*psp);
	*psp = NULL;
}

struct Lemma *spLemma(struct Special *sp) { return sp->lemma; }

// Editing command scanner, dependent on context state
void (*CSVector[])(struct Special *sp, int c) = {
	lemmaEdit,
	variantEdit,
	otherEdit,
};

void
	special(struct Ctx *ctx, int c)
{
	struct Special *sp = ctx->active;

	assert( ctx->active != 0 );
	switch (c) {
	default:
		putchar(c);
		break;
	case '}':
		if (sp->keepBrace)
			putchar('}');
		deactivate(&ctx->active);
		break;
	case '[':
		sp->cs = csLemma;
		if (sp->lemma)
			lemmaEnd(&sp->lemma);
		sp->lemma = lemmaBegin();

		putchar(c);
		break;
	case '|':
		if (sp->cs == csLemma)
			lemmaScan(sp->lemma, c);
		else
			putchar(c);
		break;
	case ']':
		sp->cs = csOther;
		putchar(c);
		break;
	case '<':
		sp->cs = csVariants;
		normalize(ctx);
		break;
	case '\\':
		c = getchar();
		if (c == 'n')
			putchar('\n');
		else if (c != EOF && c != '\n')
			putchar(c);
		break;
	case '{':
		(*CSVector[sp->cs])(sp, c);
		break;
	case '"':
		putchar(c);
		copyUntil(c);
		break;
	}
}
