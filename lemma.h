
/*
	lemma.h - header for lemma.c
*/

struct Lemma;

enum LemmaOp { vKeep, vDrop, vAdd, vSwapA, vSwapB };

extern struct Lemma *lemmaBegin();
extern void lemmaEnd(struct Lemma **lm);
extern void lemmaEdit(struct Special *sp, int ch);
extern void lemmaApply(struct WitGrp *wg, struct Lemma *lm);
extern void lemmaScan(struct Lemma *lm, int c);
