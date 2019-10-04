/*
	norm.h - Interface for normalizing witness groups
*/

struct WitGrp;
struct EditList;
struct Pattern;

struct Special {
	enum CState cs;
	int keepBrace;

	struct EditList *edits;
	struct Lemma *lemma;

	struct Pattern *pattn;
};

/* Define buffer sizes */
#define CODSIZ 32		/* to hold coded variants */
#define WITSIZ 512		/* to hold list of witnesses */
#define TOKSIZ 128		/* to hold tokens */

#define endof(buf) ((buf) + sizeof buf)

extern struct WitGrp *wgNext(struct WitGrp *wg);
extern char *wgCodes(struct WitGrp *wg);
extern char *wgWitns(struct WitGrp *wg);

extern struct Lemma *spLemma(struct Special *sp);

extern char *append(char *s, const char *t, char *end);
